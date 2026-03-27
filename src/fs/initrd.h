#pragma once

#include <stdint.h>

/**
 * @file initrd.h
 * @brief Initrd module parser and read-only file lookup API.
 * @ingroup fs_subsystem
 */

/**
 * @brief Initialize initrd metadata from Multiboot module list.
 * @param multiboot_info_addr Physical address of multiboot_info_t.
 */
void initrd_init(uint32_t multiboot_info_addr);

/**
 * @brief Check whether initrd parsing was successful.
 * @return Non-zero when initrd is available.
 */
uint8_t initrd_is_ready(void);

/**
 * @brief Number of discovered files in initrd.
 * @return File count.
 */
uint32_t initrd_file_count(void);

/**
 * @brief Resolve file contents by path/name.
 *
 * @param name File name/path inside initrd.
 * @param size Optional output size in bytes (may be 0).
 * @return Pointer to file bytes, or 0 when not found.
 */
const void* initrd_get_file_data(const char* name, uint32_t* size);

/**
 * @brief Get file size by path/name.
 * @param name File name/path inside initrd.
 * @param size Output size in bytes.
 * @return Non-zero on success, 0 when file is not found.
 */
uint8_t initrd_get_file_size_by_name(const char* name, uint32_t* size);

/**
 * @brief Get file name by index from initrd listing.
 * @param index Zero-based file index.
 * @return File name string, or 0 when index is invalid.
 */
const char* initrd_get_file_name(uint32_t index);

/**
 * @brief Get file size by index from initrd listing.
 * @param index Zero-based file index.
 * @param size Output size in bytes.
 * @return Non-zero on success, 0 when index is invalid.
 */
uint8_t initrd_get_file_size(uint32_t index, uint32_t* size);

/**
 * @brief Count files matching a path prefix.
 * @param prefix Prefix filter (for example "bin/"). Empty string matches all.
 * @return Number of matching files.
 */
uint32_t initrd_count_prefix(const char* prefix);

/**
 * @brief Get matching file name by index within prefix-filtered results.
 * @param prefix Prefix filter.
 * @param index Zero-based index within filtered results.
 * @return File name string, or 0 when no such filtered entry exists.
 */
const char* initrd_get_file_name_by_prefix(const char* prefix, uint32_t index);
