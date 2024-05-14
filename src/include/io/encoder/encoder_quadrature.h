#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "core/core_types.h"
#include "io/io_device_types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define ENC_MAX					 (UINT16_MAX)	 // Maximum encoder value (16-bits)
#define ENC_MIN					 (0)					 // Minimum encoder value (16-bits)
#define ENC_MID					 (ENC_MAX / 2) // Mid position encoder value (16-bits)
#define ENC_RANGE				 (u16)(ENC_MAX - ENC_MIN)
#define ENC_MAX_VELOCITY (1500) // Maximum encoder velocity (absolute)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
	DIR_ST	= 0x00, // Stationary
	DIR_CW	= 0x10, // Clockwise
	DIR_CCW = 0x20, // Counter-clockwise
} encoder_dir_e;

typedef struct {
	i32 velocity;		 // Current rotational velocity
	u16 curr_val;		 // Current value
	u16 prev_val;		 // Previous value
	u8	accel_mode;	 // Acceleration mode
	u8	accel_const; // Acceleration constant
	i8	direction;	 // Current direction
	u8	detent;			 // Detent - 1 = enabled, 0 = disabled

	struct {
		encoder_dir_e dir; // Current direction
		u8						rot; // Rotational state
	} quad;							 // Quadrature decoding context

} encoder_s;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Initialises a single encoder context.
 *
 * @param dev Pointer to io device structure.
 * @return 0 on success, !0 on failure.
 */
int encoder_init(iodev_s* dev);

/**
 * @brief Perform an update of a quadrature encoder.
 * This function takes as input the hardware state of channel A and B
 * of the quadrature encoder, it then processes any change in encoder state
 * and generates an encoder rotation event if required.
 *
 * This function should be called at regular intervals to ensure
 * correct processing of acceleration (the algorithm assumes a fixed time
 * constant).
 *
 * @param enc Pointer to a single encoder context.
 * @param ch_a Current state of quadrature channel A.
 * @param ch_b Current state of quadrature channel B.
 */
void encoder_update(iodev_s* dev, uint ch_a, uint ch_b);
