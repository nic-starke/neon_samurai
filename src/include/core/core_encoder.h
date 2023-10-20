/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "core/core_types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define ENC_MAX          (UINT16_MAX)
#define ENC_MIN          (0)
#define ENC_MID          (ENC_MAX / 2)
#define ENC_MAX_VELOCITY (2500)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef struct {
  i32 velocity;    // (private) Current rotational velocity
  u16 curr_val;    // (read only) Current value
  u16 prev_val;    // (private) Previous value
  u8  accel_mode;  // (public) Acceleration mode
  u8  accel_const; // (private) Acceleration constant
	i8	direction;	 // (read only) Current direction
} encoder_ctx_s;

typedef enum {
  DIR_ST  = 0x00, // Stationary
  DIR_CW  = 0x10, // Clockwise
  DIR_CCW = 0x20, // Counter-clockwise
} encoder_dir_e;

typedef struct {
  encoder_dir_e dir;       // (public) Current direction
  i16           vel;       // (public) Angular velocity
  u8            rot_state; // (private) State of rotation
} quadrature_ctx_s;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void core_encoder_update(encoder_ctx_s* enc, int direction);

/**
 * @brief To be called when new quadrature signals are available for the given
 * hardware encoder.
 */
void core_quadrature_decode(quadrature_ctx_s* ctx, unsigned int ch_a,
														unsigned int ch_b);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
