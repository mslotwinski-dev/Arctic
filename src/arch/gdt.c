#include "gdt.h"

#define GDT_ENTRIES 3

GDT_Entry gdt_table[GDT_ENTRIES];
GDT_Pointer gdt_ptr;

extern void gdt_flush(uint32_t);

void gdt_init(void) {
  gdt_ptr.limit = (sizeof(GDT_Entry) * GDT_ENTRIES) - 1;
  gdt_ptr.base = (uint32_t)&gdt_table;

  gdt_set_gate(0, 0, 0, 0, 0);                 // Null descriptor
  gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);  // Code segment
  gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);  // Data segment

  gdt_flush((uint32_t)&gdt_ptr);
}

void gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
  gdt_table[num].base_low = base & 0xFFFF;
  gdt_table[num].base_mid = (base >> 16) & 0xFF;
  gdt_table[num].base_high = (base >> 24) & 0xFF;

  gdt_table[num].limit_low = limit & 0xFFFF;
  gdt_table[num].granularity = (limit >> 16) & 0x0F;
  gdt_table[num].granularity |= gran & 0xF0;
  gdt_table[num].access = access;
}
