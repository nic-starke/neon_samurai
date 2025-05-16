/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/error.h"
#include "system/types.h"
#include "system/utility.h"
#include "hal/gpio.h"
#include "io/quadrature.h"
#include "io/switch.h"
#include "lfo/lfo.h"

#include "system/hardware.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PORT_SR_ENC				 (PORTC) // IO port for encoder IO shift registers

#define PIN_SR_ENC_LATCH	 (0) // 74HC595N
#define PIN_SR_ENC_CLOCK	 (1)
#define PIN_SR_ENC_DATA_IN (2)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct quadrature gQUAD_ENC[NUM_ENCODERS];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static struct switch_x16_ctx switch_ctx;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void hw_encoder_init(void) {
	// Configure GPIO for encoder IO shift regsiters
	gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_LATCH, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN, GPIO_INPUT);

	// Latch initial encoder data
	gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 0);
	gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 1);

	for (uint i = 0; i < NUM_ENCODERS; i++) {
		gQUAD_ENC[i].dir = 0;
		gQUAD_ENC[i].rot = 0;
	}
}

// Scan the hardware state of the midifighter and update local contexts
void hw_encoder_scan(void) {
	// Latch the IO levels into the shift registers
	gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 1);

	// Clock the 16 data bits for the encoder switches
	u16 swstates = 0;
	for (size_t i = 0; i < NUM_ENCODER_SWITCHES; i++) {
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
		u8 state = !(bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);
		swstates |= (state << i);
	}

	// Execute the debounce and update routine for the switches
	switch_x16_update(&switch_ctx, swstates);

	// Clock the 32 bits for the 2x16 quadrature encoder signals, and update
	// encoder state.
	for (int i = 0; i < NUM_ENCODERS; ++i) {
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
		u8 ch_a = (bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
		u8 ch_b = (bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);

		quadrature_update(&gQUAD_ENC[i], ch_a, ch_b);
	}

	// Close the door!
	gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 0);
}

enum switch_state hw_enc_switch_state(u8 idx) {
	assert(idx < NUM_ENCODER_SWITCHES);

	if (switchx16_was_pressed(&switch_ctx, idx)) {
		return SWITCH_PRESSED;
	}

	if (switchx16_was_released(&switch_ctx, idx)) {
		return SWITCH_RELEASED;
	}

	return SWITCH_IDLE;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
