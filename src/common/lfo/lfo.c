/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "lfo/lfo.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define SAMPLE_RATE			1000 // Update rate in Hz
#define TWO_PI					(2 * 314) // Using scaled value for 2 * π (multiplied by 100)
#define MAX_PHASE				(2 * 314) // Max phase value corresponding to 2 * π
#define AMPLITUDE_SCALE 1 // Scaling factor for amplitude and other values

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

i16 fixed_sine(i16 phase) {
	// Normalize phase to [0, 2π] range and use approximation
	i16 angle = (phase * 360) / MAX_PHASE;
	if (angle < 90) {
		return (angle * 314) / 90; // 314 is an approximation of π
	} else if (angle < 180) {
		return 314 - ((angle - 90) * 314) / 90;
	} else if (angle < 270) {
		return -((angle - 180) * 314) / 90;
	} else {
		return -314 + ((angle - 270) * 314) / 90;
	}
}

/*
Implements the 5-order polynomial approximation to sin(x).
@param i   angle (with 2^15 units/circle)
@return    16 bit fixed point Sine value (4.12) (ie: +4096 = +1 & -4096 = -1)

The result is accurate to within +- 1 count. ie: +/-2.44e-4.
*/
int16_t fpsin(int16_t i) {
	/* Convert (signed) input to a value between 0 and 8192. (8192 is pi/2, which
	 * is the region of the curve fit). */
	/* ------------------------------------------------------------------- */
	i <<= 1;
	uint8_t c = i < 0; // set carry for output pos/neg

	if (i ==
			(i |
			 0x4000)) // flip input value to corresponding value in range [0..8192)
		i = (1 << 15) - i;
	i = (i & 0x7FFF) >> 1;
	/* ------------------------------------------------------------------- */

	/* The following section implements the formula:
	 = y * 2^-n * ( A1 - 2^(q-p)* y * 2^-n * y * 2^-n * [B1 - 2^-r * y * 2^-n * C1
	* y]) * 2^(a-q) Where the constants are defined as follows:
	*/
	enum {
		A1 = 3370945099UL,
		B1 = 2746362156UL,
		C1 = 292421UL
	};
	enum {
		n = 13,
		p = 32,
		q = 31,
		r = 3,
		a = 12
	};

	uint32_t y = (C1 * ((uint32_t)i)) >> n;
	y					 = B1 - (((uint32_t)i * y) >> r);
	y					 = (uint32_t)i * (y >> n);
	y					 = (uint32_t)i * (y >> n);
	y					 = A1 - (y >> (p - q));
	y					 = (uint32_t)i * (y >> n);
	y					 = (y + (1UL << (q - a - 1))) >> (q - a); // Rounding

	return c ? -y : y;
}

i16 lfo_update(lfo_s* lfo) {
	// Calculate the phase increment (scaled)
	i16 phaseIncrement = (TWO_PI * lfo->frequency) / lfo->sampleRate;

	// Update the phase
	lfo->phase += phaseIncrement;

	// Wrap the phase to stay within 0 to TWO_PI
	if (lfo->phase >= MAX_PHASE) {
		lfo->phase -= MAX_PHASE;
	}

	// Calculate the output value based on the waveform type
	i16 output;
	switch (lfo->waveform) {
		case WAVEFORM_SINE:
			output = (lfo->amplitude * fpsin(lfo->phase)) / AMPLITUDE_SCALE;
			break;
		case WAVEFORM_SAW:
			output = (lfo->amplitude * (2 * lfo->phase / MAX_PHASE - 1));
			break;
		case WAVEFORM_SQUARE:
			output = (lfo->amplitude * (lfo->phase < MAX_PHASE / 2 ? 1 : -1));
			break;
		default:
			output = 0; // Should never reach here
			break;
	}

	return output;
}
