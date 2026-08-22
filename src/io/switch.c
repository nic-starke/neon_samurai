/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "io/switch.h"
#include "system/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

// Get the state of a single switch
enum switch_state switch_x16_state(struct switch_x16_ctx* ctx, u8 index) {
	return (ctx->current & (1u << index)) ? SWITCH_PRESSED : SWITCH_IDLE;
}

enum switch_state switch_x8_state(struct switch_x8_ctx* ctx, u8 index) {
	return (ctx->current & (1u << index)) ? SWITCH_PRESSED : SWITCH_IDLE;
}

// Get the state of all switches as a bitfield
u16 switch_x16_states(struct switch_x16_ctx* ctx) {
	return (ctx->current);
}

u8 switch_x8_states(struct switch_x8_ctx* ctx) {
	return (ctx->current);
}

inline bool switchx16_was_pressed(struct switch_x16_ctx* ctx, u8 index) {
	return (ctx->changed & ctx->current) & (1u << index);
}

inline bool switchx16_was_released(struct switch_x16_ctx* ctx, u8 index) {
	return (ctx->changed & ~ctx->current) & (1u << index);
}

inline bool switchx8_was_pressed(struct switch_x8_ctx* ctx, u8 index) {
	return (ctx->changed & ctx->current) & (1u << index);
}

inline bool switchx8_was_released(struct switch_x8_ctx* ctx, u8 index) {
	return (ctx->changed & ~ctx->current) & (1u << index);
}

/*
	A switch only changes state once every buffered sample agrees on the new
	level - all-ones to set, all-zeroes to clear - and holds its previous level
	while the samples disagree. Filtering both edges the same way means a lone
	noise sample cannot fake a release while a switch is held down.
*/
void switch_x8_debounce(struct switch_x8_ctx* ctx) {
	u8 all = 0xFF;
	u8 any = 0x00;

	for (int i = 0; i < SWITCH_DEBOUNCE_SAMPLES; ++i) {
		all &= ctx->buf[i];
		any |= ctx->buf[i];
	}

	const u8 held = ctx->current;

	ctx->previous = held;
	ctx->current	= (u8)((held & any) | all);
	ctx->changed	= (u8)(ctx->current ^ ctx->previous);
}

void switch_x16_debounce(struct switch_x16_ctx* ctx) {
	u16 all = 0xFFFF;
	u16 any = 0x0000;

	for (int i = 0; i < SWITCH_DEBOUNCE_SAMPLES; ++i) {
		all &= ctx->buf[i];
		any |= ctx->buf[i];
	}

	const u16 held = ctx->current;

	ctx->previous = held;
	ctx->current	= (u16)((held & any) | all);
	ctx->changed	= (u16)(ctx->current ^ ctx->previous);
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
