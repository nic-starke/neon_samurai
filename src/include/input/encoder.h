#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "sys/types.h"
#include "virtmap/virtmap.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define ENC_MAX					 (UINT16_MAX)	 // Maximum encoder value (16-bits)
#define ENC_MIN					 (0)					 // Minimum encoder value (16-bits)
#define ENC_MID					 (ENC_MAX / 2) // Mid position encoder value (16-bits)
#define ENC_RANGE				 (u16)(ENC_MAX - ENC_MIN)
#define ENC_MAX_VELOCITY (1500) // Maximum encoder velocity (absolute)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef struct {
	i32 velocity;		 // Current rotational velocity
	u16 curr_val;		 // Current value
	u16 prev_val;		 // Previous value
	u8	accel_mode;	 // Acceleration mode
	u8	accel_const; // Acceleration constant
	i8	direction;	 // Current direction
} encoder_s;

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
 * @return True if encoder position changed.
 */
bool encoder_update(encoder_s* enc, int direction);

/**
 * @brief Clamps the current value of the encoder between a min
 * and max value range.
 *
 * Min and max must be u16 to conform with the types used by
 * the encoder system.
 *
 * @param enc Pointer to encoder device.
 * @param min Minimum value of range.
 * @param max Maximum value of range.
 */
void encoder_clamp(encoder_s* enc, u16 min, u16 max);
