/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/system.h"
#include "application/io/encoder.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

i16 clamp(i16 val, i16 min, i16 max);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static uint8_t accel_inc[4] = {10, 50, 100, 300};
// static uint16_t accel_inc[] = {1, 2, 5, 10, 20, 50, 100, 200, 400, 800};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void encoder_update(encoder_ctx_t* enc, int direction) {
  assert(enc);

  switch (enc->direction) {
    case 1:
    case -1: {
      if (enc->direction == direction) {
        enc->accel_const = (enc->accel_const + 1) % countof(accel_inc);
      }
      break;
    }

    case 0: {
      if (enc->accel_const > 1) {
        enc->accel_const -= 1;
      } else if (enc->accel_const == 1) {
        enc->velocity = 0;
      }
      break;
    }

    default: break;
  }

  enc->direction = direction;
  enc->prev_val = enc->curr_val;
  enc->velocity += accel_inc[enc->accel_const] * enc->direction;
  enc->velocity = clamp(enc->velocity, -ENC_MAX_VELOCITY, ENC_MAX_VELOCITY);
  enc->curr_val += enc->velocity;

  // Set the changed flag true if there was a change
  // NEVER set it false - the user must clear this value if they wish
  if (enc->curr_val != enc->prev_val) {
    enc->changed = true;
  }
}

i16 clamp(i16 val, i16 min, i16 max) {
  if (val < min) {
    return min;
  } else if (val > max) {
    return max;
  } else {
    return val;
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
