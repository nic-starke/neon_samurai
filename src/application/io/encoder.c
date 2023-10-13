/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/system.h"
#include "system/utility.h"
#include "application/io/encoder.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Velocity increment
#define VEL_INC 16

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Acceleration constants
static i16 accel_inc[] = {VEL_INC * 1, VEL_INC * 5, VEL_INC * 30, VEL_INC * 60,
                          VEL_INC * 120};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void encoder_update(encoder_ctx_t* enc, int direction) {
  assert(enc);

  i32 newval = 0;

  if (enc->accel_mode == 0) {
    if (direction == 0) {
      return;
    }

    enc->prev_val = enc->curr_val;
    enc->velocity = 1500 * direction;
  } else {
    // If the encoder stopped moving then decelerate
    if (direction == 0) {
      if (enc->velocity > VEL_INC * 2) {
        enc->velocity -= accel_inc[enc->accel_const];
      } else if (enc->velocity < -(VEL_INC * 2)) {
        enc->velocity += accel_inc[enc->accel_const];
      }
      return;
    }

    // Accelerate if the direction is the same, otherwise reset the acceleration
    if (enc->direction == direction) {
      enc->accel_const = (enc->accel_const + 1) % COUNTOF(accel_inc);
    } else {
      enc->accel_const = 0;
    }

    // Update the direction
    enc->direction = direction;

    // Update the velocity
    enc->velocity += accel_inc[enc->accel_const] * enc->accel_const * direction;
    enc->prev_val = enc->curr_val;
  }
  newval =
      enc->curr_val + CLAMP(enc->velocity, -ENC_MAX_VELOCITY, ENC_MAX_VELOCITY);
  enc->curr_val = CLAMP(newval, ENC_MIN, ENC_MAX);

  // Set the changed flag true if there was a change
  // NEVER set it false - the user must clear this value if they wish
  if (enc->curr_val != enc->prev_val) {
    enc->changed = true;
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
