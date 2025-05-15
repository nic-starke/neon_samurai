/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2025) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "hal/sys.h"
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/atomic.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

__attribute__((noreturn)) void hal_system_reset(void) {
	// Disable interrupts to prevent interference during reset sequence
	cli();

	// Ensure WDT is disabled before changing configuration
	// This requires Configuration Change Protection (CCP)
	CCP			 = CCP_IOREG_gc;
	WDT.CTRL = 0x00; // Disable WDT

	// Wait for synchronization if necessary (usually very short)
	while (WDT.STATUS & WDT_SYNCBUSY_bm)
		;

	// Configure WDT for shortest timeout period (e.g., 8ms) and enable Reset mode
	// WDT_PER_8CLK_gc is the shortest, but might be too fast. Let's use 8ms.
	// WDT_ENABLE_bm enables the WDT
	// WDT_CEN_bm allows changing the WDT settings
	CCP			 = CCP_IOREG_gc;
	WDT.CTRL = WDT_PER_8CLK_gc | WDT_ENABLE_bm | WDT_CEN_bm;

	// Wait indefinitely for the WDT to reset the MCU
	while (1)
		;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
