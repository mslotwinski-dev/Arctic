#pragma once

#include <stdint.h>

/**
 * @file gdt.h
 * @brief Global Descriptor Table definitions and setup helpers.
 * @ingroup arch_x86
 */

/**
 * @brief Raw x86 GDT entry descriptor (8 bytes).
 */
typedef struct {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_mid;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
} __attribute__((packed)) GDT_Entry;

/**
 * @brief GDTR pseudo-register layout used by lgdt.
 */
typedef struct {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed)) GDT_Pointer;

/**
 * @brief Initialize a minimal flat kernel GDT and load it.
 */
void gdt_init(void);

/**
 * @brief Populate one GDT descriptor entry.
 *
 * @param num Entry index.
 * @param base Segment base.
 * @param limit Segment limit.
 * @param access Access byte flags.
 * @param gran Granularity/flags byte.
 */
void gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
