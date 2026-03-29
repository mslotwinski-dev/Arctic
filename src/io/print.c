#include "print.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

volatile char* video_memory = (volatile char*)0xB8000;

int cursor_x = 0;
int cursor_y = 0;

/**
 * @brief Update VGA hardware cursor position via I/O ports.
 * Cursor position in VGA is 16-bit linear offset (y * 80 + x).
 */
static void update_hardware_cursor(void) {
  unsigned int pos = cursor_y * VGA_WIDTH + cursor_x;
  unsigned char pos_high = (pos >> 8) & 0xFF;
  unsigned char pos_low = pos & 0xFF;
  
  /* outb requires: data in %al (8-bit), port as immediate or %dx */
  /* Write cursor position high byte (register 14) */
  __asm__ __volatile__("outb %b0, $0x3d4" : : "a"(14));
  __asm__ __volatile__("outb %b0, $0x3d5" : : "a"((int)pos_high));
  
  /* Write cursor position low byte (register 15) */
  __asm__ __volatile__("outb %b0, $0x3d4" : : "a"(15));
  __asm__ __volatile__("outb %b0, $0x3d5" : : "a"((int)pos_low));
}

void clear(void) {
  for (int y = 0; y < VGA_HEIGHT; y++) {
    for (int x = 0; x < VGA_WIDTH; x++) {
      int index = (y * VGA_WIDTH + x) * 2;
      video_memory[index] = ' ';
      video_memory[index + 1] = 0x0F;
    }
  }
  cursor_x = 0;
  cursor_y = 0;
  update_hardware_cursor();
}

void printc(const char c) {
  if (c == '\b') {
    if (cursor_x == 0 && cursor_y == 0) {
      return;
    }

    if (cursor_x > 0) {
      cursor_x--;
    } else {
      cursor_y--;
      cursor_x = VGA_WIDTH - 1;
    }

    int index = (cursor_y * VGA_WIDTH + cursor_x) * 2;
    video_memory[index] = ' ';
    video_memory[index + 1] = 0x0F;
    update_hardware_cursor();
    return;
  }

  if (c == '\n') {
    cursor_y++;
    cursor_x = 0;

    // Scrolling - jeśli kursor wychodzi poza ekran
    if (cursor_y >= VGA_HEIGHT) {
      // Przesunąć wszystko o 1 linię w górę
      for (int i = 0; i < VGA_HEIGHT - 1; i++) {
        for (int j = 0; j < VGA_WIDTH; j++) {
          int from_idx = ((i + 1) * VGA_WIDTH + j) * 2;
          int to_idx = (i * VGA_WIDTH + j) * 2;
          video_memory[to_idx] = video_memory[from_idx];
          video_memory[to_idx + 1] = video_memory[from_idx + 1];
        }
      }
      // Wyczyść ostatnią linię
      for (int j = 0; j < VGA_WIDTH; j++) {
        int idx = ((VGA_HEIGHT - 1) * VGA_WIDTH + j) * 2;
        video_memory[idx] = ' ';
        video_memory[idx + 1] = 0x0F;
      }
      cursor_y = VGA_HEIGHT - 1;
    }
    update_hardware_cursor();
    return;
  }

  // Bounds checking
  if (cursor_x >= VGA_WIDTH || cursor_y >= VGA_HEIGHT) {
    return;
  }

  int index = (cursor_y * VGA_WIDTH + cursor_x) * 2;
  video_memory[index] = c;
  video_memory[index + 1] = 0x0F;

  cursor_x++;

  if (cursor_x >= VGA_WIDTH) {
    cursor_x = 0;
    cursor_y++;

    if (cursor_y >= VGA_HEIGHT) {
      // Scrolling
      for (int i = 0; i < VGA_HEIGHT - 1; i++) {
        for (int j = 0; j < VGA_WIDTH; j++) {
          int from_idx = ((i + 1) * VGA_WIDTH + j) * 2;
          int to_idx = (i * VGA_WIDTH + j) * 2;
          video_memory[to_idx] = video_memory[from_idx];
          video_memory[to_idx + 1] = video_memory[from_idx + 1];
        }
      }
      for (int j = 0; j < VGA_WIDTH; j++) {
        int idx = ((VGA_HEIGHT - 1) * VGA_WIDTH + j) * 2;
        video_memory[idx] = ' ';
        video_memory[idx + 1] = 0x0F;
      }
      cursor_y = VGA_HEIGHT - 1;
    }
  }
  update_hardware_cursor();
}

void print(const char* str) {
  for (int i = 0; str[i] != '\0'; i++) {
    printc(str[i]);
  }
}

void println(const char* str) {
  print(str);
  printc('\n');
}

void set_cursor_position(int x, int y) {
  cursor_x = x;
  cursor_y = y;
  update_hardware_cursor();
}

void show_cursor_pos(void) {
  print("X:");
  printc('0' + (cursor_x / 10) % 10);
  printc('0' + cursor_x % 10);
  print(" Y:");
  printc('0' + (cursor_y / 10) % 10);
  printc('0' + cursor_y % 10);
  printc(' ');
}