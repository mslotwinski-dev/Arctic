#pragma once

#include <stdint.h>

/**
 * @file elf.h
 * @brief ELF32 validation/loading helpers for initrd-backed binaries.
 * @ingroup proc_subsystem
 */

/**
 * @brief Validate whether memory buffer contains a supported ELF image.
 * @param data Pointer to candidate image bytes.
 * @param size Buffer size in bytes.
 * @return Non-zero when valid ELF image.
 */
uint8_t elf_validate_buffer(const void *data, uint32_t size);

/**
 * @brief Read entry point virtual address from ELF image.
 * @param data Pointer to ELF bytes.
 * @return Entry virtual address, or 0 when invalid.
 */
uint32_t elf_get_entry(const void *data);

/**
 * @brief Count PT_LOAD segments in ELF image.
 * @param data Pointer to ELF bytes.
 * @return Number of loadable segments.
 */
uint32_t elf_count_loadable_segments(const void *data);

/**
 * @brief Probe ELF metadata for image stored in VFS.
 * @param path VFS path.
 * @param entry Output entry virtual address.
 * @param load_segments Output PT_LOAD count.
 * @return Non-zero on success.
 */
uint8_t elf_probe_from_vfs(const char *path, uint32_t *entry, uint32_t *load_segments);

/**
 * @brief Load ELF segments from VFS image into kernel-chosen memory.
 * @param path VFS path.
 * @param entry Output entry virtual address.
 * @param load_segments Output PT_LOAD count.
 * @param loaded_bytes Output number of loaded bytes.
 * @return Non-zero on success.
 */
uint8_t elf_load_from_vfs(const char *path, uint32_t *entry, uint32_t *load_segments, uint32_t *loaded_bytes);

/**
 * @brief Load ELF and report both virtual and loaded entry addresses.
 * @param path VFS path.
 * @param entry_virtual Output entry virtual address from ELF header.
 * @param entry_loaded Output actual callable loaded entry address.
 * @param load_segments Output PT_LOAD count.
 * @param loaded_bytes Output number of loaded bytes.
 * @return Non-zero on success.
 */
uint8_t elf_load_image_from_vfs(const char *path, uint32_t *entry_virtual, uint32_t *entry_loaded, uint32_t *load_segments,
								uint32_t *loaded_bytes);

/**
 * @brief Identity-map-style load of ELF segments at their virtual addresses.
 * @param path VFS path.
 * @param entry_virtual Output entry virtual address.
 * @param load_segments Output PT_LOAD count.
 * @param loaded_bytes Output number of loaded bytes.
 * @return Non-zero on success.
 */
uint8_t elf_load_virtual_identity_from_vfs(const char *path, uint32_t *entry_virtual, uint32_t *load_segments,
								   uint32_t *loaded_bytes);

/**
 * @brief Attempt to transfer control to a previously loaded entry point.
 * @param loaded_entry Callable entry address.
 * @return Non-zero when control transfer routine completed successfully.
 */
uint8_t elf_try_execute_loaded_entry(uint32_t loaded_entry);
