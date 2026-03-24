#include "idt.h"

#define IDT_ENTRIES 256

IDT_Entry idt_table[IDT_ENTRIES];
IDT_Pointer idt_ptr;

extern void idt_flush(uint32_t);

// IRQ handlers declarations
extern void irq0(void);
extern void irq1(void);

void idt_init(void) {
  idt_ptr.base = (uint32_t)&idt_table;
  idt_ptr.limit = (sizeof(IDT_Entry) * IDT_ENTRIES) - 1;

  // Wyzeruj tabelę IDT
  for (int i = 0; i < IDT_ENTRIES; i++) {
    idt_table[i].base_low = 0;
    idt_table[i].base_high = 0;
    idt_table[i].selector = 0;
    idt_table[i].zero = 0;
    idt_table[i].flags = 0;
  }

  // Ustaw handler dla IRQ0 (timer)
  idt_set_gate(32, (uint32_t)&irq0, 0x08, 0x8E);

  // Ustaw handler dla IRQ1 (keyboard)
  idt_set_gate(33, (uint32_t)&irq1, 0x08, 0x8E);

  idt_flush((uint32_t)&idt_ptr);
}

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
  idt_table[num].base_low = base & 0xFFFF;
  idt_table[num].base_high = (base >> 16) & 0xFFFF;
  idt_table[num].selector = sel;
  idt_table[num].zero = 0;
  idt_table[num].flags = flags;
}
