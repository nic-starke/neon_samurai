/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~  /
	The side switches are connected to the gpio pins PA0-PA5, and can therefore
	be read using the GPIO peripheral. The switches are active low, meaning that
	when the switch is pressed, the pin is pulled low. The switches are pulled
	up by default, so when the switch is not pressed, the pin is high.

	The pin mapping is as follows:

	SIDE_SW6 -> PORTA Pin 5)
	SIDE_SW5 -> PORTA Pin 4)
	SIDE_SW4 -> PORTA Pin 3)
	SIDE_SW3 -> PORTA Pin 0)
	SIDE_SW2 -> PORTA Pin 1)
	SIDE_SW1 -> PORTA Pin 2)

*/

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>

#include "hal/gpio.h"
#include "system/hardware.h"
#include "console/console.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PORT_SW (PORTA) // IO port for side-switches

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static struct switch_x8_ctx switch_ctx;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void hw_switch_init(void) {
	// Configure gpios for side switches
	for (u8 i = 0; i < NUM_SIDE_SWITCHES; ++i) {
		gpio_dir(&PORT_SW, i, GPIO_INPUT);
		gpio_mode(&PORT_SW, i, PORT_OPC_PULLUP_gc);
	}
}

void hw_switch_update(void) {
	// Read the GPIO states for all side switches
	u8 switch_states = 0;

	// Read each switch and set corresponding bit in the switch_states bitfield
	// Side switches are active low (pulled up by default, press connects to ground)
	for (int i = 0; i < NUM_SIDE_SWITCHES; i++) {
		// Read switch: logical NOT because switches are active low
		u8 sw_state = gpio_get(&PORT_SW, i);
		switch_states |= (sw_state ? 0 : (1 << i));
	}

	// Update the switch context with the new GPIO states
	switch_x8_update(&switch_ctx, switch_states);
}

enum switch_state hw_side_switch_state(u8 idx) {
	assert(idx < NUM_SIDE_SWITCHES);

	if (switchx8_was_pressed(&switch_ctx, idx)) {
			return SWITCH_PRESSED;
	} else if (switchx8_was_released(&switch_ctx, idx)) {
			return SWITCH_RELEASED;
	} else {
			return SWITCH_IDLE;
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
