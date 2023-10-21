/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "core/core_types.h"
#include "core/core_utility.h"
#include "core/core_encoder.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Velocity increment
#define VEL_INC 1

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
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Rotory decoder based on
 * https://github.com/buxtronix/arduino/tree/master/libraries/Rotary Copyright
 * 2011 Ben Buxton. Licenced under the GNU GPL Version 3. Contact: bb@cactii.net
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
static i16 accel_inc[] = {VEL_INC * 2, VEL_INC * 10, VEL_INC * 20, VEL_INC * 40,
                          VEL_INC * 80};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void core_quadrature_decode(quadrature_ctx_s* ctx, unsigned int ch_a,
														unsigned int ch_b) {
	unsigned int val = (ch_b << 1) | ch_a;
	ctx->rot_state	 = quad_states[ctx->rot_state & 0x0F][val];
	ctx->dir				 = ctx->rot_state & 0x30;
}

void core_encoder_update(encoder_ctx_s* enc, int direction) {
	assert(enc);

	i32 newval = 0;

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

	newval =
			enc->curr_val + CLAMP(enc->velocity, -ENC_MAX_VELOCITY, ENC_MAX_VELOCITY);
	enc->curr_val = CLAMP(newval, ENC_MIN, ENC_MAX);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
