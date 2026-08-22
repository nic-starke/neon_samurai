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
#include "event/animation.h"
#include "hal/boot.h"
#include "hal/init.h"
#include "hal/sys.h"
#include "led/led.h"
#include "midi/midi.h"
#include "midi/sysex.h"
#include "midi/webui_bridge.h"
#include "system/hardware.h"
#include "system/print.h"
#include "system/rng.h"
#include "system/time.h"
#include "usb/usb.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define INIT_OR_PANIC(call)                                                    \
	do {                                                                         \
		if ((call) != SUCCESS) {                                                   \
			hal_panic();                                                             \
		}                                                                          \
	} while (0)
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */

struct mf_rt gRT = {
		.curr_bank							 = 0,
		.live_position_streaming = false,
};

struct sys_config gCONFIG = {
		.enc_dead_time			= DEFAULT_ENC_PLAYDEAD_TIME,
		.midi_throttle_time = DEFAULT_MIDI_THROTTLE_TIME,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

// Entry point
__attribute__((noreturn)) int main(void) {
	avr_xmega128a4u_init(); // Init the AVR xmega peripherals

	rng_init();

	/*
		None of these have a degraded mode - a failed event channel registration
		or an unstarted systick leaves the firmware unable to do its job at all,
		so stop with the panel lit rather than run in a state that looks alive
		but silently does nothing.
	*/
	INIT_OR_PANIC(event_init());
	INIT_OR_PANIC(midi_init());
	INIT_OR_PANIC(animation_init());
	INIT_OR_PANIC(display_init());

	input_init();

	INIT_OR_PANIC(mf_sysex_init());
	INIT_OR_PANIC(webui_bridge_init());
	INIT_OR_PANIC(systime_start());
	INIT_OR_PANIC(usb_init());
#ifdef ENABLE_CONSOLE
	console_init();
#endif

	// Enable system interrupts (required for input and led processing)
	sei();

	// Check if the user requested a reset, or is holding the four corner
	// encoders to request bootloader entry.
	uint32_t time				= systime_ms();
	bool		 reset			= false;
	bool		 bootloader = false;
	do {
		input_update(); // Need to update input to read button state
		reset			 = is_reset_pressed();
		bootloader = is_bootloader_gesture_pressed();
		if (reset || bootloader)
			break;
	} while (systime_ms() - time < 200);

	if (bootloader) {
		bootloader_start(); // Does not return - resets into DFU bootloader
	}

	INIT_OR_PANIC(cfg_init(reset));
	INIT_OR_PANIC(cfg_load());

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
		console_update();
#endif
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
