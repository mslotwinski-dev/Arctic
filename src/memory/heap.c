#include "heap.h"

#include <stdint.h>

#include "pmm.h"

/**
 * @file heap.c
 * @brief Free-list based kernel heap allocator implementation.
 * @ingroup memory_subsystem
 *
 * Design summary:
 * - Heap grows in PMM frame-sized chunks.
 * - Every allocation block begins with a small header (heap_block_t).
 * - Allocation is first-fit with optional split.
 * - Free operation marks block free and then coalesces adjacent free blocks.
 */

/**
 * @brief Internal free-list node describing one heap block.
 */
typedef struct heap_block {
  uint32_t size;
  uint8_t free;
  struct heap_block* next;
} heap_block_t;

static heap_block_t* heap_head = 0;

/**
 * @brief Align requested payload size to 8-byte boundary.
 */
static uint32_t align8(uint32_t size) { return (size + 7U) & ~7U; }

/**
 * @brief Zero memory region without relying on libc.
 */
static void mem_zero(void* ptr, uint32_t size) {
  uint8_t* p = (uint8_t*)ptr;
  for (uint32_t i = 0; i < size; i++) {
    p[i] = 0;
  }
}

/**
 * @brief Copy memory region without relying on libc.
 */
static void mem_copy(void* dst, const void* src, uint32_t size) {
  uint8_t* d = (uint8_t*)dst;
  const uint8_t* s = (const uint8_t*)src;
  for (uint32_t i = 0; i < size; i++) {
    d[i] = s[i];
  }
}

/**
 * @brief Return last block in linked list.
 */
static heap_block_t* heap_last_block(void) {
  heap_block_t* current = heap_head;

  if (current == 0) {
    return 0;
  }

  while (current->next != 0) {
    current = current->next;
  }

  return current;
}

/**
 * @brief Grow heap list by requesting one or more PMM frames.
 *
 * @param min_payload Minimum payload size that should become available.
 * @return Pointer to first newly created block, or 0 on allocation failure.
 */
static heap_block_t* heap_grow(uint32_t min_payload) {
  uint32_t frame_addr = pmm_alloc_frame();
  heap_block_t* block;

  if (frame_addr == 0U) {
    return 0;
  }

  block = (heap_block_t*)frame_addr;
  block->size = PMM_FRAME_SIZE - (uint32_t)sizeof(heap_block_t);
  block->free = 1;
  block->next = 0;

  while (block->size < min_payload) {
    uint32_t next_frame = pmm_alloc_frame();
    heap_block_t* next_block;

    if (next_frame == 0U) {
      break;
    }

    next_block = (heap_block_t*)next_frame;
    next_block->size = PMM_FRAME_SIZE - (uint32_t)sizeof(heap_block_t);
    next_block->free = 1;
    next_block->next = 0;

    block->next = next_block;
    block = next_block;
  }

  return (heap_block_t*)frame_addr;
}

/**
 * @brief Split a free block into allocated block + trailing free remainder.
 */
static void split_block(heap_block_t* block, uint32_t wanted_size) {
  uint32_t remaining;
  heap_block_t* new_block;

  if (block == 0) {
    return;
  }

  if (block->size <= wanted_size + sizeof(heap_block_t) + 8U) {
    return;
  }

  remaining = block->size - wanted_size - (uint32_t)sizeof(heap_block_t);
  new_block = (heap_block_t*)((uint8_t*)block + sizeof(heap_block_t) + wanted_size);

  new_block->size = remaining;
  new_block->free = 1;
  new_block->next = block->next;

  block->size = wanted_size;
  block->next = new_block;
}

/**
 * @brief Merge physically adjacent free blocks to reduce fragmentation.
 */
static void coalesce_blocks(void) {
  heap_block_t* current = heap_head;

  while (current != 0 && current->next != 0) {
    uint8_t* current_end = (uint8_t*)current + sizeof(heap_block_t) + current->size;

    if (current->free != 0 && current->next->free != 0 && current_end == (uint8_t*)current->next) {
      current->size += (uint32_t)sizeof(heap_block_t) + current->next->size;
      current->next = current->next->next;
      continue;
    }

    current = current->next;
  }
}

void heap_init(void) {
  if (heap_head != 0) {
    return;
  }

  heap_head = heap_grow(64U);
}

void* kmalloc(uint32_t size) {
  heap_block_t* current;
  uint32_t wanted_size;

  if (size == 0U) {
    return 0;
  }

  wanted_size = align8(size);

  if (heap_head == 0) {
    heap_init();
    if (heap_head == 0) {
      return 0;
    }
  }

  current = heap_head;
  while (current != 0) {
    if (current->free != 0 && current->size >= wanted_size) {
      split_block(current, wanted_size);
      current->free = 0;
      return (void*)((uint8_t*)current + sizeof(heap_block_t));
    }

    if (current->next == 0) {
      break;
    }

    current = current->next;
  }

  {
    heap_block_t* new_chunk = heap_grow(wanted_size);
    heap_block_t* tail = heap_last_block();

    if (new_chunk == 0) {
      return 0;
    }

    if (tail == 0) {
      heap_head = new_chunk;
    } else {
      tail->next = new_chunk;
    }
  }

  return kmalloc(wanted_size);
}

void* kcalloc(uint32_t count, uint32_t size) {
  uint32_t total;
  void* ptr;

  if (count == 0U || size == 0U) {
    return 0;
  }

  total = count * size;
  if (size != 0U && total / size != count) {
    return 0;
  }

  ptr = kmalloc(total);
  if (ptr != 0) {
    mem_zero(ptr, total);
  }

  return ptr;
}

void* krealloc(void* ptr, uint32_t size) {
  heap_block_t* block;
  void* new_ptr;

  if (ptr == 0) {
    return kmalloc(size);
  }

  if (size == 0U) {
    kfree(ptr);
    return 0;
  }

  block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));

  if (block->size >= size) {
    return ptr;
  }

  new_ptr = kmalloc(size);
  if (new_ptr == 0) {
    return 0;
  }

  mem_copy(new_ptr, ptr, block->size);
  kfree(ptr);
  return new_ptr;
}

void kfree(void* ptr) {
  heap_block_t* block;

  if (ptr == 0) {
    return;
  }

  block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
  block->free = 1;

  coalesce_blocks();
}
