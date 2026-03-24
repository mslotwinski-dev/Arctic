#pragma once

#include <stdint.h>

#define PIT_CH0 0x40
#define PIT_CH1 0x41
#define PIT_CH2 0x42
#define PIT_CTRL 0x43

#define PIT_FREQ 1193182  // PIT frequency in Hz

void pit_init(void);
void pit_set_frequency(uint32_t hz);
