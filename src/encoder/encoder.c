/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include <stdio.h>
#include "console/console.h"
#include "io/encoder.h"
#include "event/event.h"
#include "event/io.h"
#include "system/time.h" // Include for systime_ms
#include <stdint.h>
#include <assert.h> // Include for assert

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define THR_VERY_SLOW		 150
#define THR_SLOW				 75
#define THR_MEDIUM			 30
#define THR_FAST				 10

// Corresponding acceleration factors - Higher value = more acceleration
#define FACTOR_BASE			 1
#define FACTOR_SLOW			 2
#define FACTOR_MEDIUM		 3
#define FACTOR_FAST			 5
#define FACTOR_VERY_FAST 7 // Factor when t < THR_FAST

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int encoder_movement_init(struct encoder_movement* enc) {
	assert(enc);
	enc->velocity		= 0;
	enc->direction	= 0;
	enc->accel_mode = 0;

	// Initialize time-based acceleration state
	enc->last_update_time = systime_ms();
	enc->accel_factor			= 1;

	return 0;
}

bool encoder_movement_update(struct encoder_movement* enc, int new_direction) {
	assert(enc);

	u32 current_time = systime_ms();

	// If encoder stopped
	if (new_direction == 0) {
		enc->velocity			= 0;
		enc->direction		= 0;
		enc->accel_factor = 1;
		return false; // No change
	}

	u32 time_delta				= current_time - enc->last_update_time;
	enc->last_update_time = current_time;

	// Determine the base acceleration factor based on time delta
	u16 current_accel_factor = 1; // Default factor

	if (time_delta == 0) {
		// Assign max acceleration if delta is zero (very fast)
		current_accel_factor = FACTOR_VERY_FAST;
	} else if (time_delta < THR_FAST) { // < 10ms
		current_accel_factor = FACTOR_VERY_FAST;
	} else if (time_delta < THR_MEDIUM) { // 10ms to 29ms
		current_accel_factor = FACTOR_FAST;
	} else if (time_delta < THR_SLOW) { // 30ms to 74ms
		current_accel_factor = FACTOR_MEDIUM;
	} else if (time_delta < THR_VERY_SLOW) { // 75ms to 149ms
		current_accel_factor = FACTOR_SLOW;
	} else { // >= 150ms
		current_accel_factor = FACTOR_BASE;
	}

	// Handle direction changes - Optional: Reset to base speed on change?
	if (new_direction != enc->direction) {
		enc->direction = (i8)new_direction;
		// Option: uncomment below to force slow speed on direction change
		// current_accel_factor = FACTOR_BASE;
	}

	// Store the decided factor
	enc->accel_factor = current_accel_factor;

	// Calculate velocity based on direction and the current acceleration factor
	i16 base_velocity = enc->direction; // +1 or -1
	enc->velocity			= base_velocity * enc->accel_factor;

	// Apply velocity bounds
	const i16 ENC_MAX_VELOCITY =
			50; // Example max overall velocity change per update (adjust as needed)
	if (enc->velocity > ENC_MAX_VELOCITY) {
		enc->velocity = ENC_MAX_VELOCITY;
	} else if (enc->velocity < -ENC_MAX_VELOCITY) {
		enc->velocity = -ENC_MAX_VELOCITY;
	}

	// // Debug output
	// static char buffer[40];
	// snprintf(buffer, sizeof(buffer), "v:%d f:%u t:%lu d:%d\r\n", enc->velocity,
	// 				 enc->accel_factor, time_delta, enc->direction);
	// console_puts(buffer);
	return true; // Encoder moved
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
