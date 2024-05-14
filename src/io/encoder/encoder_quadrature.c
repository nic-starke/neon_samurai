/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "core/core_types.h"
#include "event/events_io.h"

#include "io/encoder/encoder_quadrature.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define VEL_INC 2

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
	QUAD_START,
	QUAD_CCW,
	QUAD_CW,
	QUAD_MIDDLE,
	QUAD_MID_CW,
	QUAD_MID_CCW,

	QUAD_NB,
} quad_state_e;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Rotory decoder based on
 * https://github.com/buxtronix/arduino/tree/master/libraries/Rotary
 * Copyright 2011 Ben Buxton.
 * Licenced under the GNU GPL Version 3.
 * Contact: bb@cactii.net
 */
static const quad_state_e quad_states[QUAD_NB][4] = {
		// Current Quadrature GrayCode
		{QUAD_MIDDLE, QUAD_CW, QUAD_CCW, QUAD_START},
		{QUAD_MIDDLE | DIR_CCW, QUAD_START, QUAD_CCW, QUAD_START},
		{QUAD_MIDDLE | DIR_CW, QUAD_CW, QUAD_START, QUAD_START},
		{QUAD_MIDDLE, QUAD_MID_CCW, QUAD_MID_CW, QUAD_START},
		{QUAD_MIDDLE, QUAD_MIDDLE, QUAD_MID_CW, QUAD_START | DIR_CW},
		{QUAD_MIDDLE, QUAD_MID_CCW, QUAD_MIDDLE, QUAD_START | DIR_CCW},
};

// Acceleration constants
static i16 accel_inc[] = {VEL_INC * 3, VEL_INC * 8, VEL_INC * 15, VEL_INC * 30,
													VEL_INC * 50};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int encoder_init(iodev_s* dev) {
	assert(dev);
	encoder_s* enc = (encoder_s*)dev->ctx;

	enc->quad.dir		 = DIR_ST;
	enc->quad.rot		 = 0;
	enc->accel_mode	 = 1;
	enc->accel_const = 0;
	enc->curr_val		 = 0;
	enc->prev_val		 = 0;
	enc->velocity		 = 0;

	io_event_s evt;
	evt.type										= EVT_IO_ENCODER_ROTATION;
	evt.dev											= dev;
	evt.data.enc_rotation.value = 0;

	return event_post(EVENT_CHANNEL_IO, &evt);
}

void encoder_update(iodev_s* dev, uint ch_a, uint ch_b) {
	assert(dev);

	encoder_s* enc = (encoder_s*)dev->ctx;

	// Decode the quadrature encoding
	unsigned int val = (ch_b << 1) | ch_a;
	enc->quad.rot		 = quad_states[enc->quad.rot & 0x0F][val];
	enc->quad.dir		 = enc->quad.rot & 0x30;

	i32 newval;
	int direction;

	switch (enc->quad.dir) {
		case DIR_CCW: direction = -1; break;
		case DIR_CW: direction = 1; break;
		case DIR_ST: direction = 0; break;
	}

	enc->prev_val = enc->curr_val;
	if (enc->accel_mode == 0) {
		if (direction == 0) {
			return;
		}

		enc->velocity = 1500 * direction;
	} else {
		// If the encoder stopped moving then decelerate
		if (direction == 0) {
			enc->velocity =
					(enc->velocity > 0) ? enc->velocity - 1 : enc->velocity + 1;
			return;
		}

		// Accelerate if the direction is the same, otherwise reset the acceleration
		if (enc->direction == direction) {
			enc->accel_const = (enc->accel_const + 1) % COUNTOF(accel_inc);
		} else {
			enc->accel_const = 0;
			enc->velocity		 = 0;
		}

		// Update the direction
		enc->direction = direction;

		// Update the velocity
		enc->velocity += accel_inc[enc->accel_const] * enc->accel_const * direction;
	}

	// Update the stored value
	newval =
			enc->curr_val + CLAMP(enc->velocity, -ENC_MAX_VELOCITY, ENC_MAX_VELOCITY);
	enc->curr_val = CLAMP(newval, ENC_MIN, ENC_MAX);

	// Generate an event if the encoder changed position
	if (enc->curr_val != enc->prev_val) {
		io_event_s evt;
		evt.type										= EVT_IO_ENCODER_ROTATION;
		evt.dev											= dev;
		evt.data.enc_rotation.value = enc->curr_val;
		event_post(EVENT_CHANNEL_IO, &evt);
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
