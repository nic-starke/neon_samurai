#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define ENC_MAX		(UINT8_MAX)		// Maximum encoder value
#define ENC_MIN		(0)						// Minimum encoder value
#define ENC_MID		(ENC_MAX / 2) // Mid position encoder value
#define ENC_RANGE (u8)(ENC_MAX - ENC_MIN)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct encoder_movement {
	i16 velocity;					// Current rotational velocity
	u8	accel_mode;				// Acceleration mode (Currently unused, placeholder)
	i8	direction;				// Current direction (-1, 0, 1)
	u32 last_update_time; // Last time the encoder was updated
	u16 accel_factor;			// Acceleration factor (1-7)
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Initialises a single encoder context.
 *
 * @param enc Pointer to encoder device.
 * @return 0 on success, !0 on failure.
 */
int encoder_movement_init(struct encoder_movement* enc);

/**
 * @brief Perform an update of an encoder.
 * This function takes as input the hardware state of channel A and B
 * of the quadrature encoder, it then processes any change in encoder state
 * and generates an encoder rotation event if required.
 *
 * This uses a time-based acceleration algorithm to calculate the velocity
 * based on how quickly the encoder is turned.
 *
 * @param enc Pointer to encoder device.
 * @param direction Current direction of encoder (-1, 0, +1)
 * @return 1 if display needs to be updated
 */
bool encoder_movement_update(struct encoder_movement* enc, int direction);
