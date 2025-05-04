/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */


#include "LUFA/Common/Common.h"
#include "LUFA/Drivers/USB/USB.h"
#include "LUFA/Platform/XMEGA/ClockManagement.h"


#include "console/console.h"
#include "event/event.h"
#include "hal/init.h"
#include "led/led.h"
#include "midi/midi.h"
#include "midi/sysex.h"
#include "system/hardware.h"
#include "system/print.h"
#include "system/rng.h"
#include "system/time.h"
#include "usb/usb.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */

struct mf_rt gRT = {
		.curr_bank = 0,
};

struct sys_config gCONFIG = {
		.enc_dead_time			= DEFAULT_ENC_PLAYDEAD_TIME,
		.midi_throttle_time = DEFAULT_MIDI_THROTTLE_TIME,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

// Entry point
__attribute__((noreturn)) void main(void) {
	avr_xmega128a4u_init(); // Init the AVR xmega peripherals

	rng_init();
	event_init();
	midi_init();
	display_init();
	input_init();
	mf_sysex_init();
	systime_start();
	usb_init();
#ifdef ENABLE_CONSOLE
	console_init();
#endif

	// Enable system interrupts (required for input and led processing)
	sei();

	// Check if the user requested a reset
	uint32_t time	 = systime_ms();
	bool		 reset = false;
	do {
		input_update(); // Need to update input to read button state
		reset = is_reset_pressed();
		if (reset)
			break;
	} while (systime_ms() - time < 200);

	// Initialize config
	cfg_init(reset);

	// Load configuration - either defaults if reset occurred, or existing from EEPROM
	cfg_load();

	hw_led_init();

	println_pmem("Init done");

	while (1) {
		input_update();
		event_update();
		display_update();
		midi_update();
		usb_update();
		cfg_update();
#ifdef ENABLE_CONSOLE
		console_update(); // Update the console module in the main loop
#endif
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
