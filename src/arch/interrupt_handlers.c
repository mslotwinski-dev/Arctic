#include <stdint.h>

#include "../drivers/keyboard.h"
#include "../io/print.h"
#include "../proc/syscall.h"
#include "pic.h"

/**
 * @file interrupt_handlers.c
 * @brief Common C handlers for CPU exceptions, IRQs, and syscall vector.
 * @ingroup arch_x86
 */

/**
 * @brief Register snapshot layout pushed by ISR/IRQ assembly stubs.
 */
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

/** @brief ISR callback servicing keyboard IRQ1. */
extern void keyboard_handler(void);
/** @brief ISR callback servicing PIT timer IRQ0. */
extern void timer_handler(void);

static const char* exception_messages[32] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating Point",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating Point",
    "Virtualization",
    "Control Protection",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection",
    "VMM Communication",
    "Security",
    "Reserved",
};

static void print_uint(uint32_t value) {
  char buffer[11];
  int pos = 0;

  if (value == 0) {
    printc('0');
    return;
  }

  while (value > 0 && pos < 10) {
    buffer[pos++] = (char)('0' + (value % 10));
    value /= 10;
  }

  for (int i = pos - 1; i >= 0; i--) {
    printc(buffer[i]);
  }
}

/**
 * @brief CPU exception and syscall-vector handler.
 *
 * Handles int 0x80 syscall dispatch specially. Other exceptions are printed
 * and the kernel halts.
 *
 * @param regs Saved register frame including vector number and error code.
 */
void isr_handler(struct registers* regs) {
  uint32_t int_no = regs->int_no;
  const char* reason = "Unknown";

  if (int_no == 128U) {
    regs->eax = syscall_dispatch(regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);
    return;
  }

  if (int_no < 32) {
    reason = exception_messages[int_no];
  }

  println("\nKERNEL EXCEPTION");
  print("INT: ");
  print_uint(int_no);
  print(" - ");
  println(reason);
  print("ERR: ");
  print_uint(regs->err_code);
  println("\nSystem halted.");

  __asm__ __volatile__("cli");
  for (;;) {
    __asm__ __volatile__("hlt");
  }
}

/**
 * @brief IRQ dispatcher called from assembly stubs after PIC remap.
 * @param regs Saved register frame including interrupt vector number.
 */
void irq_handler(struct registers* regs) {
  switch (regs->int_no - 32) {
    case 0:  // Timer IRQ
      timer_handler();
      break;
    case 1:  // Keyboard IRQ
      keyboard_handler();
      break;
    default: pic_send_eoi((uint8_t)(regs->int_no - 32)); break;
  }
}
