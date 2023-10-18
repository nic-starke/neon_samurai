/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "core/core_types.h"
#include "core/core_switch.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

// Get the state of a single switch
switch_state_e switch_x16_state(switch_x16_ctx_s* ctx, u16 index) {
  return (ctx->current & (1u << index));
}

switch_state_e switch_x8_state(switch_x8_ctx_s* ctx, u8 index) {
  return (ctx->current & (1u << index));
}

// Get the state of all switches as a bitfield
u16 switch_x16_states(switch_x16_ctx_s* ctx) {
  return (ctx->current);
}

u8 switch_x8_states(switch_x8_ctx_s* ctx) {
  return (ctx->current);
}

// Debounce algorithm for 8 switches (call before checking switch state)
void switch_x8_debounce(switch_x8_ctx_s* ctx) {
  // Store the current state
  ctx->previous = ctx->current;
  u8 new_state  = 0xFF;

  // AND the new state with EVERY debounce sample, if there was a glitch
  // then the state of the switch will revert to 0.
  for (int i = 0; i < SWITCH_DEBOUNCE_SAMPLES; ++i) {
    new_state &= ctx->buf[i];
  }

  // Update the current raw
  ctx->current = new_state;

  // Set the raw states to XOR of new and old
  ctx->raw = new_state ^ ctx->previous;
}

// Debounce algorithm for 16 switches
void switch_x16_debounce(switch_x16_ctx_s* ctx) {
  // Store the current state
  ctx->previous = ctx->current;
  u16 new_state = 0xFFFF;

  // AND the new state with EVERY debounce sample, if there was a glitch
  // then the state of the switch will revert to 0.
  for (int i = 0; i < SWITCH_DEBOUNCE_SAMPLES; ++i) {
    new_state &= ctx->buf[i];
  }

  // Update the current raw
  ctx->current = new_state;

  // Set the raw states to XOR of new and old
  ctx->raw = new_state ^ ctx->previous;
}

void switch_x8_update(switch_x8_ctx_s* ctx, u8 gpio_state) {
  // Update the gpio states
  ctx->buf[ctx->index] = gpio_state;

  // Increment index and wrap if index == SWITCH_DEBOUNCE_SAMPLES
  ctx->index = (ctx->index + 1) % SWITCH_DEBOUNCE_SAMPLES;
}

void switch_x16_update(switch_x16_ctx_s* ctx, u16 gpio_state) {
  // Update the gpio states
  ctx->buf[ctx->index] = gpio_state;

  // Increment index and wrap if index == SWITCH_DEBOUNCE_SAMPLES
  ctx->index = (ctx->index + 1) % SWITCH_DEBOUNCE_SAMPLES;
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
