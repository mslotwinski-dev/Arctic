#pragma once

#include <stdint.h>

/**
 * @file pic.h
 * @brief 8259A PIC initialization and IRQ acknowledge/mask control.
 * @ingroup arch_x86
 */

/** @brief Master PIC command port. */
#define PIC_MASTER_CMD 0x20
/** @brief Master PIC data (mask) port. */
#define PIC_MASTER_DATA 0x21
/** @brief Slave PIC command port. */
#define PIC_SLAVE_CMD 0xA0
/** @brief Slave PIC data (mask) port. */
#define PIC_SLAVE_DATA 0xA1

/** @brief End Of Interrupt command value sent to PIC command ports. */
#define PIC_EOI 0x20

/**
 * @brief Remap and initialize master/slave PIC controllers.
 */
void pic_init(void);

/**
 * @brief Send End Of Interrupt command for a handled IRQ line.
 * @param irq IRQ number in range 0..15.
 */
void pic_send_eoi(uint8_t irq);

/**
 * @brief Mask (disable) a specific IRQ line on PIC.
 * @param irq IRQ number in range 0..15.
 */
void pic_set_irq_mask(uint8_t irq);

/**
 * @brief Unmask (enable) a specific IRQ line on PIC.
 * @param irq IRQ number in range 0..15.
 */
void pic_clear_irq_mask(uint8_t irq);
