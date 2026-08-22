/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/config.h"
#include "system/error.h"
#include "system/print.h"
#include "system/time.h"
#include "io/encoder.h"
#include "virtmap/virtmap.h"
#include "event/event.h"
#include "event/io.h"
#include "event/midi.h"
#include "event/sys.h"
#include "event/animation.h" // Add animation event header
#include "midi/sysex.h"

#include "system/hardware.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void sw_encoder_init(void);
static void sw_encoder_update(void);
static void sw_side_switch_init(void);
static void sw_side_switch_update(void);
static void vmap_update(struct encoder* enc, struct virtmap* map);
static void set_vmap_active(struct encoder* enc, u8 bank, u8 new_active);
static int	midi_in_handler(void* evt);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

EVT_HANDLER(1, evt_midi, midi_in_handler);

struct encoder		 gENCODERS[NUM_ENC_BANKS][NUM_ENCODERS];
struct side_switch gSIDE_SWITCHES[NUM_SIDE_SWITCHES];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void input_init(void) {
	hw_encoder_init();
	hw_switch_init();
	sw_encoder_init();
	sw_side_switch_init();
	event_channel_subscribe(EVENT_CHANNEL_MIDI_IN, &evt_midi);
}

void input_update(void) {
	hw_encoder_scan();
	hw_switch_update();
	sw_encoder_update();
	sw_side_switch_update();
}

// Corner indices in the 4x4 encoder grid (row-major, idx = row * 4 + col).
// NOTE: this mapping is inferred from the row layout documented in
// sw_encoder_init() below and has not been verified against physical
// hardware - confirm on a real device before relying on it.
#define ENC_IDX_TOP_LEFT	(0)
#define ENC_IDX_TOP_RIGHT (3)
#define ENC_IDX_BOT_LEFT	(12)
#define ENC_IDX_BOT_RIGHT (15)

#define ENC_IDX_RESET_A		(2)
#define ENC_IDX_RESET_B		(3)

bool is_reset_pressed(void) {
	return hw_enc_switch_held(ENC_IDX_RESET_A) &&
				 hw_enc_switch_held(ENC_IDX_RESET_B);
}

