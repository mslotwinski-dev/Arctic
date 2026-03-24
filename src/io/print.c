#include "print.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

volatile char* video_memory = (volatile char*)0xB8000;

int cursor_x = 0;
int cursor_y = 0;

void clear(void) {
  for (int y = 0; y < VGA_HEIGHT; y++) {
    for (int x = 0; x < VGA_WIDTH; x++) {
      // Każdy znak zajmuje 2 bajty (1 bajt to znak, 2 bajt to kolor)
      int index = (y * VGA_WIDTH + x) * 2;
      video_memory[index] = ' ';       // Pusty znak (spacja)
      video_memory[index + 1] = 0x0F;  // Kolor: biały tekst na czarnym tle
    }
  }
  cursor_x = 0;
  cursor_y = 0;
}

void printc(const char c) {
  if (c == '\n') {
    cursor_y++;
    cursor_x = 0;
    return;
  }

  int index = (cursor_y * VGA_WIDTH + cursor_x) * 2;
  video_memory[index] = c;
  video_memory[index + 1] = 0x0F;

  cursor_x++;

  if (cursor_x >= VGA_WIDTH) {
    cursor_x = 0;
    cursor_y++;
  }
}

void print(const char* str) {
  for (int i = 0; str[i] != '\0'; i++) {
    printc(str[i]);
  }
}