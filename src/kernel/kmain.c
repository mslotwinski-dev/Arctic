#include "../arch/gdt.h"
#include "../arch/idt.h"
#include "../arch/pic.h"
#include "../arch/pit.h"
#include "../drivers/keyboard.h"
#include "../io/print.h"

extern void enable_interrupts(void);

void kernel_main(void) __asm__("kernel_main");

void hello_world(void) {
  println("  ,---.  ,------.  ,-----.,--------.,--. ,-----.                   ");
  println(" /  O  \\ |  .--. ''  .--./'--.  .--'|  |'  .--./     ,---.  ,---.  ");
  println("|  .-.  ||  '--'.'|  |       |  |   |  ||  |        | .-. |(  .-'  ");
  println("|  | |  ||  |\\  \\ '  '--'\\   |  |   |  |'  '--'\\    ' '-' '.-'  `) ");
  println("`--' `--'`--' '--' `-----'   `--'   `--' `-----'     `---' `----'  ");
}

void kernel_main(void) {
  clear();

  hello_world();

  print("GDT: ");
  gdt_init();
  println("OK");

  print("IDT: ");
  idt_init();
  println("OK");

  print("PIC: ");
  pic_init();
  println("OK");

  print("PIT: ");
  pit_init();
  println("OK");

  print("KBD: ");
  keyboard_init();
  println("OK");

  print("IRQ: ");
  enable_interrupts();
  println("OK");

  println("Ready! Type something:");

  while (1) {
  }
}