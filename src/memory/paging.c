#include "paging.h"

#include <stdint.h>

#define PAGE_SIZE 4096U
#define PAGE_ENTRIES 1024U
#define PAGING_TABLE_COUNT 4U

#define PAGE_FLAG_PRESENT 0x1U
#define PAGE_FLAG_RW 0x2U

typedef uint32_t page_table_t[PAGE_ENTRIES];

static uint32_t page_directory[PAGE_ENTRIES] __attribute__((aligned(4096)));
static page_table_t page_tables[PAGING_TABLE_COUNT] __attribute__((aligned(4096)));

static uint8_t paging_prepared = 0;
static uint32_t paging_mapped_pages = 0;

void paging_prepare_identity(uint32_t map_bytes) {
  uint32_t max_pages = PAGING_TABLE_COUNT * PAGE_ENTRIES;
  uint32_t requested_pages = (map_bytes + PAGE_SIZE - 1U) / PAGE_SIZE;
  uint32_t page_count = requested_pages;

  if (page_count == 0U) {
    page_count = 1U;
  }

  if (page_count > max_pages) {
    page_count = max_pages;
  }

  for (uint32_t i = 0; i < PAGE_ENTRIES; i++) {
    page_directory[i] = 0;
  }

  for (uint32_t t = 0; t < PAGING_TABLE_COUNT; t++) {
    for (uint32_t i = 0; i < PAGE_ENTRIES; i++) {
      uint32_t page_index = t * PAGE_ENTRIES + i;
      uint32_t phys_addr = page_index * PAGE_SIZE;

      if (page_index < page_count) {
        page_tables[t][i] = phys_addr | PAGE_FLAG_PRESENT | PAGE_FLAG_RW;
      } else {
        page_tables[t][i] = 0;
      }
    }

    if (t * PAGE_ENTRIES < page_count) {
      page_directory[t] = ((uint32_t)&page_tables[t]) | PAGE_FLAG_PRESENT | PAGE_FLAG_RW;
    }
  }

  paging_mapped_pages = page_count;
  paging_prepared = 1;
}

uint8_t paging_is_prepared(void) { return paging_prepared; }

uint32_t paging_get_mapped_pages(void) { return paging_mapped_pages; }

uint32_t paging_get_identity_limit_bytes(void) { return paging_mapped_pages * PAGE_SIZE; }

uint32_t paging_get_directory_physical(void) { return (uint32_t)&page_directory; }

void paging_enable(void) {
  uint32_t cr0;
  uint32_t dir = paging_get_directory_physical();

  if (paging_prepared == 0U) {
    return;
  }

  __asm__ __volatile__("mov %0, %%cr3" : : "r"(dir));
  __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
  cr0 |= 0x80000000U;
  __asm__ __volatile__("mov %0, %%cr0" : : "r"(cr0));
}
