#pragma once

#include <stdint.h>

typedef struct {
  uint16_t base_low;
  uint16_t selector;
  uint8_t zero;
  uint8_t flags;
  uint16_t base_high;
} __attribute__((packed)) IDT_Entry;

typedef struct {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed)) IDT_Pointer;

void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
