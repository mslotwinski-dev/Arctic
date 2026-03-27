#pragma once

#include <stdint.h>

/**
 * @file idt.h
 * @brief Interrupt Descriptor Table structures and configuration API.
 * @ingroup arch_x86
 */

/**
 * @brief Raw x86 IDT gate entry (interrupt/trap gate descriptor).
 */
typedef struct {
  uint16_t base_low;
  uint16_t selector;
  uint8_t zero;
  uint8_t flags;
  uint16_t base_high;
} __attribute__((packed)) IDT_Entry;

/**
 * @brief IDTR pseudo-register layout used by lidt.
 */
typedef struct {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed)) IDT_Pointer;

/**
 * @brief Build and load the IDT with CPU exception and IRQ handlers.
 */
void idt_init(void);

/**
 * @brief Set one IDT gate entry.
 *
 * @param num Interrupt vector index.
 * @param base Handler function address.
 * @param sel Code segment selector.
 * @param flags Gate attributes and privilege flags.
 */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
