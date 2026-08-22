/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>

#include "hal/boot.h"
#include "hal/sys.h"

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

		// An erased boot section reads back as 0xFFFF. Jumping into it executes
		// erased flash until the program counter wraps, which presents as the
		// device hanging with no LEDs rather than as a missing bootloader, so
		// fall through to the application instead.
		if (pgm_read_word_far((uint32_t)BOOTLOADER_VECTOR * 2) == 0xFFFF) {
			return;
		}

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
	Sets the key and resets. bootloader_check() runs from .init3 on the way back
	up, sees the key plus the watchdog reset flag, and jumps to the bootloader.
*/
void bootloader_start(void) {
	boot_key = BOOTKEY;
	hal_system_reset();
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
