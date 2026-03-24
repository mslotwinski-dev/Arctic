#include <stdint.h>

#include "../drivers/keyboard.h"
#include "../io/print.h"

struct registers {
  uint32_t gs;
  uint32_t fs;
  uint32_t es;
  uint32_t ds;
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
  uint32_t int_no;
  uint32_t err_code;
  uint32_t eip, cs, eflags, useresp, ss;
};

extern void keyboard_handler(void);
extern void timer_handler(void);

void isr_handler(struct registers regs __attribute__((unused))) {
  // ISR handler dla exception'ów - tu możesz dodać obsługę divide by zero, etc
  // Na razie nic nie robimy
}

void irq_handler(struct registers regs) {
  switch (regs.int_no - 32) {
    case 0:  // Timer IRQ
      timer_handler();
      break;
    case 1:  // Keyboard IRQ
      keyboard_handler();
      break;
    default: break;
  }
}
