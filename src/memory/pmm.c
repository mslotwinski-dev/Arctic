#include "pmm.h"

#include <stdint.h>

typedef struct {
  uint32_t flags;
  uint32_t mem_lower;
  uint32_t mem_upper;
  uint32_t boot_device;
  uint32_t cmdline;
  uint32_t mods_count;
  uint32_t mods_addr;
  uint32_t syms0;
  uint32_t syms1;
  uint32_t syms2;
  uint32_t syms3;
  uint32_t mmap_length;
  uint32_t mmap_addr;
} __attribute__((packed)) multiboot_info_t;

typedef struct {
  uint32_t mod_start;
  uint32_t mod_end;
  uint32_t cmdline;
  uint32_t pad;
} __attribute__((packed)) multiboot_module_t;

typedef struct {
  uint32_t size;
  uint64_t addr;
  uint64_t len;
  uint32_t type;
} __attribute__((packed)) multiboot_mmap_entry_t;

static uint8_t pmm_bitmap[PMM_MAX_FRAMES / 8];
static uint32_t pmm_total_frames = PMM_MAX_FRAMES;
static uint32_t pmm_free_frames = 0;

static inline uint32_t pmm_bit_index(uint32_t frame) { return frame >> 3; }
static inline uint8_t pmm_bit_mask(uint32_t frame) { return (uint8_t)(1U << (frame & 7U)); }

static void pmm_mark_frame_used(uint32_t frame) {
  uint32_t byte_index;
  uint8_t mask;

  if (frame >= pmm_total_frames) {
    return;
  }

  byte_index = pmm_bit_index(frame);
  mask = pmm_bit_mask(frame);

  if ((pmm_bitmap[byte_index] & mask) == 0U) {
    pmm_bitmap[byte_index] |= mask;
    if (pmm_free_frames > 0U) {
      pmm_free_frames--;
    }
  }
}

static void pmm_mark_frame_free(uint32_t frame) {
  uint32_t byte_index;
  uint8_t mask;

  if (frame >= pmm_total_frames) {
    return;
  }

  byte_index = pmm_bit_index(frame);
  mask = pmm_bit_mask(frame);

  if ((pmm_bitmap[byte_index] & mask) != 0U) {
    pmm_bitmap[byte_index] &= (uint8_t)(~mask);
    pmm_free_frames++;
  }
}

static void pmm_mark_region(uint32_t start, uint32_t length, uint8_t used) {
  uint64_t start64 = (uint64_t)start;
  uint64_t end64 = start64 + (uint64_t)length;
  uint32_t first_frame;
  uint32_t last_frame;

  if (length == 0U) {
    return;
  }

  if (end64 > 0x100000000ULL) {
    end64 = 0x100000000ULL;
  }

  if (used != 0U) {
    first_frame = (uint32_t)(start64 / PMM_FRAME_SIZE);
    last_frame = (uint32_t)((end64 + PMM_FRAME_SIZE - 1ULL) / PMM_FRAME_SIZE);
  } else {
    first_frame = (uint32_t)((start64 + PMM_FRAME_SIZE - 1ULL) / PMM_FRAME_SIZE);
    last_frame = (uint32_t)(end64 / PMM_FRAME_SIZE);
  }

  if (last_frame > pmm_total_frames) {
    last_frame = pmm_total_frames;
  }

  for (uint32_t frame = first_frame; frame < last_frame; frame++) {
    if (used != 0U) {
      pmm_mark_frame_used(frame);
    } else {
      pmm_mark_frame_free(frame);
    }
  }
}

void pmm_init(uint32_t multiboot_info_addr, uint32_t kernel_start, uint32_t kernel_end) {
  multiboot_info_t* mbi = (multiboot_info_t*)multiboot_info_addr;
  uint32_t bitmap_size = sizeof(pmm_bitmap);

  for (uint32_t i = 0; i < bitmap_size; i++) {
    pmm_bitmap[i] = 0xFF;
  }

  pmm_free_frames = 0;

  if ((mbi->flags & (1U << 6)) != 0U) {
    uint32_t mmap_end = mbi->mmap_addr + mbi->mmap_length;
    multiboot_mmap_entry_t* entry = (multiboot_mmap_entry_t*)mbi->mmap_addr;

    while ((uint32_t)entry < mmap_end) {
      if (entry->type == 1U) {
        uint64_t region_addr = entry->addr;
        uint64_t region_len = entry->len;

        if (region_addr < 0x100000000ULL) {
          uint32_t free_start = (uint32_t)region_addr;
          uint64_t max_len = 0x100000000ULL - region_addr;
          if (region_len > max_len) {
            region_len = max_len;
          }
          pmm_mark_region(free_start, (uint32_t)region_len, 0);
        }
      }

      entry = (multiboot_mmap_entry_t*)((uint32_t)entry + entry->size + sizeof(entry->size));
    }
  }

  pmm_mark_region(0, 0x00100000U, 1);

  if (kernel_end > kernel_start) {
    pmm_mark_region(kernel_start, kernel_end - kernel_start, 1);
  }

  pmm_mark_region(multiboot_info_addr, sizeof(multiboot_info_t), 1);

  if ((mbi->flags & (1U << 3)) != 0U) {
    multiboot_module_t* modules = (multiboot_module_t*)mbi->mods_addr;
    pmm_mark_region(mbi->mods_addr, mbi->mods_count * (uint32_t)sizeof(multiboot_module_t), 1);

    for (uint32_t i = 0; i < mbi->mods_count; i++) {
      uint32_t module_start = modules[i].mod_start;
      uint32_t module_end = modules[i].mod_end;
      if (module_end > module_start) {
        pmm_mark_region(module_start, module_end - module_start, 1);
      }
    }
  }

  if ((mbi->flags & (1U << 6)) != 0U) {
    pmm_mark_region(mbi->mmap_addr, mbi->mmap_length, 1);
  }
}

uint32_t pmm_alloc_frame(void) {
  for (uint32_t frame = 0; frame < pmm_total_frames; frame++) {
    uint32_t byte_index = pmm_bit_index(frame);
    uint8_t mask = pmm_bit_mask(frame);

    if ((pmm_bitmap[byte_index] & mask) == 0U) {
      pmm_mark_frame_used(frame);
      return frame * PMM_FRAME_SIZE;
    }
  }

  return 0;
}

void pmm_free_frame(uint32_t frame_addr) {
  uint32_t frame = frame_addr / PMM_FRAME_SIZE;
  pmm_mark_frame_free(frame);
}

uint32_t pmm_get_total_frames(void) { return pmm_total_frames; }
uint32_t pmm_get_free_frames(void) { return pmm_free_frames; }
