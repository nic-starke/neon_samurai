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
		gQUAD_ENC[i].dir	 = DIR_ST;
		gQUAD_ENC[i].rot	 = 0;
		gQUAD_ENC[i].accum = 0;
	}
}

/*
	One pass over the 74HC165 chain: 16 switch bits then 32 quadrature bits, all
	on the same three pins. Driven through the port registers directly rather
	than gpio_*() - this runs every main loop iteration, and at 48 bits the call
	overhead dominated the actual work.
*/
void hw_encoder_scan(void) {
	const u8 clock = (u8)(1u << PIN_SR_ENC_CLOCK);
	const u8 data	 = (u8)(1u << PIN_SR_ENC_DATA_IN);
	const u8 latch = (u8)(1u << PIN_SR_ENC_LATCH);

	// Latch the IO levels into the shift registers
	PORT_SR_ENC.OUTSET = latch;

	// Clock the 16 data bits for the encoder switches - active low.
	u16 swstates = 0;

	for (u8 i = 0; i < NUM_ENCODER_SWITCHES; i++) {
		PORT_SR_ENC.OUTCLR = clock;

		if ((PORT_SR_ENC.IN & data) == 0) {
			swstates |= (u16)(1u << i);
		}

		PORT_SR_ENC.OUTSET = clock;
	}

	// Execute the debounce and update routine for the switches
	switch_x16_update(&switch_ctx, swstates);

	// Clock the 32 bits for the 2x16 quadrature encoder signals, and update
	// encoder state.
	for (u8 i = 0; i < NUM_ENCODERS; ++i) {
		PORT_SR_ENC.OUTCLR = clock;
		const u8 ch_a			 = (PORT_SR_ENC.IN & data) ? 1u : 0u;
		PORT_SR_ENC.OUTSET = clock;

		PORT_SR_ENC.OUTCLR = clock;
		const u8 ch_b			 = (PORT_SR_ENC.IN & data) ? 1u : 0u;
		PORT_SR_ENC.OUTSET = clock;

		quadrature_update(&gQUAD_ENC[i], ch_a, ch_b);
	}

	// Close the door!
	PORT_SR_ENC.OUTCLR = latch;
}

bool hw_enc_switch_held(u8 idx) {
	assert(idx < NUM_ENCODER_SWITCHES);

	return (switch_x16_states(&switch_ctx) & (1u << idx)) != 0;
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
