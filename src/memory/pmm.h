#pragma once

#include <stdint.h>

/**
 * @file pmm.h
 * @brief Physical memory manager (frame bitmap allocator) API.
 * @ingroup memory_subsystem
 */

/** @brief Physical frame size in bytes (4 KiB). */
#define PMM_FRAME_SIZE 4096U
/** @brief Maximum number of frames tracked by the bitmap. */
#define PMM_MAX_FRAMES 1048576U

/**
 * @brief Initialize PMM using Multiboot memory metadata.
 *
 * @param multiboot_info_addr Physical address of multiboot_info_t.
 * @param kernel_start Physical start address of kernel image.
 * @param kernel_end Physical end address of kernel image.
 */
void pmm_init(uint32_t multiboot_info_addr, uint32_t kernel_start, uint32_t kernel_end);

/**
 * @brief Allocate one 4 KiB physical frame.
 * @return Physical frame base address, or 0 when no frame is available.
 */
uint32_t pmm_alloc_frame(void);

/**
 * @brief Free a previously allocated physical frame.
 * @param frame_addr Physical frame base address (4 KiB aligned expected).
 */
void pmm_free_frame(uint32_t frame_addr);

/**
 * @brief Get total number of tracked frames.
 * @return Frame count.
 */
uint32_t pmm_get_total_frames(void);

/**
 * @brief Get number of currently free frames.
 * @return Frame count.
 */
uint32_t pmm_get_free_frames(void);
