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

#define VEL_INC 1

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Acceleration constants
static i16 accel_curve[] = {VEL_INC,			VEL_INC * 5,	VEL_INC * 15,
														VEL_INC * 30, VEL_INC * 60, VEL_INC * 100,
														VEL_INC * 200};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int encoder_init(encoder_s* enc) {
	assert(enc);
	enc->accel_mode = 0;
	enc->accel_tick = 0;
	enc->velocity		= 0;
	return 0;
}

void encoder_update(encoder_s* enc, int direction) {
	if (enc->accel_mode == 0) {
		if (direction == 0) {
			enc->velocity = 0;
			return;
		}

		enc->direction = (i8)direction;
		enc->velocity	 = 5 * direction;
		// enc->velocity = ENC_MAX_VELOCITY * direction;
	} else {

		// if (direction == 0) {
		// 	enc->velocity = enc->velocity - (enc->velocity / 4);
		// }

		if (direction == enc->direction) {
			if (enc->accel_tick < (COUNTOF(accel_curve) - 1)) {
				enc->accel_tick++;
			}
		} else {
			if (enc->accel_tick > 0) {
				enc->accel_tick--;
			}
		}
		enc->velocity =
				(accel_curve[enc->accel_tick] * enc->accel_tick * direction);

		enc->direction = (i8)direction;
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
