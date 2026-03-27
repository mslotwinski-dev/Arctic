#pragma once

#include <stdint.h>

/**
 * @file paging.h
 * @brief Identity paging preparation and activation API.
 * @ingroup memory_subsystem
 */

/**
 * @brief Build an identity-mapped page directory/table set.
 * @param map_bytes Number of low-memory bytes to identity-map.
 */
void paging_prepare_identity(uint32_t map_bytes);

/**
 * @brief Check if paging structures were prepared.
 * @return Non-zero when prepared, 0 otherwise.
 */
uint8_t paging_is_prepared(void);

/**
 * @brief Number of pages currently mapped by the prepared identity setup.
 * @return Mapped page count.
 */
uint32_t paging_get_mapped_pages(void);

/**
 * @brief Upper bound (bytes) of identity-mapped region.
 * @return Byte limit.
 */
uint32_t paging_get_identity_limit_bytes(void);

/**
 * @brief Physical address of active/prepared page directory.
 * @return Physical address of page directory root.
 */
uint32_t paging_get_directory_physical(void);

/**
 * @brief Enable paging (typically by loading CR3 and setting CR0.PG).
 */
void paging_enable(void);
