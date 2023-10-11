/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"
#include "drivers/quad_encoder.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
  QUAD_START,
  QUAD_CCW,
  QUAD_CW,
  QUAD_MIDDLE,
  QUAD_MID_CW,
  QUAD_MID_CCW,

  QUAD_NB,
} quad_state_e;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Rotory decoder based on
 * https://github.com/buxtronix/arduino/tree/master/libraries/Rotary Copyright
 * 2011 Ben Buxton. Licenced under the GNU GPL Version 3. Contact: bb@cactii.net
 */
static const quad_state_e quad_states[QUAD_NB][4] = {
    // Current Quadrature GrayCode
    {QUAD_MIDDLE, QUAD_CW, QUAD_CCW, QUAD_START},
    {QUAD_MIDDLE | DIR_CCW, QUAD_START, QUAD_CCW, QUAD_START},
    {QUAD_MIDDLE | DIR_CW, QUAD_CW, QUAD_START, QUAD_START},
    {QUAD_MIDDLE, QUAD_MID_CCW, QUAD_MID_CW, QUAD_START},
    {QUAD_MIDDLE, QUAD_MIDDLE, QUAD_MID_CW, QUAD_START | DIR_CW},
    {QUAD_MIDDLE, QUAD_MID_CCW, QUAD_MIDDLE, QUAD_START | DIR_CCW},
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void quad_encoder_update(quad_encoder_ctx_t* ctx, unsigned int ch_a,
                         unsigned int ch_b) {
  unsigned int val = (ch_b << 1) | ch_a;
  ctx->rot_state   = quad_states[ctx->rot_state & 0x0F][val];
  ctx->dir         = ctx->rot_state & 0x30;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
