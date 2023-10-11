/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/system.h"
#include "application/io/encoder.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// #define VEL_INC (INT16_MAX / 22)
#define VEL_INC (100)
#define VEL_MAX (2000)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void encoder_update(encoder_ctx_t* enc, int direction) {
  // If not rotating skip (nothing to do)
  if (direction == 0) {
    if (enc->velocity > VEL_INC) {
      enc->velocity -= 5;
    } else if (enc->velocity < -VEL_INC) {
      enc->velocity += 5;
    }
    return;
  }
  // Process the rotation of the encoder
  i32 velocity = VEL_INC * direction;

  if (direction != enc->direction) {
    enc->velocity  = 0;
    enc->direction = direction;
  } else {
    velocity += enc->velocity;
  }

  enc->velocity += velocity;

  if (abs(enc->velocity) > VEL_MAX) {
    enc->velocity = VEL_MAX * enc->direction;
  }

  enc->prev_val = enc->curr_val;
  i32 newval    = (i32)enc->curr_val + enc->velocity;
  if (newval >= ENC_MAX) {
    enc->curr_val = ENC_MAX;
  } else if (newval <= ENC_MIN) {
    enc->curr_val = ENC_MIN;
  } else {
    enc->curr_val += enc->velocity;
  }

  // Set the changed flag true if there was a change
  // NEVER set it false - the user must clear this value if they wish
  if (enc->curr_val != enc->prev_val) {
    enc->changed = true;
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
