#include "pic.h"

static inline void outb(uint16_t port, uint8_t data) {
  __asm__ __volatile__("outb %b0, %w1" : : "a"(data), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
  uint8_t value;
  __asm__ __volatile__("inb %w1, %b0" : "=a"(value) : "Nd"(port));
  return value;
}

void pic_init(void) {
  // ICW1 - start inicjalizacji
  outb(PIC_MASTER_CMD, 0x11);
  outb(PIC_SLAVE_CMD, 0x11);

  // ICW2 - ustaw base vectors
  outb(PIC_MASTER_DATA, 0x20);  // Master IRQ0 -> INT32
  outb(PIC_SLAVE_DATA, 0x28);   // Slave IRQ0 -> INT40

  // ICW3 - setup cascading
  outb(PIC_MASTER_DATA, 1 << 2);  // Slave na IRQ2
  outb(PIC_SLAVE_DATA, 2);        // Slave cascade identity

  // ICW4 - 8086 mode
  outb(PIC_MASTER_DATA, 0x01);
  outb(PIC_SLAVE_DATA, 0x01);

  // Mask all IRQs initially (0xFF = all masked)
  outb(PIC_MASTER_DATA, 0xFF);
  outb(PIC_SLAVE_DATA, 0xFF);
}

void pic_send_eoi(uint8_t irq) {
  if (irq >= 8) {
    outb(PIC_SLAVE_CMD, PIC_EOI);
  }
  outb(PIC_MASTER_CMD, PIC_EOI);
}

void pic_set_irq_mask(uint8_t irq) {
  uint16_t port;
  uint8_t mask;

  port = (irq < 8) ? PIC_MASTER_DATA : PIC_SLAVE_DATA;
  irq = (irq < 8) ? irq : irq - 8;

  mask = inb(port) | (1 << irq);
  outb(port, mask);
}

void pic_clear_irq_mask(uint8_t irq) {
  uint16_t port;
  uint8_t mask;

  port = (irq < 8) ? PIC_MASTER_DATA : PIC_SLAVE_DATA;
  irq = (irq < 8) ? irq : irq - 8;

  mask = inb(port) & ~(1 << irq);
  outb(port, mask);
}
