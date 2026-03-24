#pragma once

#include <stdint.h>

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_CMD_PORT 0x64

#define PS2_STATUS_OUTPUT_BUFFER 0x01
#define PS2_STATUS_INPUT_BUFFER 0x02

void ps2_init(void);
uint8_t ps2_read_scancode(void);
char ps2_scancode_to_ascii(uint8_t scancode);
void keyboard_init(void);
