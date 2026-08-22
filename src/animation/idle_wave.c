/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*
	The shape of the idle wave, kept apart from the drawing of it.

	Nothing here touches a peripheral, a timer or the frame buffer, so it can
	be built for a host and tested directly - which is the only practical way
	to check that the wavefront travels the way it is meant to, rather than
	flashing a device and watching it.
*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/pgmspace.h>

#include "system/types.h"
#include "animation/idle.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// The phase wraps with a mask rather than a modulo, which needs the step
// count to be a power of two.
_Static_assert((IDLE_WAVE_STEPS & (IDLE_WAVE_STEPS - 1u)) == 0,
							 "the wave phase wraps with a mask");

// Every encoder must be able to lag by its full start delay inside one
// period, or the wavefront wraps around and meets itself.
_Static_assert(((IDLE_GRID_COLS - 1u) * IDLE_COL_DELAY_STEPS) +
											 ((IDLE_GRID_ROWS - 1u) * IDLE_ROW_DELAY_STEPS) <
									 IDLE_WAVE_STEPS,
							 "the start delays span more than one wave period");

// The diagonal wavefront depends on a row starting sooner than the next
// encoder along the row below it.
_Static_assert(IDLE_ROW_DELAY_STEPS < IDLE_COL_DELAY_STEPS,
							 "the wave would travel along rows rather than diagonally");

// The scale factor must actually reduce, or the fade would never finish.
_Static_assert(IDLE_FADE_OUT_NUM < IDLE_FADE_OUT_DEN,
							 "the fade-out factor does not dim anything");

_Static_assert(IDLE_LEVEL_FLOOR < IDLE_LEVEL_PEAK,
							 "the envelope needs somewhere to swell to");

_Static_assert(IDLE_HUE_BASE + ((IDLE_NUM_ENCODERS - 1u) * IDLE_HUE_STEP) <
									 IDLE_HUE_WHEEL,
							 "the palette runs off the end of the hue wheel");

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
	One swell, as a raised cosine. Sampled rather than computed because the
	part has no floating point and this is drawn sixteen times a frame.
*/
static const u8 wave_envelope[IDLE_WAVE_STEPS] PROGMEM = {
		0,	 1,		2,	 5,		10,	 15,	21,	 29,	37,	 47,	57,	 67,	79,
		90,	 103, 115, 127, 140, 152, 165, 176, 188, 198, 208, 218, 226,
		234, 240, 245, 250, 253, 254, 255, 254, 253, 250, 245, 240, 234,
		226, 218, 208, 198, 188, 176, 165, 152, 140, 128, 115, 103, 90,
		79,	 67,	57,	 47,	37,	 29,	21,	 15,	10,	 5,		2,	 1,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

u8 idle_wave_offset(u8 encoder_idx) {
	if (encoder_idx >= IDLE_NUM_ENCODERS) {
		return 0;
	}

	const u8 row = (u8)(encoder_idx / IDLE_GRID_COLS);
	const u8 col = (u8)(encoder_idx % IDLE_GRID_COLS);

	// Measured from the last encoder, which leads the wave.
	const u8 col_lag = (u8)((IDLE_GRID_COLS - 1u) - col);
	const u8 row_lag = (u8)((IDLE_GRID_ROWS - 1u) - row);

	return (u8)((col_lag * IDLE_COL_DELAY_STEPS) +
							(row_lag * IDLE_ROW_DELAY_STEPS));
}

u8 idle_wave_level(u8 encoder_idx, u16 step) {
	const u8 offset = idle_wave_offset(encoder_idx);

	// Subtracting the offset would underflow early in the run, so the phase is
	// walked backwards by adding a whole period first.
	const u8 phase =
			(u8)((step + IDLE_WAVE_STEPS - offset) & (IDLE_WAVE_STEPS - 1u));

	const u8 shape = pgm_read_byte(&wave_envelope[phase]);

	// Map the envelope onto the floor-to-peak range.
	const u16 span = IDLE_LEVEL_PEAK - IDLE_LEVEL_FLOOR;
	return (u8)(IDLE_LEVEL_FLOOR + (((u16)shape * span) / 255u));
}

u16 idle_wave_hue(u8 encoder_idx) {
	if (encoder_idx >= IDLE_NUM_ENCODERS) {
		return IDLE_HUE_BASE;
	}

	/*
		The wave leads at the last encoder, and that is the one the palette is
		anchored to - it holds the base shade and the rest runs back from it.
	*/
	const u8 from_lead = (u8)((IDLE_NUM_ENCODERS - 1u) - encoder_idx);

	return (u16)(IDLE_HUE_BASE + ((u16)from_lead * IDLE_HUE_STEP));
}

void idle_dim_planes(u16 planes[IDLE_BCM_PLANES]) {
	u8 level[IDLE_LEDS_PER_ENC] = {0};

	// The frame buffer is active-low, so a clear bit is a lit LED.
	for (u8 p = 0; p < IDLE_BCM_PLANES; p++) {
		const u16 lit = (u16)~planes[p];

		for (u8 b = 0; b < IDLE_LEDS_PER_ENC; b++) {
			if (lit & (u16)(1u << b)) {
				level[b] |= (u8)(1u << p);
			}
		}
	}

	for (u8 b = 0; b < IDLE_LEDS_PER_ENC; b++) {
		level[b] = (u8)(((u16)level[b] * IDLE_FADE_OUT_NUM) / IDLE_FADE_OUT_DEN);
	}

	for (u8 p = 0; p < IDLE_BCM_PLANES; p++) {
		u16 state = 0;

		for (u8 b = 0; b < IDLE_LEDS_PER_ENC; b++) {
			if (level[b] & (u8)(1u << p)) {
				state |= (u16)(1u << b);
			}
		}

		planes[p] = (u16)~state;
	}
}
