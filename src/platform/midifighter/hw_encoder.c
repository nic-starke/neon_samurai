/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "sys/error.h"
#include "sys/rgb.h"
#include "sys/types.h"
#include "sys/utility.h"
#include "hal/avr/xmega/128a4u/gpio.h"
#include "input/quadrature.h"
#include "input/switch.h"
#include "lfo/lfo.h"
#include "virtmap/virtmap.h"

#include "midifighter/midifighter.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PORT_SR_ENC				 (PORTC) // IO port for encoder IO shift registers

#define PIN_SR_ENC_LATCH	 (0) // 74HC595N
#define PIN_SR_ENC_CLOCK	 (1)
#define PIN_SR_ENC_DATA_IN (2)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

quadrature_s mf_enc_quad[MF_NUM_ENCODERS];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static switch_x16_ctx_s switch_ctx;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void hw_encoder_init(void) {
	// Configure GPIO for encoder IO shift regsiters
	gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_LATCH, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN, GPIO_INPUT);

	// Latch initial encoder data
	gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 0);
	gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 1);

	for (uint i = 0; i < MF_NUM_ENCODERS; i++) {
		mf_enc_quad[i].dir = 0;
		mf_enc_quad[i].rot = 0;
	}
}

// Scan the hardware state of the midifighter and update local contexts
void hw_encoder_scan(void) {
	// Latch the IO levels into the shift registers
	gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 1);

	// Clock the 16 data bits for the encoder switches
	u16 swstates = 0;
	for (size_t i = 0; i < MF_NUM_ENCODER_SWITCHES; i++) {
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);
		u8 state = !(bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
		swstates |= (state << i);
	}

	// Execute the debounce and update routine for the switches
	switch_x16_update(&switch_ctx, swstates);

	// Clock the 32 bits for the 2x16 quadrature encoder signals, and update
	// encoder state.
	for (int i = 0; i < MF_NUM_ENCODERS; ++i) {
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
		u8 ch_a = (bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
		u8 ch_b = (bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);

		quadrature_update(&mf_enc_quad[i], ch_a, ch_b);
	}

	// Close the door!
	gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 0);
}

switch_state_e hw_enc_switch_state(uint idx) {
	assert(idx < MF_NUM_ENCODER_SWITCHES);

	if (switch_was_pressed(&switch_ctx, idx)) {
		return SWITCH_PRESSED;
	}

	if (switch_was_released(&switch_ctx, idx)) {
		return SWITCH_RELEASED;
	}

	return SWITCH_IDLE;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
