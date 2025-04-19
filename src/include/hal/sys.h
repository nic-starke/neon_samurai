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

#ifdef __cplusplus
}
#endif

#endif // HAL_AVR_XMEGA_128A4U_SYS_H
