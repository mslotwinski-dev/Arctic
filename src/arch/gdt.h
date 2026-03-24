#pragma once

#include <stdint.h>

typedef struct {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_mid;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
} __attribute__((packed)) GDT_Entry;

typedef struct {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed)) GDT_Pointer;

void gdt_init(void);
void gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
