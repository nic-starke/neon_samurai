/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/system.h"
#include "drivers/encoder.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
  QUAD_00,
  QUAD_01,
  QUAD_10,
  QUAD_11,
  QUAD_100,
  QUAD_101,

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
    {QUAD_11, QUAD_10, QUAD_01, QUAD_00},            // 00
    {QUAD_11 | DIR_CCW, QUAD_00, QUAD_01, QUAD_00},  // 01
    {QUAD_11 | DIR_CW, QUAD_10, QUAD_00, QUAD_00},   // 10
    {QUAD_11, QUAD_101, QUAD_100, QUAD_00},          // 11
    {QUAD_11, QUAD_11, QUAD_100, QUAD_00 | DIR_CW},  // 100
    {QUAD_11, QUAD_101, QUAD_11, QUAD_00 | DIR_CCW}, // 101
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void encoder_update(encoder_hwctx_t* enc) {
  unsigned int val = (enc->io_b << 1) | enc->io_a;
  enc->rot         = quad_states[enc->rot & 0x0F][val];
  enc->dir         = enc->rot & 0x30;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
