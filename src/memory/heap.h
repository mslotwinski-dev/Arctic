#pragma once

#include <stdint.h>

/**
 * @file heap.h
 * @brief Kernel heap allocator API built on PMM frame allocations.
 * @ingroup memory_subsystem
 */

/**
 * @brief Initialize the kernel heap allocator.
 *
 * This function is idempotent. The first call requests one or more frames from
 * PMM and creates the initial free-list head block.
 */
void heap_init(void);

/**
 * @brief Allocate a heap block.
 *
 * The requested size is internally aligned to 8 bytes.
 *
 * @param size Number of payload bytes requested.
 * @return Pointer to payload memory, or 0 when allocation fails.
 */
void* kmalloc(uint32_t size);

/**
 * @brief Allocate and zero-initialize an array.
 *
 * @param count Number of elements.
 * @param size Size of one element in bytes.
 * @return Pointer to zeroed payload memory, or 0 on failure/overflow.
 */
void* kcalloc(uint32_t count, uint32_t size);

/**
 * @brief Resize an existing allocation.
 *
 * Semantics mirror standard C realloc-like behavior:
 * - ptr == 0 behaves like kmalloc(size)
 * - size == 0 behaves like kfree(ptr) and returns 0
 *
 * @param ptr Existing allocation pointer.
 * @param size New requested payload size in bytes.
 * @return Reused or newly allocated pointer, or 0 on failure.
 */
void* krealloc(void* ptr, uint32_t size);

/**
 * @brief Free a previously allocated heap block.
 *
 * Passing 0 is safe and has no effect.
 *
 * @param ptr Pointer returned by kmalloc/kcalloc/krealloc.
 */
void kfree(void* ptr);
