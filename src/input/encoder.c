/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "input/encoder.h"
#include "event/event.h"
#include "event/io.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define VEL_INC 2

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Acceleration constants
static i16 accel_curve[] = {VEL_INC * 8, VEL_INC * 10, VEL_INC * 15,
														VEL_INC * 30, VEL_INC * 50};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int encoder_init(encoder_s* enc) {
	assert(enc);

	enc->accel_mode	 = 1;
	enc->accel_const = 0;
	enc->curr_val		 = 0;
	enc->prev_val		 = 0;
	enc->velocity		 = 0;
	return 0;
}

void encoder_update(encoder_s* enc, int direction) {
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
			enc->accel_const = (enc->accel_const + 1) % COUNTOF(accel_curve);
		} else {
			enc->accel_const = 0;
			enc->velocity		 = 0;
		}

		// Update the direction
		enc->direction = direction;

		// Update the velocity
		enc->velocity +=
				accel_curve[enc->accel_const] * enc->accel_const * direction;
	}

	// Apply the velocity
	i32 newval =
			enc->curr_val + CLAMP(enc->velocity, -ENC_MAX_VELOCITY, ENC_MAX_VELOCITY);
	enc->curr_val = CLAMP(newval, ENC_MIN, ENC_MAX);

	// Generate an event if the encoder changed position
	if (enc->curr_val != enc->prev_val) {
		io_event_s evt;
		evt.type = EVT_IO_ENCODER_ROTATION;
		evt.ctx	 = enc;
		event_post(EVENT_CHANNEL_IO, &evt);
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
