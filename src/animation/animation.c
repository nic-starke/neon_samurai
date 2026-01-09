/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string.h>
#include <avr/pgmspace.h>

#include "event/event.h"
#include "event/general.h"
#include "led/animation.h"
#include "led/led.h"
#include "lfo/lfo.h"
#include "system/error.h"
#include "system/hardware.h"
#include "system/time.h"
#include "system/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define ANIM_DEFAULT_DURATION_MS 250
#define ANIM_EVENT_QUEUE_SIZE		 5
#define ANIM_MAX_CONCURRENT			 (NUM_ENCODERS)
#define ANIM_MAX_FRAMES					 8

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum animation_type {
	ANIM_TYPE_NONE,
	ANIM_TYPE_BANK_CHANGE,

	ANIM_TYPE_MIDI_LEARN_ACTIVE,
	ANIM_TYPE_MIDI_LEARN_SUCCESS,
	ANIM_TYPE_MIDI_LEARN_FAIL,

	ANIM_TYPE_LFO_TYPE,

	ANIM_TYPE_NB,
};

enum override_type {
	// This animation will never attempt to override another animation of the same
	// type.
	OVERRIDE_NEVER,

	// This animation can override an animation of the same type on the target
	// encoder.
	OVERRIDE_TGT_ENCODER_SAME_TYPE,

	// This animation can override any animation on the target encoder.
	OVERRIDE_TGT_ENCODER_ANY_TYPE,

	// This animation can override an animation of the same type on any encoder
	OVERRIDE_ANY_ENCODER_SAME_TYPE,

	// This animation overrides all animations currently running on all encoders.
	OVERRIDE_NUKE_ALL,

	OVERRIDE_NB,
};

struct animation {
	u32									start_time;
	u32									duration;
	enum animation_type type;
	bool								active;
	u8									current_frame;
	u8									total_frames;
	u8									enc_idx;
	union {
		struct {
			u8 prev_bank;
			u8 new_bank;
		} bank_change;
	} data;
};

