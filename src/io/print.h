#pragma once

/**
 * @file print.h
 * @brief Minimal text output helpers for kernel console.
 * @ingroup io_subsystem
 */

/**
 * @brief Clear console buffer and reset cursor position.
 */
void clear(void);

/**
 * @brief Print a single character to console.
 * @param c Character to print.
 */
void printc(const char c);

/**
 * @brief Print a null-terminated string.
 * @param str String to print.
 */
void print(const char* str);

/**
 * @brief Print a null-terminated string followed by newline.
 * @param str String to print.
 */
void println(const char* str);

/**
 * @brief Move hardware/text cursor to specified coordinates.
 * @param x Column position.
 * @param y Row position.
 */
void set_cursor_position(int x, int y);

/**
 * @brief Print current cursor coordinates for diagnostics.
 */
void show_cursor_pos(void);