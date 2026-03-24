#include "../io/print.h"

void kernel_main(void) __asm__("kernel_main");

void kernel_main(void) {
  clear();

  print("Arctic OS!\n");
  print("Mój pierwszy system operacyjny jebac microsoft\n");
  print("");
  print("Hei");
  print("Hei");
}