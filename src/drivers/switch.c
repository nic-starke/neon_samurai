/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/system.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
  SWITCH_PRESSED,
  SWITCH_RELEASED,
} switch_state_e;

#define SWITCH_DEBOUNCE_SAMPLES (10)

typedef struct {
  uint8_t buf[SWITCH_DEBOUNCE_SAMPLES]; // debounce buffer (private)
  uint8_t deb;                          // raw states bitfield (private)
  uint8_t raw;                          // switch states bitfield (private)
} switch_x8_ctx_t;

/*
This algorithm debounces momentary switches.
The raw raw of the 16 switches is stored in a bitfield.

and performs a bitwise AND operation between the current deb states and
the raw switch states in the buffer. This operation filters out any glitches in
the raw switch states, as a glitch would result in a 0 bit in the deb
raw. Finally, the algorithm computes the changed states by performing a
bitwise deb operation between the new deb states and the previous
deb states. The changed states are stored in two variables.
*/
void switch_x8_debounce(switch_x8_ctx_t* ctx) {
  // Store the current state
  uint8_t old_state = ctx->deb;
  uint8_t new_state = 0xFF;

  // AND the new state with EVERY debounce sample, if there was a glitch
  // then the state of the switch will revert to 0.
  for (int i = 0; i < SWITCH_DEBOUNCE_SAMPLES; ++i) {
    new_state &= ctx->buf[i];
  }

  // Update the current raw
  ctx->deb = new_state;

  // Set the raw states to XOR of new and old
  ctx->raw = new_state ^ old_state;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
