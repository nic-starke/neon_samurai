/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2025) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifndef HAL_AVR_XMEGA_128A4U_SYS_H
#define HAL_AVR_XMEGA_128A4U_SYS_H

#include <avr/io.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Performs a software reset of the microcontroller.
 *
 * This function uses the Watchdog Timer (WDT) to trigger a system reset.
 * It disables interrupts, configures the WDT for the shortest timeout,
 * enables the WDT, and then enters an infinite loop until the WDT resets
 * the device.
 *
 * @note This function does not return.
 */
__attribute__((noreturn)) void hal_system_reset(void);

/**
 * @brief Stop dead with every RGB LED lit red. Does not return.
 *
 * For conditions the firmware cannot run through. assert() is not usable for
 * this: NDEBUG is set in both release build types, and avr-libc's assert()
 * calls abort(), which spins silently with no indication on the panel.
 */
__attribute__((noreturn)) void hal_panic(void);

#ifdef __cplusplus
}
#endif

#endif // HAL_AVR_XMEGA_128A4U_SYS_H
