#include "pit.h"

static inline void outb(uint16_t port, uint8_t data) {
  __asm__ __volatile__("outb %b0, %w1" : : "a"(data), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
  uint8_t value;
  __asm__ __volatile__("inb %w1, %b0" : "=a"(value) : "Nd"(port));
  return value;
}

static inline void io_wait(void) { __asm__ __volatile__("outb %%al, $0x80" : : "a"(0)); }

void pit_set_frequency(uint32_t hz) {
  uint32_t divisor = PIT_FREQ / hz;

  // Mode 2 (0x34): Rate Generator - periodic interrupts
  // Bits: 6-7 (channel 0), 4-5 (both bytes), 1-3 (mode 2)
  outb(PIT_CTRL, 0x34);
  io_wait();

  // Send divisor (16-bit) - low byte first, then high byte
  outb(PIT_CH0, (uint8_t)(divisor & 0xFF));
  io_wait();
  outb(PIT_CH0, (uint8_t)((divisor >> 8) & 0xFF));
  io_wait();
}

void pit_init(void) {
  // Set timer to 100 Hz (generates 100 interrupts per second)
  pit_set_frequency(100);
}
