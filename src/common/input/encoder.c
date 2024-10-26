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
#include <stdint.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define ACCEL_MAX				4
#define ACCEL_THRESHOLD 1
#define DECEL_RATE			8
#define ACCEL_DIVISOR		8

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int encoder_init(encoder_s* enc) {
	assert(enc);
	enc->velocity		= 0;
	enc->accel_tick = 0;
	enc->direction	= 0;
	return 0;
}

void encoder_update(encoder_s* enc, int new_direction) {
	if (new_direction == 0) {
		// No movement, aggressively reduce acceleration and velocity
		if (enc->accel_tick > 0) {
			enc->accel_tick -= DECEL_RATE;
		} else if (enc->accel_tick < 0) {
			enc->accel_tick += DECEL_RATE;
		}

		// Gradually decrease velocity more aggressively to prevent "jumping"
		if (enc->velocity > 0) {
			enc->velocity -= DECEL_RATE * 2; // Decelerate faster when no input
		} else if (enc->velocity < 0) {
			enc->velocity +=
					DECEL_RATE * 2; // Faster deceleration in the opposite direction
		}

		// Ensure velocity reaches zero eventually
		if (abs(enc->velocity) < DECEL_RATE * 2) {
			enc->velocity = 0;
		}

	} else {
		// Directional consistency: reset acceleration if direction changes
		if (new_direction != enc->direction) {
			enc->accel_tick = 0; // Reset acceleration when changing direction
		}

		// Gradually accelerate in the same direction
		if (new_direction == enc->direction) {
			if (enc->accel_tick < ACCEL_MAX && new_direction > 0) {
				enc->accel_tick += (ACCEL_MAX - enc->accel_tick) / 2; // Smooth accel
			} else if (enc->accel_tick > -ACCEL_MAX && new_direction < 0) {
				enc->accel_tick += (-ACCEL_MAX - enc->accel_tick) / 2; // Smooth decel
			}
		}

		// Calculate movement, adding acceleration
		int16_t movement = new_direction;
		if (enc->accel_tick > ACCEL_THRESHOLD ||
				enc->accel_tick < -ACCEL_THRESHOLD) {
			movement += (enc->accel_tick - ACCEL_THRESHOLD);
		}

		// Update velocity, but cap the maximum velocity
		enc->velocity += movement;

		// Cap the velocity to prevent it from jumping to extreme values
		if (enc->velocity > ACCEL_MAX) {
			enc->velocity = ACCEL_MAX;
		} else if (enc->velocity < -ACCEL_MAX) {
			enc->velocity = -ACCEL_MAX;
		}

		// Save the current direction
		enc->direction = new_direction;
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