// Configuration data for each type of animation
struct animation_cfg {
	enum override_type override;
	void (*render_func)(struct animation* anim);
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int general_events_handler(void* event);

static struct animation* get_free_anim_slot(u8									enc_idx,
																						enum animation_type new_anim_type);

static int start_bank_change(u8 prev_bank, u8 new_bank);

static void render_animations(void);
static void render_bank_change(struct animation* anim);
static void render_lfo(struct animation* anim);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

EVT_HANDLER(1, general_events, general_events_handler);
static struct animation anim_states[ANIM_MAX_CONCURRENT] = {0};

static const struct animation_cfg anim_configs[ANIM_TYPE_NB] = {
		[ANIM_TYPE_NONE] =
				{
						.override = OVERRIDE_NEVER,
						.render_func = NULL,
				},

		[ANIM_TYPE_BANK_CHANGE] =
				{
						.override = OVERRIDE_ANY_ENCODER_SAME_TYPE,
						.render_func = render_bank_change,
				},

		[ANIM_TYPE_MIDI_LEARN_ACTIVE] =
				{
						.override = OVERRIDE_TGT_ENCODER_ANY_TYPE,
				},

		[ANIM_TYPE_MIDI_LEARN_SUCCESS] =
				{
						.override = OVERRIDE_TGT_ENCODER_ANY_TYPE,
				},

		[ANIM_TYPE_MIDI_LEARN_FAIL] =
				{
						.override = OVERRIDE_TGT_ENCODER_ANY_TYPE,
				},

		[ANIM_TYPE_LFO_TYPE] =
				{
						.override = OVERRIDE_TGT_ENCODER_ANY_TYPE,
				},
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int animation_init(void) {
	// Initialize all animation states
	for (u8 i = 0; i < ANIM_MAX_CONCURRENT; i++) {
		anim_states[i].type		= ANIM_TYPE_NONE;
		anim_states[i].active = false;
	}

	int ret = event_channel_subscribe(EVENT_CHANNEL_GEN, &general_events);
	RETURN_ON_ERR(ret);

	return SUCCESS;
}

bool animation_is_active(void) {
	// Check if any animation is active
	for (u8 i = 0; i < ANIM_MAX_CONCURRENT; i++) {
		if (anim_states[i].active) {
			return true;
		}
	}
	return false;
}

int animation_update(void) {
	u32 current_time = systime_ms();

	// Update all active animations
	for (u8 i = 0; i < ANIM_MAX_CONCURRENT; i++) {
		struct animation* anim = &anim_states[i];

		if (!anim->active) {
			continue;
		}

		u32 elapsed = current_time - anim->start_time;

		// Check if the animation has completed
		if (elapsed >= anim->duration) {
			anim->active = false;

			// Force redraw for the target encoder (will render the default state)
			struct encoder* enc = &gENCODERS[gRT.curr_bank][anim->enc_idx];
			enc->update_display = 1;
			continue;
		}

		// Calculate current frame based on elapsed time
		u32 frame_duration = anim->duration / anim->total_frames;
		u8	new_frame			 = (u8)(elapsed / frame_duration);

		if (new_frame != anim->current_frame) {
			anim->current_frame = new_frame;
		}
	}

	render_animations();

	return SUCCESS;
}


int start_lfo_flash(u32 duration_ms) {
	// // Cancel any existing LFO flash animations
	// for (u8 i = 0; i < ANIM_MAX_CONCURRENT; i++) {
	// 	if (anim_states[i].active && anim_states[i].type == ANIM_TYPE_LFO_FLASH) {
	// 		// Cancel this animation
	// 		anim_states[i].active = false;

	// 		// Force a redraw of the encoder that was being animated
	// 		struct encoder* enc = &gENCODERS[gRT.curr_bank][anim_states[i].enc_idx];
	// 		enc->update_display = 1;
	// 	}
	// }

	// // Start animation for each encoder that has an active LFO in the current bank
	// bool any_started = false;
	// for (u8 e = 0; e < NUM_ENCODERS; e++) {
	// 	// Check if this encoder has an active LFO assigned to it in the current
	// 	// bank
	// 	if (lfo_encoder_has_active_lfo_on_current_bank(e)) {
	// 		// Find a free animation slot
	// 		int slot = get_free_anim_slot();
	// 		if (slot < 0) {
	// 			return ERR_NO_MEM; // No free animation slots available
	// 		}

	// 		// Set up the animation in the free slot
	// 		struct animation* anim		= &anim_states[slot];
	// 		anim->type								= ANIM_TYPE_LFO_FLASH;
	// 		anim->start_time					= systime_ms();
	// 		anim->duration						= duration_ms;
	// 		anim->current_frame				= 0;
	// 		anim->total_frames				= 4; // 4 frames is enough for a flash effect
	// 		anim->enc_idx							= e;
	// 		anim->active							= true;
	// 		anim->can_be_overridden		= true;
	// 		anim->overrides_same_type = true;

	// 		any_started = true;
	// 	}
	// }

	// return any_started ? SUCCESS : ERR_NO_ANIMATION;
	return SUCCESS;
}



/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int general_events_handler(void* event) {
	assert(event);
	struct gen_event* evt = (struct gen_event*)event;

	enum animation_type anim_type = ANIM_TYPE_NONE;

	switch (evt->type) {
		case EVT_GEN_BANK_CHANGE: {
			anim_type = ANIM_TYPE_BANK_CHANGE;
			break;
		}

		case EVT_GEN_MIDI_LEARN_START: {
			anim_type = ANIM_TYPE_MIDI_LEARN_ACTIVE;
			break;
		}

		case EVT_GEN_MIDI_LEARN_SUCCESS: {
			anim_type = ANIM_TYPE_MIDI_LEARN_SUCCESS;
			break;
		}

		case EVT_GEN_MIDI_LEARN_CANCEL: {
			anim_type = ANIM_TYPE_MIDI_LEARN_FAIL;
			break;
		}

		default: return SUCCESS;
	}

	switch(anim_type) {
		case ANIM_TYPE_NONE: {
			return SUCCESS;
		}

		case ANIM_TYPE_BANK_CHANGE: {
			return start_bank_change(evt->bank_change.prev_bank, evt->bank_change.new_bank);
		}

		case ANIM_TYPE_MIDI_LEARN_ACTIVE: {
			return ERR_NOT_IMPLEMENTED;
		}

		case ANIM_TYPE_MIDI_LEARN_SUCCESS: {
			return ERR_NOT_IMPLEMENTED;
		}

		case ANIM_TYPE_MIDI_LEARN_FAIL: {
			return ERR_NOT_IMPLEMENTED;
		}

		case ANIM_TYPE_LFO_TYPE: {
			return ERR_NOT_IMPLEMENTED;
		}


		default: return ERR_BAD_PARAM;
	}

	return SUCCESS;
}

static void render_animations(void) {

	for (u8 i = 0; i < ANIM_MAX_CONCURRENT; i++) {
		struct animation* anim = &anim_states[i];
		const struct animation_cfg* anim_cfg = &anim_configs[anim->type];

		if ((anim->active) && (anim_cfg->render_func != NULL)) {
			anim_cfg->render_func(anim);
		}
	}
}

static int start_bank_change(u8 prev_bank, u8 new_bank) {
	if (prev_bank == new_bank) {
		return SUCCESS;
	}

	// Calculate which encoder will flash, based on the new bank value
	u8 enc_idx = 3 - new_bank;

	struct animation* new_anim =
			get_free_anim_slot(enc_idx, ANIM_TYPE_BANK_CHANGE);

	if (new_anim == NULL) {
		return ERR_NO_MEM;
	}

	new_anim->type				 = ANIM_TYPE_BANK_CHANGE;
	new_anim->duration		 = ANIM_DEFAULT_DURATION_MS;
	new_anim->total_frames = ANIM_MAX_FRAMES;

	new_anim->enc_idx										 = enc_idx;
	new_anim->data.bank_change.new_bank	 = new_bank;
	new_anim->data.bank_change.prev_bank = prev_bank;

	new_anim->start_time = systime_ms();
	new_anim->current_frame = 0;
	new_anim->active = true;

	return SUCCESS;
}


static void render_bank_change(struct animation* anim) {
	// Calculate animation step for RGB LEDs (off-on pattern)
	bool rgb_on = (anim->current_frame % 2 == 0);

	// Use the frame buffer directly
	for (u8 f = 0; f < NUM_PWM_FRAMES; f++) {
		u16 current_state = ~gFRAME_BUFFER[f][anim->enc_idx];

		const u16 set_bits_bc = (u16)((1U << RGB_RED_BIT) | (1U << RGB_GREEN_BIT) |
																	(1U << RGB_BLUE_BIT));
		// Apply bitwise NOT to an unsigned int to prevent intermediate negative
		// signed int value
		u16				rgb_mask_bc = (u16)(~((unsigned int)set_bits_bc));

		current_state &= rgb_mask_bc;

		if (rgb_on) {
			if (f < NUM_PWM_FRAMES -
									1) { // All LEDs fully on except for the last frame (off)
				current_state |= (1U << RGB_RED_BIT);
				current_state |= (1U << RGB_GREEN_BIT);
				current_state |= (1U << RGB_BLUE_BIT);
			}
		}
		gFRAME_BUFFER[f][anim->enc_idx] = ~current_state;
	}
}

static void render_lfo(struct animation* anim) {
	// // animated_encoder_idx is the encoder that should be flashing.
	// // We need to find which LFO is assigned to this encoder on the current bank
	// // to get its color.

	// enum lfo_waveform waveform_to_use				= LFO_WAVE_NONE;
	// bool							found_lfo_for_encoder = false;

	// // Access g_lfos and gRT.curr_bank (assuming they are available via included
	// // headers) lfo.h should provide 'extern struct lfo_state g_lfos[MAX_LFOS];'
	// // system/hardware.h should provide 'extern struct mf_rt gRT;'
	// for (u8 lfo_idx = 0; lfo_idx < MAX_LFOS; ++lfo_idx) {
	// 	if (gLFOs[lfo_idx].active &&
	// 			gLFOs[lfo_idx].assigned_encoder == animated_encoder_idx &&
	// 			gLFOs[lfo_idx].assigned_bank == gRT.curr_bank) {
	// 		waveform_to_use				= gLFOs[lfo_idx].waveform;
	// 		found_lfo_for_encoder = true;
	// 		break;
	// 	}
	// }

	// if (!found_lfo_for_encoder || waveform_to_use == LFO_WAVE_NONE) {
	// 	// No LFO on this encoder for the current bank, or LFO is off. Clear RGB.
	// 	for (u8 f = 0; f < NUM_PWM_FRAMES; f++) {
	// 		u16 current_state =
	// 				~gFRAME_BUFFER[f][animated_encoder_idx]; // Get actual state
	// 		const u16 set_bits_lfo1 =
	// 				(u16)((1U << RGB_RED_BIT) | (1U << RGB_GREEN_BIT) |
	// 							(1U << RGB_BLUE_BIT));
	// 		// Apply bitwise NOT to an unsigned int
	// 		u16 rgb_mask_lfo1 = (u16)(~((unsigned int)set_bits_lfo1));
	// 		current_state &= rgb_mask_lfo1; // Clear RGB bits
	// 		gFRAME_BUFFER[f][animated_encoder_idx] =
	// 				~current_state; // Write back inverted
	// 	}
	// 	return SUCCESS;
	// }

	// // Calculate animation step for flashing (on-off pattern)
	// bool lfo_on = (anim->current_frame % 2 == 0);

	// // Set RGB color based on LFO waveform type
	// u8 rgb_r = 0, rgb_g = 0, rgb_b = 0;

	// if (lfo_on) {
	// 	switch (waveform_to_use) {
	// 		case LFO_WAVE_SINE:
	// 			rgb_r = 0x1F; // Red for sine
	// 			break;
	// 		case LFO_WAVE_TRIANGLE:
	// 			rgb_g = 0x1F; // Green for triangle
	// 			break;
	// 		case LFO_WAVE_SQUARE:
	// 			rgb_b = 0x1F; // Blue for square
	// 			break;
	// 		case LFO_WAVE_SAWTOOTH_UP:
	// 			rgb_r = 0x1F;
	// 			rgb_g = 0x1F; // Yellow for saw up
	// 			break;
	// 		case LFO_WAVE_SAWTOOTH_DN:
	// 			rgb_r = 0x1F;
	// 			rgb_b = 0x1F; // Purple for saw down
	// 			break;
	// 		default: // LFO_WAVE_NONE or other unexpected states
	// 			// Keep RGB off if no specific color or LFO is off
	// 			break;
	// 	}
	// }

	// for (u8 f = 0; f < NUM_PWM_FRAMES; f++) {
	// 	// Start with the current frame buffer state to preserve indicator LEDs
	// 	u16 current_state =
	// 			~gFRAME_BUFFER[f][animated_encoder_idx]; // Invert to get actual state

	// 	// Clear the RGB bits first
	// 	const u16 set_bits_lfo2 =
	// 			(u16)((1U << RGB_RED_BIT) | (1U << RGB_GREEN_BIT) |
	// 						(1U << RGB_BLUE_BIT));
	// 	// Apply bitwise NOT to an unsigned int
	// 	u16 rgb_mask_lfo2 = (u16)(~((unsigned int)set_bits_lfo2));
	// 	current_state &= rgb_mask_lfo2;

	// 	// Add the new RGB values based on animation state and PWM frame
	// 	if (rgb_r > f)
	// 		current_state |= (1U << RGB_RED_BIT);
	// 	if (rgb_g > f)
	// 		current_state |= (1U << RGB_GREEN_BIT);
	// 	if (rgb_b > f)
	// 		current_state |= (1U << RGB_BLUE_BIT);

	// 	gFRAME_BUFFER[f][animated_encoder_idx] = ~current_state;
	// }
	// return SUCCESS;
}

static struct animation* get_free_anim_slot(u8									enc_idx,
																						enum animation_type new_anim_type) {
	struct animation* next = &anim_states[0];
	struct animation* free = NULL;

	// We need to search for a free slot, but also we need to handle the override
	// type of the animation

	for (u8 i = 0; i < ANIM_MAX_CONCURRENT; i++, next++) {
		if (free == NULL && !next->active) {
			free = next;
		}

		switch (anim_configs[new_anim_type].override) {
			case OVERRIDE_NEVER: {
				// In this case we cannot override any animation, so we just use the
				// first free slot.
				break;
			}

			case OVERRIDE_TGT_ENCODER_SAME_TYPE: {
				// In this case we can override the target encoder if it has the same
				// animation
				if ((next->enc_idx == enc_idx) && (next->type == new_anim_type)) {
					free = next;
					goto DONE;
				}
				break;
			}

			case OVERRIDE_TGT_ENCODER_ANY_TYPE: {
				if (next->enc_idx == enc_idx) {
					free = next;
					goto DONE;
				}
				break;
			}

			case OVERRIDE_ANY_ENCODER_SAME_TYPE: {
				if (next->type == new_anim_type) {
					free = next;
					goto DONE;
				}
				break;
			}

			case OVERRIDE_NUKE_ALL: {
				// Set all anims to false
				next->active = false;
				continue;
			}

			default: return NULL;
		}
	}

DONE:
	// If the pointer is valid then we can erase the current animation details
	// and return it
	if (free) {
		memset(free, 0, sizeof(struct animation));
		return free;
	}

	return NULL;
}

