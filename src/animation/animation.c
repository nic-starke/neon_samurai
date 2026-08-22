/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string.h>
#include <avr/pgmspace.h>

#include "system/types.h"
#include "system/error.h"
#include "system/time.h"
#include "system/hardware.h"
#include "led/led.h" // Added to access RGB bit definitions

#include "event/event.h"
#include "event/animation.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define ANIMATION_EVENT_QUEUE_SIZE 4

// Frames across the bank-change fade. Finer than ANIMATION_MAX_FRAMES so the
// ramp is smooth rather than stepped, and a multiple of three so the
// off/white/off/colour segments divide evenly.
#define ANIM_BANK_CHANGE_FRAMES		 33

_Static_assert((ANIM_BANK_CHANGE_FRAMES % 3) == 0,
							 "the off/white/off/colour segments must divide evenly");

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct animation_state {
	enum animation_type type;
	u32									start_time;
	u32									duration;
	u8									current_frame;
	u8									total_frames;
	bool								active;
	u8									target_encoder; // Target encoder for this animation
	bool can_be_overridden;		// Whether this animation can be overridden by a new
														// one
	bool overrides_same_type; // Whether this animation cancels others of the same
														// type

	union {
		struct {
			u8 prev_bank;
			u8 new_bank;
		} bank_change;
	} data;
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int	animation_event_handler(void* event);
static int	draw_bank_change_animation(u8											 encoder_idx,
																			 struct animation_state* anim);
static u8		mix_channel(u8 from, u8 to, u8 weight);
static int	find_free_animation_slot(void);
static bool is_encoder_animated(u8 encoder_idx);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */

// Animation event channel
static struct animation_event animation_event_queue[ANIMATION_EVENT_QUEUE_SIZE];

EVT_HANDLER(1, animation_handler, animation_event_handler);

struct event_channel animation_event_ch = {
		.queue			= (u8*)animation_event_queue,
		.queue_size = ANIMATION_EVENT_QUEUE_SIZE,
		.data_size	= sizeof(struct animation_event),
		.handlers		= &animation_handler,
		.onehandler = true,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Array of animation states for concurrent animations
static struct animation_state anim_states[ANIMATION_MAX_CONCURRENT] = {0};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int animation_init(void) {
	// Register the animation event channel
	int ret =
			event_channel_register(EVENT_CHANNEL_ANIMATION, &animation_event_ch);
	RETURN_ON_ERR(ret);

	// Initialize all animation states
	for (u8 i = 0; i < ANIMATION_MAX_CONCURRENT; i++) {
		anim_states[i].type		= ANIM_TYPE_NONE;
		anim_states[i].active = false;
	}

	return SUCCESS;
}

bool animation_is_active(void) {
	// Check if any animation is active
	for (u8 i = 0; i < ANIMATION_MAX_CONCURRENT; i++) {
		if (anim_states[i].active) {
			return true;
		}
	}
	return false;
}

bool animation_is_active_idx(u8 anim_idx) {
	if (anim_idx >= ANIMATION_MAX_CONCURRENT) {
		return false;
	}
	return anim_states[anim_idx].active;
}

u8 animation_get_active_count(void) {
	u8 count = 0;
	for (u8 i = 0; i < ANIMATION_MAX_CONCURRENT; i++) {
		if (anim_states[i].active) {
			count++;
		}
	}
	return count;
}

int animation_update(void) {
	bool any_updated	= false;
	u32	 current_time = systime_ms();

	// Update all active animations
	for (u8 i = 0; i < ANIMATION_MAX_CONCURRENT; i++) {
		struct animation_state* anim = &anim_states[i];

		if (!anim->active) {
			continue;
		}

		u32 elapsed = current_time - anim->start_time;

		// Check if the animation has completed
		if (elapsed >= anim->duration) {
			anim->active = false;
			any_updated	 = true;

			// Force redraw for the target encoder
			struct encoder* enc = &gENCODERS[gRT.curr_bank][anim->target_encoder];
			enc->update_display = 1;
			continue;
		}

		// Calculate current frame based on elapsed time
		u32 frame_duration = anim->duration / anim->total_frames;
		u8	new_frame			 = elapsed / frame_duration;

		if (new_frame != anim->current_frame) {
			anim->current_frame = new_frame;
			any_updated					= true;
		}
	}

	// If any animation was updated, request redraw of all encoders
	if (any_updated) {
		for (u8 i = 0; i < NUM_ENCODERS; i++) {
			if (is_encoder_animated(i)) {
				struct encoder* enc = &gENCODERS[gRT.curr_bank][i];
				enc->update_display = 1;
			}
		}
	}

	return SUCCESS;
}

int animation_start_bank_change(u8 prev_bank, u8 new_bank) {
	// The first row indicates the active bank, right to left: bank 0 lights
	// encoder 3, bank 3 lights encoder 0.
	u8 encoder_to_flash = (new_bank < 4) ? (u8)(3 - new_bank) : 3;

	// First, cancel any existing bank change animations
	for (u8 i = 0; i < ANIMATION_MAX_CONCURRENT; i++) {
		if (anim_states[i].active && anim_states[i].type == ANIM_TYPE_BANK_CHANGE) {
			// Cancel this animation
			anim_states[i].active = false;

			// Force a redraw of the encoder that was being animated
			struct encoder* enc =
					&gENCODERS[gRT.curr_bank][anim_states[i].target_encoder];
			enc->update_display = 1;
		}
	}

	// Find a free animation slot
	int slot = find_free_animation_slot();
	if (slot < 0) {
		return ERR_NO_MEM; // No free animation slots available
	}

	// Set up the animation in the free slot
	struct animation_state* anim		 = &anim_states[slot];
	anim->type											 = ANIM_TYPE_BANK_CHANGE;
	anim->start_time								 = systime_ms();
	anim->duration									 = ANIMATION_DEFAULT_DURATION_MS;
	anim->current_frame							 = 0;
	anim->total_frames							 = ANIM_BANK_CHANGE_FRAMES;
	anim->target_encoder						 = encoder_to_flash;
	anim->data.bank_change.prev_bank = prev_bank;
	anim->data.bank_change.new_bank	 = new_bank;
	anim->active										 = true;
	anim->can_be_overridden = true; // Bank switch animations can be overridden
	anim->overrides_same_type =
			true; // Bank switch animations override other bank switch animations

	return slot;
}

int animation_start_custom(enum animation_type type, u8 encoder_idx,
													 u32 duration_ms, u8 frames) {
	if (encoder_idx >= NUM_ENCODERS) {
		return ERR_BAD_PARAM;
	}

	// Check if there are animations of the same type that need to be overridden
	for (u8 i = 0; i < ANIMATION_MAX_CONCURRENT; i++) {
		if (anim_states[i].active && anim_states[i].type == type) {
			// If this is the same type and it can be overridden, cancel it
			if (anim_states[i].can_be_overridden) {
				anim_states[i].active = false;

				// Force a redraw of the encoder that was being animated
				struct encoder* enc =
						&gENCODERS[gRT.curr_bank][anim_states[i].target_encoder];
				enc->update_display = 1;
			}
			// If this animation doesn't allow others of same type, don't create a new
			// one
			else if (anim_states[i].overrides_same_type) {
				return ERR_ANIMATION_BUSY; // An animation that can't be overridden is
																	 // already running
			}
		}
	}

	// Find a free animation slot
	int slot = find_free_animation_slot();
	if (slot < 0) {
		return ERR_NO_MEM; // No free animation slots available
	}

	// Set up the animation in the free slot
	struct animation_state* anim = &anim_states[slot];
	anim->type									 = type;
	anim->start_time						 = systime_ms();
	anim->duration							 = duration_ms;
	anim->current_frame					 = 0;
	anim->total_frames					 = frames;
	anim->target_encoder				 = encoder_idx;
	anim->active								 = true;
	anim->can_be_overridden = true; // By default, animations can be overridden
	anim->overrides_same_type =
			false; // By default, don't override other animations

	return slot;
}

int animation_draw_encoder(u8 encoder_idx) {
	if (encoder_idx >= NUM_ENCODERS) {
		return ERR_BAD_PARAM;
	}

	// Check if this encoder is part of any active animation
	for (u8 i = 0; i < ANIMATION_MAX_CONCURRENT; i++) {
		struct animation_state* anim = &anim_states[i];

		if (!anim->active || anim->target_encoder != encoder_idx) {
			continue;
		}

		// Found an animation for this encoder
		switch (anim->type) {
			case ANIM_TYPE_BANK_CHANGE:
				return draw_bank_change_animation(encoder_idx, anim);

			default:
				// Unknown animation type, skip
				break;
		}
	}

	// No active animations for this encoder
	return ERR_NO_ANIMATION;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int animation_event_handler(void* event) {
	struct animation_event* evt = (struct animation_event*)event;

	switch (evt->type) {
		case ANIM_EVT_BANK_CHANGE:
			return animation_start_bank_change(evt->data.bank_change.prev_bank,
																				 evt->data.bank_change.new_bank);

		default: return ERR_NOT_IMPLEMENTED;
	}
}

static u8 mix_channel(u8 from, u8 to, u8 weight) {
	return (u8)((i16)from + (((i16)to - (i16)from) * weight) / 255);
}

static int draw_bank_change_animation(u8											encoder_idx,
																			struct animation_state* anim) {
	u8 frame = anim->current_frame;
	if (frame > anim->total_frames) {
		frame = anim->total_frames;
	}

	const struct encoder* enc = &gENCODERS[gRT.curr_bank][encoder_idx];
	const struct virtmap* vm	= &enc->vmaps[enc->vmap_active];

	const u8 segment = (u8)(anim->total_frames / 3u);
	u8			 r;
	u8			 g;
	u8			 b;

	if (frame < segment) {
		// Off to white.
		r = g = b = (u8)(((u16)frame * MAX_BRIGHTNESS) / segment);
	} else if (frame < (u8)(segment * 2u)) {
		// White back to off.
		const u8 w = (u8)(((u16)(frame - segment) * MAX_BRIGHTNESS) / segment);
		r = g = b = (u8)(MAX_BRIGHTNESS - w);
	} else {
		// Off up to the encoder's own colour.
		const u8 span = (u8)(anim->total_frames - (segment * 2u));
		const u8 w = (u8)(((u16)(frame - (segment * 2u)) * MAX_BRIGHTNESS) / span);

		r = mix_channel(0, vm->rgb.red, w);
		g = mix_channel(0, vm->rgb.green, w);
		b = mix_channel(0, vm->rgb.blue, w);
	}

	const u16 rgb_mask =
			(u16) ~((1 << RGB_RED_BIT) | (1 << RGB_GREEN_BIT) | (1 << RGB_BLUE_BIT));

	for (u8 p = 0; p < NUM_BCM_PLANES; p++) {
		u16 state = (u16)(~gFRAME_BUFFER[p][encoder_idx]) & rgb_mask;

		if (r & 1u) {
			state |= (1 << RGB_RED_BIT);
		}
		if (g & 1u) {
			state |= (1 << RGB_GREEN_BIT);
		}
		if (b & 1u) {
			state |= (1 << RGB_BLUE_BIT);
		}

		gFRAME_BUFFER[p][encoder_idx] = (u16)~state;

		r >>= 1;
		g >>= 1;
		b >>= 1;
	}

	return SUCCESS;
}

static int find_free_animation_slot(void) {
	for (u8 i = 0; i < ANIMATION_MAX_CONCURRENT; i++) {
		if (!anim_states[i].active) {
			return i;
		}
	}
	return ERR_NO_MEM; // No free slots
}

static bool is_encoder_animated(u8 encoder_idx) {
	for (u8 i = 0; i < ANIMATION_MAX_CONCURRENT; i++) {
		if (anim_states[i].active && anim_states[i].target_encoder == encoder_idx) {
			return true;
		}
	}
	return false;
}