bool is_bootloader_gesture_pressed(void) {
	return hw_enc_switch_held(ENC_IDX_TOP_LEFT) &&
				 hw_enc_switch_held(ENC_IDX_TOP_RIGHT) &&
				 hw_enc_switch_held(ENC_IDX_BOT_LEFT) &&
				 hw_enc_switch_held(ENC_IDX_BOT_RIGHT);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void sw_encoder_init(void) {
	// Initialise encoder devices and virtual parameter mappings
	enum midi_cc cc = MIDI_CC_MIN;
	for (uint b = 0; b < NUM_ENC_BANKS; b++) {
		for (uint e = 0; e < NUM_ENCODERS; e++) {
			struct encoder* enc = &gENCODERS[b][e];

			encoder_movement_init(&enc->enc_ctx);
			enc->idx							= (u8)e;
			enc->quad_ctx					= &gQUAD_ENC[e];
			enc->display.mode			= DIS_MODE_MULTI_PWM;
			enc->display.virtmode = VIRTMAP_DISPLAY_OVERLAY;
			enc->vmap_mode				= VIRTMAP_MODE_TOGGLE;
			enc->vmap_active			= 0;
			enc->sw_mode					= SW_MODE_VMAP_CYCLE;
			enc->sw_state					= SWITCH_IDLE;

			// Defaults
			// Row 1 (idx = 0,1,2,3) = pan encoder (detent true)
			// Row 2 (idx = 4,5,6,7) = filter encoder (detent true)
			// Row 3 (idx = 8,9,10,11) = send encoder (detent false)
			// Row 4 (idx = 12,13,14,15) = volume filter encoder (detent false)

			if (enc->idx < 4) {
				enc->detent				= true;
				enc->display.mode = DIS_MODE_SINGLE;
			} else if (enc->idx < 8) {
				enc->detent				= true;
				enc->display.mode = DIS_MODE_MULTI_PWM;
			} else if (enc->idx < 12) {
				enc->detent				= false;
				enc->display.mode = DIS_MODE_SINGLE;
			} else {
				enc->detent				= false;
				enc->display.mode = DIS_MODE_MULTI;
			}

			for (uint v = 0; v < NUM_VMAPS_PER_ENC; v++) {
				struct virtmap* map		= &enc->vmaps[v];
				map->position.start		= ENC_MIN;
				map->position.stop		= ENC_MAX;
				map->cfg.midi.mode		= MIDI_MODE_CC;
				map->cfg.type					= PROTOCOL_MIDI;
				map->cfg.midi.channel = 0;
				map->cfg.midi.cc			= cc++;
				vmap_apply_mode_range(map);

				// Set initial HSV values based on encoder index
				// This will create a nice color gradient across encoders
				// The hue value range is 0-1536 (0-360 degrees in 16-bit)
				// The value 0 = red, and 1536 = red again

				// Set the hue based on the encoder index
				map->hsv.hue				= (u16)(enc->idx * 96);
				map->hsv.saturation = 255;
				map->hsv.value			= 255;

				// Update RGB values from HSV values
				color_update_vmap_rgb(map);

				// Assign RB (red/blue detent LEDs) based on encoder index
				if (enc->detent) {
					map->curr_pos = ENC_MID;
					// Assign RB based on encoder index
					if (enc->idx < 4) {
						map->rb.red	 = MAX_BRIGHTNESS;
						map->rb.blue = 0x00;
					} else if (enc->idx < 8) {
						map->rb.red	 = MAX_BRIGHTNESS / 2;
						map->rb.blue = MAX_BRIGHTNESS;
					} else if (enc->idx < 12) {
						map->rb.red	 = 0x00;
						map->rb.blue = MAX_BRIGHTNESS;
					} else {
						map->rb.red	 = MAX_BRIGHTNESS;
						map->rb.blue = MAX_BRIGHTNESS;
					}
				}
			}
		}
	}
}

static void sw_encoder_update(void) {
	for (uint i = 0; i < NUM_ENCODERS; i++) {
		struct encoder* enc = &gENCODERS[gRT.curr_bank][i];

		const enum switch_state edge = hw_enc_switch_state(enc->idx);

		if (edge == SWITCH_PRESSED) {
			enc->sw_state = SWITCH_PRESSED;
			switch (enc->sw_mode) {
				case SW_MODE_NONE: {
					break;
				}

				case SW_MODE_VMAP_CYCLE: {
					set_vmap_active(enc, gRT.curr_bank,
													(enc->vmap_active + 1) % NUM_VMAPS_PER_ENC);
					mf_draw_encoder(enc);
					break;
				}

				case SW_MODE_VMAP_HOLD: {
					// ?
					break;
				}

				case SW_MODE_RESET_ON_PRESS: {
					enc->vmaps[enc->vmap_active].curr_pos = 0;
					break;
				}

				case SW_MODE_RESET_ON_RELEASE: {
					break;
				}

				case SW_MODE_FINE_ADJUST_TOGGLE: {
					break;
				}

				case SW_MODE_FINE_ADJUST_HOLD: {
					break;
				}

				default: break;
			}

		} else if (edge == SWITCH_RELEASED) {
			enc->sw_state = SWITCH_IDLE;
			switch (enc->sw_mode) {
				case SW_MODE_NONE: {
					break;
				}

				case SW_MODE_VMAP_CYCLE: {
					break;
				}

				case SW_MODE_VMAP_HOLD: {
					// ?
					break;
				}

				case SW_MODE_RESET_ON_PRESS: {
					break;
				}

				case SW_MODE_RESET_ON_RELEASE: {
					enc->vmaps[enc->vmap_active].curr_pos = 0;
					break;
				}

				case SW_MODE_FINE_ADJUST_TOGGLE: {
					break;
				}

				case SW_MODE_FINE_ADJUST_HOLD: {
					break;
				}

				default: break;
			}
		}

		int	 dir	 = quadrature_direction(enc->quad_ctx);
		bool moved = encoder_movement_update(&enc->enc_ctx, dir);

		if (!moved) {
			continue;
		}

		if (enc->vmap_mode == VIRTMAP_MODE_TOGGLE) {
			vmap_update(enc, &enc->vmaps[enc->vmap_active]);
		} else {
			for (uint v = 0; v < NUM_VMAPS_PER_ENC; v++) {
				vmap_update(enc, &enc->vmaps[v]);
			}
		}

		/*
			At this point the encoder was moved, and we have calculated a new
			position, and values for each of its virtual mappings. Now we need
			to update the display.
			The
		*/

		if (enc->update_display == 0) {
			enc->update_display = systime_ms();
		}
	}
}

// Choke point for every runtime enc->vmap_active mutation - posts an IO
// event instead of pushing sysex directly, so this file stays unaware of
// who's listening. Callers still call mf_draw_encoder() themselves after.
static void set_vmap_active(struct encoder* enc, u8 bank, u8 new_active) {
	enc->vmap_active = new_active;

	struct io_event evt = {
			.type	 = EVT_IO_ENCODER_FIELD_CHANGED,
			.bank	 = bank,
			.enc	 = enc->idx,
			.vmap	 = 0xFF, // encoder-scoped field, not per-vmap
			.field = IO_FIELD_VMAP_ACTIVE,
			.value = new_active,
	};
	event_post(EVENT_CHANNEL_IO, &evt);
}

// Choke point for every gRT.curr_bank mutation - posts the bank-change
// animation event, redraws the new bank, and notifies a webui client, so
// every caller (side switches here, sysex SET) gets all three for free.
void set_active_bank(u8 new_bank) {
	u8 prev_bank = gRT.curr_bank;
	if (new_bank == prev_bank) {
		return;
	}
	gRT.curr_bank = new_bank;

	struct animation_event anim_evt = {
			.type							= ANIM_EVT_BANK_CHANGE,
			.data.bank_change = {.prev_bank = prev_bank, .new_bank = new_bank},
	};
	event_post(EVENT_CHANNEL_ANIMATION, &anim_evt);

	for (u8 e = 0; e < NUM_ENCODERS; e++) {
		gENCODERS[new_bank][e].update_display = 1;
	}

	struct io_event io_evt = {
			.type	 = EVT_IO_ENCODER_FIELD_CHANGED,
			.bank	 = 0xFF,
			.enc	 = 0xFF,
			.vmap	 = 0xFF,
			.field = IO_FIELD_ACTIVE_BANK,
			.value = new_bank,
	};
	event_post(EVENT_CHANNEL_IO, &io_evt);
}

static void vmap_update(struct encoder* enc, struct virtmap* vmap) {
	i16 newpos = vmap->curr_pos + enc->enc_ctx.velocity;
	newpos		 = CLAMP(newpos, ENC_MIN, ENC_MAX);
	newpos		 = CLAMP(newpos, vmap->position.start, vmap->position.stop);

	if ((vmap->curr_pos == newpos) ||
			!(IN_RANGE(newpos, vmap->position.start, vmap->position.stop))) {
		return;
	}

	vmap->curr_pos = (u8)newpos;

	// Unsolicited live-position push for a connected web client - see
	// MF_SYSEX_PARAM_VMAP_CURR_POS/ENCODER_LIVE_POSITION_STREAM in
	// sysex.h. Kept inline rather than going through EVENT_CHANNEL_IO
	// like set_vmap_active() above - this fires on every quadrature
	// tick, too hot a path for the extra queue hop.
	if (gRT.live_position_streaming) {
		midi_event_s live_pos_evt = {
				.type = MIDI_EVENT_SYSEX,
				.data.sysex_out =
						{
								.cmd			= MF_SYSEX_WEBUI_PUSH,
								.param		= MF_SYSEX_PARAM_VMAP_CURR_POS,
								.data_len = 4,
								.data			= {gRT.curr_bank, enc->idx, (u8)(vmap - enc->vmaps),
														 vmap->curr_pos},
						},
		};
		event_post(EVENT_CHANNEL_MIDI_OUT, &live_pos_evt);
	}

	switch (vmap->cfg.type) {

		case PROTOCOL_MIDI: {
			switch (vmap->cfg.midi.mode) {
				case MIDI_MODE_DISABLED: {
					break;
				}

				case MIDI_MODE_CC: {
					i16 val = convert_range_i16(vmap->curr_pos, vmap->position.start,
																			vmap->position.stop, vmap->range.lower,
																			vmap->range.upper);

					if (vmap->curr_val == val) {
						break;
					}

					vmap->curr_val = val;

					midi_event_s midi_evt;
					midi_evt.type						 = MIDI_EVENT_CC;
					midi_evt.data.cc.channel = vmap->cfg.midi.channel;
					midi_evt.data.cc.control = vmap->cfg.midi.cc;
					midi_evt.data.cc.value	 = val & MIDI_CC_MAX;
					event_post(EVENT_CHANNEL_MIDI_OUT, &midi_evt);
					break;
				}

				case MIDI_MODE_CC_14: {
					i16 val = convert_range_i16(vmap->curr_pos, vmap->position.start,
																			vmap->position.stop, vmap->range.lower,
																			vmap->range.upper);

					if (vmap->curr_val == val) {
						break;
					}

					vmap->curr_val = val;

					midi_event_s midi_evt;
					// Send the MSB
					midi_evt.type						 = MIDI_EVENT_CC;
					midi_evt.data.cc.channel = vmap->cfg.midi.channel;
					midi_evt.data.cc.control = vmap->cfg.midi.cc;
					midi_evt.data.cc.value	 = (val >> 7) & 0x7F;
					event_post(EVENT_CHANNEL_MIDI_OUT, &midi_evt);

					// Then the LSB
					midi_evt.data.cc.control = (u8)vmap->cfg.midi.cc + 32;
					midi_evt.data.cc.value	 = val & 0x7F;
					event_post(EVENT_CHANNEL_MIDI_OUT, &midi_evt);
					break;
				}

				case MIDI_MODE_REL_CC: {
					break;
				}
				case MIDI_MODE_NOTE: {
					break;
				}
			}

			break;
		}

		case PROTOCOL_OSC: {
			break;
		}

		case PROTOCOL_NONE:
		default: break;
	}
}

static int midi_in_handler(void* evt) {
	midi_event_s* midi = (midi_event_s*)evt;

	switch (midi->type) {
		case MIDI_EVENT_CC: {
			for (uint b = 0; b < NUM_ENC_BANKS; b++) {
				for (uint e = 0; e < NUM_ENCODERS; e++) {
					struct encoder* enc = &gENCODERS[b][e];

					for (int v = 0; v < NUM_VMAPS_PER_ENC; v++) {
						struct virtmap* vmap = &enc->vmaps[v];
						if (vmap->cfg.type != PROTOCOL_MIDI) {
							continue;
						} else if (vmap->cfg.midi.channel != midi->data.cc.channel) {
							continue;
						} else if (vmap->cfg.midi.cc != midi->data.cc.control) {
							continue;
						}

						// do not update if the encoder is moving.
						if (enc->enc_ctx.velocity != 0) {
							continue;
						}

						i16 newpos = convert_range_i16(
								midi->data.cc.value, vmap->range.lower, vmap->range.upper,
								vmap->position.start, vmap->position.stop);
						newpos = CLAMP(newpos, ENC_MIN, ENC_MAX);

						if (vmap->curr_pos == (u8)newpos) {
							continue;
						}

						vmap->curr_pos = (u8)newpos;

						if (enc->update_display == 0) {
							enc->update_display = systime_ms();
						}
					}
				}
			}

			break;
		}
	}

	return 0;
}

static void sw_side_switch_init(void) {
	// Initialize side switches with their default modes
	// 0 is bottom left, increasing in clockwise direction
	gSIDE_SWITCHES[1].mode = SIDE_SW_MODE_BANK_PREV;
	gSIDE_SWITCHES[4].mode = SIDE_SW_MODE_BANK_NEXT;

	gSIDE_SWITCHES[2].mode = SIDE_SW_MODE_ALL_VMAP_CYCLE;
	gSIDE_SWITCHES[3].mode = SIDE_SW_MODE_ALL_VMAP_HOLD;

	gSIDE_SWITCHES[0].mode = SIDE_SW_MODE_NONE;
	gSIDE_SWITCHES[5].mode = SIDE_SW_MODE_NONE;
}

static void sw_side_switch_update(void) {
	// For each side switch, handle actions according to its mode and current
	// state
	for (u8 i = 0; i < NUM_SIDE_SWITCHES; i++) {

		enum switch_state state = hw_side_switch_state(i);

		if (state == SWITCH_PRESSED) {
			switch (gSIDE_SWITCHES[i].mode) {
				case SIDE_SW_MODE_NONE:
					// Do nothing
					break;

				case SIDE_SW_MODE_ALL_VMAP_CYCLE:
					// Cycle vmaps on all encoders
					for (u8 e = 0; e < NUM_ENCODERS; e++) {
						struct encoder* enc = &gENCODERS[gRT.curr_bank][e];
						set_vmap_active(enc, gRT.curr_bank,
														(enc->vmap_active + 1) % NUM_VMAPS_PER_ENC);
						mf_draw_encoder(enc);
					}
					break;

				case SIDE_SW_MODE_ALL_VMAP_HOLD:
					// Store current vmap for each encoder to restore later
					for (u8 e = 0; e < NUM_ENCODERS; e++) {
						struct encoder* enc = &gENCODERS[gRT.curr_bank][e];
						gSIDE_SWITCHES[i].prev_vmap_active[e] = enc->vmap_active;
						set_vmap_active(enc, gRT.curr_bank,
														(enc->vmap_active + 1) % NUM_VMAPS_PER_ENC);
						mf_draw_encoder(enc);
					}
					break;

				case SIDE_SW_MODE_BANK_PREV: {
					u8 new_bank =
							gRT.curr_bank > 0 ? gRT.curr_bank - 1 : NUM_ENC_BANKS - 1;
					set_active_bank(new_bank);
					break;
				}

				case SIDE_SW_MODE_BANK_NEXT: {
					set_active_bank((gRT.curr_bank + 1) % NUM_ENC_BANKS);
					break;
				}

				case SIDE_SW_MODE_RESERVED:
					// Reserved for future use
					break;
			}
		} else if (state == SWITCH_RELEASED) {
			switch (gSIDE_SWITCHES[i].mode) {
				case SIDE_SW_MODE_ALL_VMAP_HOLD:
					// Restore original vmap for each encoder
					for (u8 e = 0; e < NUM_ENCODERS; e++) {
						struct encoder* enc = &gENCODERS[gRT.curr_bank][e];
						set_vmap_active(enc, gRT.curr_bank,
														gSIDE_SWITCHES[i].prev_vmap_active[e]);
						mf_draw_encoder(enc);
					}
					break;

				default:
					// Most switch modes don't need action on release
					break;
			}
		}
	}
}
