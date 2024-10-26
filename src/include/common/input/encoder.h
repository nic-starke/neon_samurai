#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "sys/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define ENC_MAX					 (UINT8_MAX)	 // Maximum encoder value
#define ENC_MIN					 (0)					 // Minimum encoder value
#define ENC_MID					 (ENC_MAX / 2) // Mid position encoder value
#define ENC_RANGE				 (u8)(ENC_MAX - ENC_MIN)
#define ENC_MAX_VELOCITY (1500) // Maximum encoder velocity (absolute)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef struct {
	i16 velocity;		// Current rotational velocity
	u8	accel_mode; // Acceleration mode
	i8	accel_tick; // Acceleration tick constant
	i8	direction;	// Current direction
} encoder_s;

// typedef struct {
//     int16_t value;
//     int8_t accel;
//     int8_t last_direction;
//     uint8_t consistent_turns;
// } encoder_s;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Initialises a single encoder context.
 *
 * @param enc Pointer to encoder device.
 * @return 0 on success, !0 on failure.
 */
int encoder_init(encoder_s* enc);

/**
 * @brief Perform an update of an encoder.
 * This function takes as input the hardware state of channel A and B
 * of the quadrature encoder, it then processes any change in encoder state
 * and generates an encoder rotation event if required.
 *
 * This function should be called at regular intervals to ensure
 * correct processing of acceleration (the algorithm assumes a fixed time
 * constant).
 *
 * @param enc Pointer to encoder device.
 * @param direction Current direction of encoder (-1, 0, +1)
 */
void encoder_update(encoder_s* enc, int direction);
