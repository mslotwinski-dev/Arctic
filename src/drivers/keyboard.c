#include "keyboard.h"

#include "../arch/pic.h"
#include "../io/print.h"

static inline uint8_t inb(uint16_t port) {
  uint8_t value;
  __asm__ __volatile__("inb %w1, %b0" : "=a"(value) : "Nd"(port));
  return value;
}

static inline void outb(uint16_t port, uint8_t data) {
  __asm__ __volatile__("outb %b0, %w1" : : "a"(data), "Nd"(port));
}

#define SHIFT_FLAG 0x80
#define KBD_BUFFER_SIZE 128

static unsigned char ps2_normal[] = {0,    27,  '1', '2', '3', '4', '5', '6', '7', '8', '9',  '0', '-', '=',  '\b',
                                     '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',  '[', ']', '\n', 0,
                                     'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,   '\\', 'z',
                                     'x',  'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,   '*',  0,   ' ', 0};

static unsigned char ps2_shifted[] = {0,    27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',  '\b',
                                      '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
                                      'A',  'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|',  'Z',
                                      'X',  'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0};

static uint8_t shift_pressed = 0;
static char kbd_buffer[KBD_BUFFER_SIZE];
static uint32_t kbd_head = 0;
static uint32_t kbd_tail = 0;

static void keyboard_push_char(char c) {
  uint32_t next_head = (kbd_head + 1U) % KBD_BUFFER_SIZE;
  if (next_head == kbd_tail) {
    return;
  }

  kbd_buffer[kbd_head] = c;
  kbd_head = next_head;
}

int keyboard_pop_char(char* out_char) {
  if (out_char == 0) {
    return 0;
  }

  if (kbd_head == kbd_tail) {
    return 0;
  }

  *out_char = kbd_buffer[kbd_tail];
  kbd_tail = (kbd_tail + 1U) % KBD_BUFFER_SIZE;
  return 1;
}

void ps2_init(void) {
  // Minimal PS2 init - skip blocking waits
  // Just enable scanning
  outb(PS2_CMD_PORT, 0xAE);
}

uint8_t ps2_read_scancode(void) {
  while (!(inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_BUFFER));
  return inb(PS2_DATA_PORT);
}

char ps2_scancode_to_ascii(uint8_t scancode) {
  if (scancode & 0x80) {
    // Key release
    if ((scancode & 0x7F) == 0x2A || (scancode & 0x7F) == 0x36) {
      shift_pressed = 0;
    }
    return 0;
  }

  // Key press
  if (scancode == 0x2A || scancode == 0x36) {
    shift_pressed = 1;
    return 0;
  }

  unsigned char* table = shift_pressed ? ps2_shifted : ps2_normal;

  if (scancode < sizeof(ps2_normal)) {
    return table[scancode];
  }

  return 0;
}

void keyboard_init(void) {
  ps2_init();
  pic_clear_irq_mask(0);  // Enable IRQ0 (timer)
  pic_clear_irq_mask(1);  // Enable IRQ1 (keyboard)
  println("KBD: IRQs unmask");
}

static uint32_t timer_ticks = 0;

void timer_handler(void) {
  timer_ticks++;

  // Print dot for every 10th interrupt (approximately 1 per second at 100Hz)
  // if (timer_ticks % 10 == 0) {
  // printc('.');
  // }
  pic_send_eoi(0);
}

void keyboard_handler(void) {
  // Debug: verify handler is called
  // printc('K');

  // Non-blocking scancode read
  uint8_t status = inb(PS2_STATUS_PORT);
  if (status & PS2_STATUS_OUTPUT_BUFFER) {
    uint8_t scancode = inb(PS2_DATA_PORT);
    char c = ps2_scancode_to_ascii(scancode);

    if (c != 0) {
      keyboard_push_char(c);
      printc(c);
    }
  }

  pic_send_eoi(1);
}
