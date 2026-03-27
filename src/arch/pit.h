#pragma once

#include <stdint.h>

/**
 * @file pit.h
 * @brief Programmable Interval Timer (PIT) configuration interface.
 * @ingroup arch_x86
 */

/** @brief PIT channel 0 data port. */
#define PIT_CH0 0x40
/** @brief PIT channel 1 data port. */
#define PIT_CH1 0x41
/** @brief PIT channel 2 data port. */
#define PIT_CH2 0x42
/** @brief PIT mode/command register port. */
#define PIT_CTRL 0x43

/** @brief Input oscillator frequency of PIT in Hertz. */
#define PIT_FREQ 1193182  // PIT frequency in Hz

/**
 * @brief Initialize PIT to kernel default tick frequency.
 */
void pit_init(void);

/**
 * @brief Program PIT channel 0 to requested interrupt frequency.
 * @param hz Desired tick frequency in Hertz.
 */
void pit_set_frequency(uint32_t hz);
