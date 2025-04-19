/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "platform/midifighter/midifighter.h"
#include "platform/midifighter/sysex.h"

#include "hal/avr/xmega/128a4u/init.h"

#include "LUFA/Common/Common.h"
#include "LUFA/Drivers/USB/USB.h"
#include "LUFA/Platform/XMEGA/ClockManagement.h"

#include "sys/time.h"
#include "sys/print.h"
#include "usb/usb.h"
#include "protocol/midi/midi.h"
#include "event/event.h"
#include "display/display.h"
#include "common/console/console.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */

mf_rt_s gRT = {
		.curr_bank = 0,
};

sys_config_s gCONFIG = {
		.enc_dead_time			= DEFAULT_ENC_PLAYDEAD_TIME,
		.midi_throttle_time = DEFAULT_MIDI_THROTTLE_TIME,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

// Entry point
__attribute__((noreturn)) void main(void) {
	avr_xmega128a4u_init(); // Init the AVR xmega peripherals

	event_init();
	midi_init();
	mf_input_init();
	mf_sysex_init();
	systime_start();
	usb_init();
#ifdef VSER_ENABLE
	console_init();
#endif

	// Enable system interrupts (required for input and led processing)
	sei();

	// Check if the user requested a reset
	uint32_t time	 = systime_ms();
	bool		 reset = false;
	do {
		mf_input_update(); // Need to update input to read button state
		reset = mf_is_reset_pressed();
		if (reset)
			break;
	} while (systime_ms() - time < 200);

	// Initialize config
	mf_cfg_init(reset);

	// Load configuration - either defaults if reset occurred, or existing from EEPROM
	mf_cfg_load();

	hw_led_init();

	println_pmem("Init done");

	while (1) {
		mf_input_update();
		event_update();
		display_update();
		midi_update();
		usb_update();
		mf_cfg_update();
#ifdef VSER_ENABLE
		console_update(); // Update the console module in the main loop
#endif
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
