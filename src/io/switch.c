/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "io/switch.h"
#include "system/types.h"
#include "event/io.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

// Get the state of a single switch
enum switch_state switch_x16_state(struct switch_x16_ctx* ctx, u8 index) {
	return (ctx->current & (1u << index));
}

enum switch_state switch_x8_state(struct switch_x8_ctx* ctx, u8 index) {
	return (ctx->current & (1u << index));
}

// Get the state of all switches as a bitfield
u16 switch_x16_states(struct switch_x16_ctx* ctx) {
	return (ctx->current);
}

u8 switch_x8_states(struct switch_x8_ctx* ctx) {
	return (ctx->current);
}

inline bool switchx16_was_pressed(struct switch_x16_ctx* ctx, u8 index) {
	return (ctx->raw & ctx->current) & (1u << index);
}

inline bool switchx16_was_released(struct switch_x16_ctx* ctx, u8 index) {
	return (ctx->raw & ~ctx->current) & (1u << index);
}

inline bool switchx8_was_pressed(struct switch_x8_ctx* ctx, u8 index) {
	return (ctx->raw & ctx->current) & (1u << index);
}

inline bool switchx8_was_released(struct switch_x8_ctx* ctx, u8 index) {
	return (ctx->raw & ~ctx->current) & (1u << index);
}

// Debounce algorithm for 8 switches (call before checking switch state)
void switch_x8_debounce(struct switch_x8_ctx* ctx) {
	// Store the current state
	ctx->previous = ctx->current;
	ctx->current	= 0xFF;

	// AND the new state with EVERY debounce sample, if there was a glitch
	// then the state of the switch will revert to 0.
	for (int i = 0; i < SWITCH_DEBOUNCE_SAMPLES; ++i) {
		ctx->current &= ctx->buf[i];
	}

	// Set the raw states to XOR of new and old
	ctx->raw = ctx->current ^ ctx->previous;
}

// Debounce algorithm for 16 switches
void switch_x16_debounce(struct switch_x16_ctx* ctx) {
	// Store the current state
	ctx->previous = ctx->current;
	ctx->current	= 0xFFFF;

	// AND the new state with EVERY debounce sample, if there was a glitch
	// then the state of the switch will revert to 0.
	for (int i = 0; i < SWITCH_DEBOUNCE_SAMPLES; ++i) {
		ctx->current &= ctx->buf[i];
	}

	// Set the raw states to XOR of new and old
	ctx->raw = ctx->current ^ ctx->previous;
}

void switch_x8_update(struct switch_x8_ctx* ctx, u8 gpio_state) {
	// Update the gpio states
	ctx->buf[ctx->index] = gpio_state;

	// Increment index and wrap if index == SWITCH_DEBOUNCE_SAMPLES
	ctx->index = (u8)(ctx->index + 1) % SWITCH_DEBOUNCE_SAMPLES;

	switch_x8_debounce(ctx);
}

void switch_x16_update(struct switch_x16_ctx* ctx, u16 gpio_state) {
	// Update the gpio states
	ctx->buf[ctx->index] = gpio_state;

	// Increment index and wrap if index == SWITCH_DEBOUNCE_SAMPLES
	ctx->index = (u8)(ctx->index + 1) % SWITCH_DEBOUNCE_SAMPLES;

	switch_x16_debounce(ctx);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

#if 0

/**
 * @brief Check if a side switch was pressed.
 * This can check all side switches, or a specific set by using a mask.
 * @param Mask - Can be used to mask which side switch to check
 * @return u8 A bitfield of the current side switch states. Note - there
 * are only 6 side switches.
 */
u8 SideSwitchWasPressed(u8 Mask) {
  return (mSideSwitchStates.raw_state & mSideSwitchStates.debounces_states) &
		 Mask;
}

/**
 * @brief Check if a side switch was released.
 * This can check all side switches, or a specific set by using a mask.
 * @param Mask - Can be used to mask which side switch to check
 * @return u8 A bitfield of the current side switch states. Note - there
 * are only 6 side switches.
 */
u8 SideSwitchWasReleased(u8 Mask) {
  return (mSideSwitchStates.raw_state & (~mSideSwitchStates.debounces_states)) &
		 Mask;
}

#endif
