#pragma once

#include <stdint.h>

/**
 * @file keyboard.h
 * @brief PS/2 keyboard controller helpers and character input API.
 * @ingroup io_subsystem
 */

/** @brief PS/2 data port (read scancodes / write device data). */
#define PS2_DATA_PORT 0x60
/** @brief PS/2 controller status register port. */
#define PS2_STATUS_PORT 0x64
/** @brief PS/2 controller command register port. */
#define PS2_CMD_PORT 0x64

/** @brief Status bit: output buffer has unread data. */
#define PS2_STATUS_OUTPUT_BUFFER 0x01
/** @brief Status bit: input buffer is full (controller busy). */
#define PS2_STATUS_INPUT_BUFFER 0x02

/**
 * @brief Initialize PS/2 keyboard controller state used by the driver.
 */
void ps2_init(void);

/**
 * @brief Read one raw keyboard scancode from controller.
 * @return Scancode byte.
 */
uint8_t ps2_read_scancode(void);

/**
 * @brief Convert a set-1 scancode to ASCII when possible.
 * @param scancode Raw keyboard scancode byte.
 * @return ASCII character or 0 when not representable.
 */
char ps2_scancode_to_ascii(uint8_t scancode);

/**
 * @brief Initialize keyboard interrupt-driven input path.
 */
void keyboard_init(void);

/**
 * @brief Pop one character from the keyboard input buffer.
 * @param out_char Output character pointer.
 * @return Non-zero on success, 0 when buffer is empty.
 */
int keyboard_pop_char(char* out_char);
