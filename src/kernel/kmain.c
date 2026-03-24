#include "../arch/gdt.h"
#include "../arch/idt.h"
#include "../arch/pic.h"
#include "../drivers/keyboard.h"
#include "../io/print.h"

extern void enable_interrupts(void);

void kernel_main(void) __asm__("kernel_main");

void kernel_main(void) {
  clear();

  println("  ,---.  ,------.  ,-----.,--------.,--. ,-----.                   ");
  println(" /  O  \\ |  .--. ''  .--./'--.  .--'|  |'  .--./     ,---.  ,---.  ");
  println("|  .-.  ||  '--'.'|  |       |  |   |  ||  |        | .-. |(  .-'  ");
  println("|  | |  ||  |\\  \\ '  '--'\\   |  |   |  |'  '--'\\    ' '-' '.-'  `) ");
  println("`--' `--'`--' '--' `-----'   `--'   `--' `-----'     `---' `----'  ");

  println("Arctic OS!");

  print("GDT: ");
  gdt_init();
  println("OK");

  print("IDT: ");
  idt_init();
  println("OK");

  print("PIC: ");
  pic_init();
  println("OK");

  print("Keyboard: ");
  keyboard_init();
  println("OK");

  print("Enable IRQs: ");
  enable_interrupts();
  println("OK");

  println("Ready! Type something:");
}