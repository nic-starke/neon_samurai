/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2025) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "hal/sys.h"
#include "system/hardware.h"
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

	// WDT_CEN_bm must be set in the same write as the change, so the shortest
	// timeout and the enable go in together behind one CCP unlock.
	CCP			 = CCP_IOREG_gc;
	WDT.CTRL = WDT_PER_8CLK_gc | WDT_ENABLE_bm | WDT_CEN_bm;

	// Wait indefinitely for the WDT to reset the MCU
	while (1)
		;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

__attribute__((noreturn)) void hal_panic(void) {
	cli();
	hw_led_panic_red();

	while (1) {
		;
	}
}
