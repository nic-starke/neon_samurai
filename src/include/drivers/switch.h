/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*
  There are two sets of structs and functions for switches, switch_x8_xxx and
  switch_x16_xxx. x8 is for a set of 8 switches, x16 is for a set of 16
  switches. If you need to handle 32 switches then use two sets of x16. If you
  need to handle < 8 switches then just use the x8. Each unique set of switches
  needs its own unique context struct. For general use:

      1. Poll the state of your switches and then add them to a bitfield. Then
      call the switch_xN_update() function and pass in the bitfield.

      2. Call the debounce function once before you attempt to read the state
      of the switches.

      3. Call the switch_xN_state functions to get the state of the switch(es).


  Note - The debounce should not be called every time you poll, that is a waste
  of cpu time.
*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/system.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define SWITCH_DEBOUNCE_SAMPLES (10)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
  SWITCH_RELEASED,
  SWITCH_PRESSED,
} switch_state_e;

typedef struct {
  uint8_t buf[SWITCH_DEBOUNCE_SAMPLES]; // debounce buffer (private)
  uint8_t index;                        // current buffer index
  uint8_t deb;                          // raw states bitfield (private)
  uint8_t raw;                          // switch states bitfield (private)
} switch_x8_ctx_t;

typedef struct {
  uint16_t buf[SWITCH_DEBOUNCE_SAMPLES]; // debounce buffer (private)
  uint16_t index;                        // current buffer index
  uint16_t deb;                          // raw states bitfield (private)
  uint16_t raw;                          // switch states bitfield (private)
} switch_x16_ctx_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Get the state of a single switch
switch_state_e switch_x16_state(switch_x16_ctx_t* ctx, uint16_t index);
switch_state_e switch_x8_state(switch_x8_ctx_t* ctx, uint8_t index);

// Get the state of all switches as a bitfield
uint16_t switch_x16_states(switch_x16_ctx_t* ctx);
uint8_t  switch_x8_states(switch_x8_ctx_t* ctx);

/**
 * @brief Run the debounce algorithm for all switches.
 *
 * @param ctx Switch context.
 */
void switch_x8_debounce(switch_x8_ctx_t* ctx);
void switch_x16_debounce(switch_x16_ctx_t* ctx);

/**
 * @brief Switch update functions are to be called after you have polled
 * the switch GPIO and retrieved their raw states. As an example:
 *
 * 1. Create a bitfield to store the switch states, set to 0
 * uint8_t state_bitfield = 0x00;
 *
 * 2. Retrieve the state of each switch, set the corresponding bit in the
 * bitfield for (int i = 0; i < 8; ++i) { state_bitfield |=
 * (get_state_of_gpio(i) << i);
 * }
 *
 * 3. Call switch update.
 * switch_x8_update(ctx, state_bitfield);
 *
 * @param ctx Switch context.
 * @param states Bitfield of switch states.
 */
void switch_x16_update(switch_x16_ctx_t* ctx, uint16_t states);
void switch_x8_update(switch_x8_ctx_t* ctx, uint8_t states);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
