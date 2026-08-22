/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>
#include <avr/wdt.h>

#include "hal/boot.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define BOOTKEY						0x99C0FFEE
#define BOOTLOADER_VECTOR ((BOOT_SECTION_START + 0x1FC) / 2)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// The boot key is placed in the no init section - it will not be initialised by
// crt0, meaning that its value will be retained AFTER a soft-reset. The boot
// key is checked at system startup and if its value matches BOOTKEY then the
// bootloader execution will jump to the bootloader.
__attribute__((section(".noinit"))) static uint32_t boot_key;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void bootloader_check(void) {
	// Check if the reset was caused by the watchdog timer, and that the bootkey
	// is valid.
	if (((RST.STATUS & RST_WDRF_bm)) && (boot_key == BOOTKEY)) {
		boot_key = 0; // Reset the bootkey to stop a bootloader loop.

		// The watchdog that produced this reset is still running - an xmega
		// watchdog reset does not clear WDT.CTRL. Left enabled it resets the
		// device again ~30ms after the jump below, long before the bootloader
		// can enumerate as a USB DFU device.
		wdt_disable();

		// RST.STATUS flags are sticky until written back, so clear the source
		// rather than leaving WDRF set for every subsequent boot.
		RST.STATUS = RST_WDRF_bm;

		/**
		 * Copied from the GCC AVR options documentation -
		 * https://gcc.gnu.org/onlinedocs/gcc-6.3.0/gcc/AVR-Options.html In
		 * order to facilitate indirect jump on devices with more than 128 Ki
		 * bytes of program memory space, there is a special function register
		 * called EIND that serves as most significant part of the target
		 * address when EICALL or EIJMP instructions are used.
		 * */
		EIND = (uint8_t)(BOOTLOADER_VECTOR >> 16);
		((void (*)(void))(uint16_t)BOOTLOADER_VECTOR)();
	}
}

/*
	This function starts the watchdog timer and then enters an infinite loop.
	The watchdog timer will reset the AVR after 30ms as the system did not
	"pet the dog". The bootloader_check function will then be called during
	the system startup (after reset), and because the bootkey was set to
	the required value, the bootloader will be executed.
*/
void bootloader_start(void) {
	boot_key = BOOTKEY;
	wdt_enable(WDTO_30MS);
	while (1) {}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
