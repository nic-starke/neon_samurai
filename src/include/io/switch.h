/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*
	There are two sets of structs and functions for switches, switch_x8_xxx and
	switch_x16_xxx. x8 is for a set of 8 switches, x16 is for a set of 16
	switches. If you need to handle 32 switches then use two sets of x16. If you
	need to handle < 8 switches then just use the x8. Each unique set of
	 switches needs its own unique context struct. For general use:

			1. Poll the state of your switches and then add them to a bitfield.
	 Then call the switch_xN_update() function and pass in the bitfield.

			2. Call the debounce function once before you attempt to read the
	 state of the switches.

			3. Call the switch_xN_state functions to get the state of the
	 switch(es).


	Note - The debounce should not be called every time you poll, that is a
	 waste of cpu time.
*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define SWITCH_DEBOUNCE_SAMPLES (20)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum switch_state {
	SWITCH_IDLE,
	SWITCH_RELEASED,
	SWITCH_PRESSED,
};

struct switch_x8_ctx {
	u8 buf[SWITCH_DEBOUNCE_SAMPLES]; // debounce buffer (private)
	u8 index;												 // current buffer index
	u8 current;											 // states bitfield (private)
	u8 previous;										 // states bitfield (private)
	u8 raw;													 // switch states bitfield (private)
};

struct switch_x16_ctx {
	u16 buf[SWITCH_DEBOUNCE_SAMPLES]; // debounce buffer (private)
	u8	index;												// current buffer index
	u16 current;											// states bitfield (private)
	u16 previous;											// states bitfield (private)
	u16 raw;													// switch states bitfield (private)
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Get the state of a single switch
enum switch_state switch_x16_state(struct switch_x16_ctx* ctx, u8 index);
enum switch_state switch_x8_state(struct switch_x8_ctx* ctx, u8 index);

bool switchx16_was_pressed(struct switch_x16_ctx*, u8 index);
bool switchx16_was_released(struct switch_x16_ctx*, u8 index);
bool switchx8_was_pressed(struct switch_x8_ctx*, u8 index);
bool switchx8_was_released(struct switch_x8_ctx*, u8 index);

// Get the state of all switches as a bitfield
u16 switch_x16_states(struct switch_x16_ctx* ctx);
u8	switch_x8_states(struct switch_x8_ctx* ctx);

/**
 * @brief Run the debounce algorithm for all switches.
 *
 * @param ctx Switch context.
 */
void switch_x8_debounce(struct switch_x8_ctx* ctx);
void switch_x16_debounce(struct switch_x16_ctx* ctx);

/**
 * @brief Switch update functions are to be called after you have polled
 * the switch GPIO and retrieved their raw states. As an example:
 *
 * 1. Create a bitfield to store the switch states, set to 0
 * u8 state_bitfield = 0x00;
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
void switch_x16_update(struct switch_x16_ctx* ctx, u16 states);
void switch_x8_update(struct switch_x8_ctx* ctx, u8 states);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
