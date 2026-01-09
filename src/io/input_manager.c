/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <math.h>
#include <string.h>

#include "console/console.h"
#include "event/event.h"
#include "event/general.h"
#include "event/midi.h"
#include "event/sys.h"
#include "io/encoder.h"
#include "led/led.h"
#include "lfo/lfo.h"
#include "system/config.h"
#include "system/error.h"
#include "system/hardware.h"
#include "system/print.h"
#include "system/time.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define WAVEFORM_CHANGE_THRESHOLD 20
#define LFO_MENU_TIMEOUT_MS				10000
#define LFO_MENU_LONG_PRESS_MS		1000

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void sw_encoder_init(void);
static void sw_encoder_update(void);
static void sw_side_switch_init(void);
static void sw_side_switch_update(void);
static void vmap_update(struct encoder* enc, struct virtmap* map);
static int	midi_in_handler(void* evt);

// Add state for LFO display priority
static enum lfo_display_priority g_active_lfo_display_mode =
		LFO_DISPLAY_PRIO_NONE;

/* LFO Menu State */
static enum lfo_menu_page g_lfo_menu_page									 = LFO_MENU_PAGE_NONE;
static u32								g_lfo_menu_last_activity_time		 = 0;
static u32								g_side_button_5_press_start_time = 0;
static bool								g_side_button_5_was_long_pressed =
		false; // Flag to indicate if current press is/was a long press

// MIDI Learn State
static bool g_midi_learn_active = false;
static u8		g_midi_learn_target_encoder_idx =
		0xFF; // Store which encoder is in learn mode
static u32							g_midi_learn_start_time = 0;
static struct proto_cfg g_midi_learn_original_vmap_cfg;

/* Variables to track accumulated encoder movement for waveform selection */
static i16 g_waveform_accumulator[MAX_LFOS] = {0};

EVT_HANDLER(1, evt_midi, midi_in_handler);

struct encoder		 gENCODERS[NUM_ENC_BANKS][NUM_ENCODERS];
struct side_switch gSIDE_SWITCHES[NUM_SIDE_SWITCHES];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */

// Getter function for LED module
enum lfo_display_priority input_manager_get_active_lfo_display_mode(void) {
	return g_active_lfo_display_mode;
}

// Getter function for LFO menu page for LED module
enum lfo_menu_page input_manager_get_lfo_menu_page(void) {
	return g_lfo_menu_page;
}

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

bool is_reset_pressed(void) {
	return hw_enc_switch_state(2) == SWITCH_PRESSED &&
				 hw_enc_switch_state(3) == SWITCH_PRESSED;
}

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
				map->range.lower			= MIDI_CC_MIN;
				map->range.upper			= MIDI_CC_MAX;
				map->cfg.midi.mode		= MIDI_MODE_CC;
				map->cfg.type					= PROTOCOL_MIDI;
				map->cfg.midi.channel = 0;
				map->cfg.midi.cc			= cc++;

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
						map->rb.red	 = 0x1F;
						map->rb.blue = 0x00;
					} else if (enc->idx < 8) {
						map->rb.red	 = 0x0F;
						map->rb.blue = 0x1F;
					} else if (enc->idx < 12) {
						map->rb.red	 = 0x00;
						map->rb.blue = 0x1F;
					} else {
						map->rb.red	 = 0x1F;
						map->rb.blue = 0x1F;
					}
				}
			}
		}
	}
}

static void sw_encoder_update(void) {
	// Check if any special side switches are active
	bool side_switch_0_held = hw_side_switch_is_held(0);

	u32 current_time_ms = systime_ms(); // Get time once for this update cycle

	for (uint i = 0; i < NUM_ENCODERS; i++) {
		struct encoder* enc = &gENCODERS[gRT.curr_bank][i];
		enc->sw_state				= hw_enc_switch_state(enc->idx);

		// MIDI Learn Activation
		if (side_switch_0_held && enc->sw_state == SWITCH_PRESSED) {
			if (g_midi_learn_active && g_midi_learn_target_encoder_idx == enc->idx) {
				// Pressing again cancels learn for this encoder
				g_midi_learn_active = false;

				struct gen_event evt = {.type				= EVT_GEN_MIDI_LEARN_CANCEL,
																.midi_learn = {
																		.encoder_idx = enc->idx,
																}};
				event_post(EVENT_CHANNEL_GEN, &evt);
			} else {
				// Start MIDI Learn
				g_midi_learn_active							= true;
				g_midi_learn_target_encoder_idx = enc->idx;
				g_midi_learn_start_time					= current_time_ms;
				// Store original config of the active vmap
				memcpy(&g_midi_learn_original_vmap_cfg,
							 &enc->vmaps[enc->vmap_active].cfg, sizeof(struct proto_cfg));
				struct gen_event evt = {.type				= EVT_GEN_MIDI_LEARN_START,
																.midi_learn = {
																		.encoder_idx = enc->idx,
																}};
				event_post(EVENT_CHANNEL_GEN, &evt);

				console_puts_p(PSTR("MIDI Learn: Started for encoder "));
				console_put_uint(enc->idx);
				console_puts_p(PSTR(" vmap "));
				console_put_uint(enc->vmap_active);
				console_puts_p(PSTR("\r\n"));
			}
			g_lfo_menu_last_activity_time = current_time_ms; // Activity
			enc->sw_state									= SWITCH_IDLE;		 // Consume
			continue;
		}

		// LFO Waveform Cycling (now via encoder press in LFO menu pages)
		if ((g_lfo_menu_page == LFO_MENU_PAGE_RATE ||
				 g_lfo_menu_page == LFO_MENU_PAGE_DEPTH) &&
				enc->sw_state == SWITCH_PRESSED && enc->idx < MAX_LFOS) {
			console_puts_p(PSTR("LFO assignment active + encoder "));
			console_put_uint(enc->idx);
			console_puts_p(PSTR(" pressed - cycling LFO\r\n"));
			lfo_cycle_waveform(enc->idx);

			if (lfo_get_waveform(enc->idx) != LFO_WAVE_NONE) {
				lfo_assign(enc->idx, gRT.curr_bank, enc->idx, enc->vmap_active);
			} else {
				// Optional: If cycled to OFF, unassign it.
				lfo_unassign(enc->idx);
			}

			enc->update_display						= 1;
			g_lfo_menu_last_activity_time = current_time_ms;
			enc->sw_state									= SWITCH_IDLE; // Consume the switch event
			continue; // Skip normal encoder switch handling
		}

		// Normal encoder switch press/release logic (only if not in an LFO menu)
		if (g_lfo_menu_page != LFO_MENU_PAGE_NONE) {
			// If in any LFO menu, consume regular switch presses to avoid conflicts
			if (enc->sw_state == SWITCH_PRESSED || enc->sw_state == SWITCH_RELEASED)
				enc->sw_state = SWITCH_IDLE;
		} else { // Not in LFO menu, handle normal switch presses
			if (enc->sw_state == SWITCH_PRESSED) {
				switch (enc->sw_mode) {
					case SW_MODE_NONE: {
						break;
					}

					case SW_MODE_VMAP_CYCLE: {
						enc->vmap_active = (u8)((enc->vmap_active + 1) % NUM_VMAPS_PER_ENC);
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

				enc->sw_state = SWITCH_IDLE;
			} else if (enc->sw_state == SWITCH_RELEASED) {
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
				enc->sw_state = SWITCH_IDLE;
			}
		}

		int	 dir	 = quadrature_direction(enc->quad_ctx);
		bool moved = encoder_movement_update(&enc->enc_ctx, dir);

		if (!moved) {
			continue;
		}

		// If encoder moved while in an LFO menu, update activity time
		if (g_lfo_menu_page != LFO_MENU_PAGE_NONE) {
			g_lfo_menu_last_activity_time = current_time_ms;
		}

		// --- LFO Menu Mode: Encoder Turn Actions (Rate/Depth) ---
		// Waveform selection is now done by encoder PRESS in these menus.
		if (g_lfo_menu_page == LFO_MENU_PAGE_RATE && enc->idx < MAX_LFOS &&
				lfo_get_waveform(enc->idx) != LFO_WAVE_NONE) {
			float current_rate = lfo_get_rate(enc->idx);
			i16		velocity_step =
					enc->enc_ctx.velocity; // Use full velocity for more range

			float rate_change_factor;
			// More sensitive at lower rates, less sensitive at higher rates
			if (current_rate < 0.1f)
				rate_change_factor = 0.0005f * (float)abs(velocity_step);
			else if (current_rate < 1.0f)
				rate_change_factor = 0.005f * (float)abs(velocity_step);
			else if (current_rate < 5.0f)
				rate_change_factor = 0.05f * (float)abs(velocity_step);
			else
				rate_change_factor = 0.25f * (float)abs(velocity_step);

			if (velocity_step == 0)
				rate_change_factor = 0.0f;

			float rate_change =
					(velocity_step > 0) ? rate_change_factor : -rate_change_factor;

			// Ensure a very small minimum change if there was movement
			if (velocity_step != 0 && rate_change_factor > 0.0f &&
					fabsf(rate_change) < 0.0001f) {
				rate_change = (velocity_step > 0) ? 0.0001f : -0.0001f;
			}

			float new_rate = current_rate + rate_change;
			new_rate			 = CLAMP(new_rate, 0.01f, 10.0f); // Clamp to 10Hz maximum
			lfo_set_rate(enc->idx, new_rate);

			console_puts_p(PSTR("LFO Menu (Rate) LFO "));
			console_put_uint(enc->idx);
			console_puts_p(PSTR(": Rate set to "));
			console_put_float_val(new_rate);
			console_puts_p(PSTR(" Hz\r\n"));

			enc->update_display = 1;
			continue; // Skip normal encoder movement handling
		}

		if (g_lfo_menu_page == LFO_MENU_PAGE_DEPTH && enc->idx < MAX_LFOS &&
				lfo_get_waveform(enc->idx) != LFO_WAVE_NONE) {
			i8	current_depth = lfo_get_depth(enc->idx);
			i16 velocity_step = enc->enc_ctx.velocity / 4; // Slower adjustment
			if (velocity_step == 0 && enc->enc_ctx.velocity != 0) {
				velocity_step = (enc->enc_ctx.velocity > 0) ? 1 : -1;
			}
			i16 new_depth_16 =
					(i16)current_depth + velocity_step; // Perform arithmetic as i16
			i8 new_depth = (i8)CLAMP(new_depth_16, -100, 100); // Clamp and cast to i8
			lfo_set_depth(enc->idx, new_depth);

			console_puts_p(PSTR("LFO Menu (Depth) LFO "));
			console_put_uint(enc->idx);
			console_puts_p(PSTR(": Depth set to "));
			console_put_int(new_depth);
			console_puts_p(PSTR(" %\r\n"));

			enc->update_display						= 1;
			g_lfo_menu_last_activity_time = current_time_ms; // Update activity time
			continue; // Skip normal encoder movement handling
		}

		// Normal encoder movement handling (only if not in an LFO menu AND not in
		// MIDI learn for this encoder)
		if (g_lfo_menu_page == LFO_MENU_PAGE_NONE &&
				(!g_midi_learn_active || g_midi_learn_target_encoder_idx != enc->idx)) {
			struct virtmap* vmap	 = &enc->vmaps[enc->vmap_active];
			u8							oldpos = lfo_get_base_position_for_manual_control(
					 gRT.curr_bank, enc->idx, enc->vmap_active);
			u8 newpos = (u8)CLAMP((i16)oldpos + enc->enc_ctx.velocity,
														vmap->position.start, vmap->position.stop);

			if (newpos != oldpos) {
				lfo_notify_manual_change(gRT.curr_bank, enc->idx, enc->vmap_active,
																 newpos);
				bool has_active_lfo = false; // Check if an *active* LFO is assigned
				for (u8 lfo_i = 0; lfo_i < MAX_LFOS; lfo_i++) {
					if (gLFOs[lfo_i].active &&
							gLFOs[lfo_i].assigned_bank == gRT.curr_bank &&
							gLFOs[lfo_i].assigned_encoder == enc->idx &&
							gLFOs[lfo_i].assigned_vmap == enc->vmap_active) {
						has_active_lfo = true;
						break;
					}
				}
				if (!has_active_lfo) { // Only update vmap directly if no active LFO
					vmap->curr_pos = newpos;
					vmap_update(enc, vmap); // This eventually sends MIDI
				}
			}
			if (enc->update_display == 0) {
				enc->update_display = current_time_ms;
			}
		}
	}
}

static void vmap_update(struct encoder* enc, struct virtmap* vmap) {
	// This function handles protocol updates for manual encoder movements
	// LFO-controlled encoders also call this when their base position changes

	switch (vmap->cfg.type) {

		case PROTOCOL_MIDI: {
			switch (vmap->cfg.midi.mode) {
				case MIDI_MODE_DISABLED: {
					break;
				}

				case MIDI_MODE_CC: {
					bool invert = (vmap->range.lower > vmap->range.upper);

					i16 val = convert_range_i16(vmap->curr_pos, vmap->position.start,
																			vmap->position.stop, vmap->range.lower,
																			vmap->range.upper);

					if (invert) {
						val = MIDI_CC_MAX - val;
					}

					// Clamp final MIDI value to valid range
					val = CLAMP(val, 0, MIDI_CC_MAX);

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
					bool invert = (vmap->range.lower > vmap->range.upper);

					i16 val = convert_range_i16(vmap->curr_pos, vmap->position.start,
																			vmap->position.stop, vmap->range.lower,
																			vmap->range.upper);

					if (invert) {
						val = 0x3FFF - val;
					}

					// Clamp to 14-bit range
					val = CLAMP(val, 0, 0x3FFF);

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
		case MIDI_EVENT_CC: // Handle incoming CC for MIDI Learn
			if (g_midi_learn_active) {
				midi_cc_event_s* cc_evt = &midi->data.cc; // Get CC event data
				struct encoder*	 learn_enc =
						&gENCODERS[gRT.curr_bank]
											[g_midi_learn_target_encoder_idx]; // Assuming current
																												 // bank
				struct virtmap* learn_vmap =
						&learn_enc->vmaps[learn_enc->vmap_active]; // Assuming active vmap

				console_puts_p(PSTR("MIDI Learn: Received CC. Chan: "));
				console_put_uint(cc_evt->channel);
				console_puts_p(PSTR(", CC: "));
				console_put_uint(cc_evt->control);
				console_puts_p(PSTR(", Val: "));
				console_put_uint(cc_evt->value);
				console_puts_p(PSTR("\r\n"));

				learn_vmap->cfg.type				 = PROTOCOL_MIDI;
				learn_vmap->cfg.midi.mode		 = MIDI_MODE_CC;
				learn_vmap->cfg.midi.channel = cc_evt->channel;
				learn_vmap->cfg.midi.cc			 = cc_evt->control;
				// Keep existing range

				console_puts_p(PSTR("MIDI Learn: SUCCESS for encoder "));
				console_put_uint(g_midi_learn_target_encoder_idx);
				console_puts_p(PSTR(", vmap "));
				console_put_uint(learn_enc->vmap_active);
				console_puts_p(PSTR("\r\n"));

				g_midi_learn_active = false;
				struct gen_event evt;
				evt.type									 = EVT_GEN_MIDI_LEARN_SUCCESS;
				evt.midi_learn.encoder_idx = g_midi_learn_target_encoder_idx;
				event_post(EVENT_CHANNEL_GEN, &evt);
			}

			// Continue with regular CC processing for all encoders
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
						u16 newpos = (u16)convert_range_i16(
								midi->data.cc.value, vmap->range.lower, vmap->range.upper,
								vmap->position.start, vmap->position.stop);

						vmap->curr_pos = (u8)newpos;
					}
				}
			}
			break;
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

	// Assign MIDI Learn to side switch 0 and LFO menu control to side switch 5
	gSIDE_SWITCHES[0].mode = SIDE_SW_MODE_MIDI_LEARN;
	gSIDE_SWITCHES[5].mode = SIDE_SW_MODE_LFO_MENU_CONTROL;
}

static void sw_side_switch_update(void) {
	// For each side switch, handle actions according to its mode and current
	// state
	for (u8 i = 0; i < NUM_SIDE_SWITCHES; i++) {
		enum switch_state state = hw_side_switch_state(i);
		u32								current_time =
				systime_ms(); // Get current time for this switch iteration

		// LFO Menu Timeout Check (runs for each switch, but g_lfo_menu_page is
		// global)
		if (g_lfo_menu_page != LFO_MENU_PAGE_NONE) {
			if (current_time - g_lfo_menu_last_activity_time >= LFO_MENU_TIMEOUT_MS) {
				console_puts_p(PSTR("LFO Menu: Timeout -> Exiting\\r\\n"));
				g_lfo_menu_page = LFO_MENU_PAGE_NONE;
				for (u8 e_idx = 0; e_idx < NUM_ENCODERS; e_idx++)
					gENCODERS[gRT.curr_bank][e_idx].update_display = 1;
			}
		}

		// MIDI Learn Timeout Check (similar logic)
		if (g_midi_learn_active &&
				(current_time - g_midi_learn_start_time >= 10000)) {
			console_puts_p(PSTR("MIDI Learn: Timeout for encoder "));
			console_put_uint(g_midi_learn_target_encoder_idx);
			console_puts_p(PSTR("\r\n"));
			// Revert to original config
			struct encoder* learn_enc =
					&gENCODERS[gRT.curr_bank]
										[g_midi_learn_target_encoder_idx]; // Assuming learn is on
																											 // current bank for now
			struct virtmap* learn_vmap =
					&learn_enc
							 ->vmaps[learn_enc->vmap_active]; // Assuming learn on active vmap
			memcpy(&learn_vmap->cfg, &g_midi_learn_original_vmap_cfg,
						 sizeof(struct proto_cfg));
			g_midi_learn_active				= false;
			learn_enc->update_display = 1;
		}

		// --- Special Handling for Side Switch 5 (LFO Menu Control) ---
		if (gSIDE_SWITCHES[i].mode == SIDE_SW_MODE_LFO_MENU_CONTROL) {
			if (state == SWITCH_PRESSED) {
				g_side_button_5_press_start_time = current_time;
				g_side_button_5_was_long_pressed = false; // Reset long press flag
			} else if (hw_side_switch_is_held(i)) {
				// Check for long press only if a press was initiated and not yet
				// flagged as long
				if (g_side_button_5_press_start_time != 0 &&
						!g_side_button_5_was_long_pressed) {
					if (current_time - g_side_button_5_press_start_time >=
							LFO_MENU_LONG_PRESS_MS) {
						g_side_button_5_was_long_pressed =
								true; // Flag that a long press occurred
						if (g_lfo_menu_page == LFO_MENU_PAGE_NONE) {
							console_puts_p(
									PSTR("LFO Menu: Long press -> Entering Rate Menu\\r\\n"));
							g_lfo_menu_page = LFO_MENU_PAGE_RATE;
						} else {
							console_puts_p(
									PSTR("LFO Menu: Long press -> Exiting Menu\\r\\n"));
							g_lfo_menu_page = LFO_MENU_PAGE_NONE;
						}
						g_lfo_menu_last_activity_time =
								current_time; // Update activity time
						for (u8 e = 0; e < NUM_ENCODERS; e++)
							gENCODERS[gRT.curr_bank][e].update_display = 1;
					}
				}
			} else if (state == SWITCH_RELEASED) {
				// Process release only if a press was initiated
				if (g_side_button_5_press_start_time != 0) {
					if (!g_side_button_5_was_long_pressed) {
						// This was a SHORT PRESS release
						if (g_lfo_menu_page == LFO_MENU_PAGE_NONE) {
							console_puts_p(PSTR("LFO Menu: Short press (not in menu) -> "
																	"Flashing Active LFOs\\r\\n"));

							// No change to g_lfo_menu_page, no menu timeout update needed for
							// flash
						} else if (g_lfo_menu_page == LFO_MENU_PAGE_RATE) {
							console_puts_p(PSTR(
									"LFO Menu: Short press (Rate page) -> Depth Page\\r\\n"));
							g_lfo_menu_page = LFO_MENU_PAGE_DEPTH;
							g_lfo_menu_last_activity_time =
									current_time; // Update activity time
							for (u8 e = 0; e < NUM_ENCODERS; e++)
								gENCODERS[gRT.curr_bank][e].update_display = 1;
						} else if (g_lfo_menu_page == LFO_MENU_PAGE_DEPTH) {
							console_puts_p(PSTR(
									"LFO Menu: Short press (Depth page) -> Rate Page\\r\\n"));
							g_lfo_menu_page = LFO_MENU_PAGE_RATE; // Cycle back to Rate page
							g_lfo_menu_last_activity_time =
									current_time; // Update activity time
							for (u8 e = 0; e < NUM_ENCODERS; e++)
								gENCODERS[gRT.curr_bank][e].update_display = 1;
						}
					}
					// Reset press_start_time for the next distinct press, regardless of
					// short or long action.
					g_side_button_5_press_start_time = 0;
				}
			}
			continue; // Crucial: Skip generic side switch handling for button 5
		}

		// --- Generic Side Switch Handling (for switches other than LFO control)
		// ---
		if (state == SWITCH_PRESSED) {
			// If any other side switch is pressed while in LFO menu, exit LFO menu
			if (g_lfo_menu_page != LFO_MENU_PAGE_NONE) {
				g_lfo_menu_page = LFO_MENU_PAGE_NONE;
				for (u8 e_idx = 0; e_idx < NUM_ENCODERS; e_idx++)
					gENCODERS[gRT.curr_bank][e_idx].update_display = 1;
			}
			// Cancel MIDI learn if active and a different side switch is pressed
			if (g_midi_learn_active) {
				console_puts_p(
						PSTR("MIDI Learn: Cancelled by other side switch press.\r\n"));
				struct encoder* learn_enc =
						&gENCODERS[gRT.curr_bank][g_midi_learn_target_encoder_idx];
				memcpy(&learn_enc->vmaps[learn_enc->vmap_active].cfg,
							 &g_midi_learn_original_vmap_cfg, sizeof(struct proto_cfg));
				g_midi_learn_active = false;
			}

			switch (gSIDE_SWITCHES[i].mode) {
				case SIDE_SW_MODE_NONE:
					// Do nothing
					break;

				case SIDE_SW_MODE_ALL_VMAP_CYCLE:
					// Cycle vmaps on all encoders
					for (u8 e = 0; e < NUM_ENCODERS; e++) {
						struct encoder* enc = &gENCODERS[gRT.curr_bank][e];
						enc->vmap_active = (u8)((enc->vmap_active + 1) % NUM_VMAPS_PER_ENC);
						mf_draw_encoder(enc);
					}
					break;

				case SIDE_SW_MODE_ALL_VMAP_HOLD:
					// Store current vmap for each encoder to restore later
					for (u8 e = 0; e < NUM_ENCODERS; e++) {
						struct encoder* enc = &gENCODERS[gRT.curr_bank][e];
						gSIDE_SWITCHES[i].prev_vmap_active[e] = enc->vmap_active;
						enc->vmap_active = (u8)((enc->vmap_active + 1) % NUM_VMAPS_PER_ENC);
						mf_draw_encoder(enc);
					}
					break;

				case SIDE_SW_MODE_BANK_PREV: {
					int prev_bank = gRT.curr_bank;

					// Decrease bank index with wrapping
					if (gRT.curr_bank > 0) {
						gRT.curr_bank = (u8)(gRT.curr_bank - 1);
					} else {
						gRT.curr_bank = (u8)(NUM_ENC_BANKS - 1);
					}

					struct gen_event evt;
					evt.bank_change.new_bank	= gRT.curr_bank;
					evt.bank_change.prev_bank = prev_bank;
					event_post(EVENT_CHANNEL_GEN, &evt);
					break;
				}

				case SIDE_SW_MODE_BANK_NEXT: {
					int prev_bank = gRT.curr_bank;
					// Increase bank index with wrapping
					gRT.curr_bank = (u8)((gRT.curr_bank + 1) % NUM_ENC_BANKS);

					struct gen_event evt;
					evt.bank_change.new_bank	= gRT.curr_bank;
					evt.bank_change.prev_bank = prev_bank;
					event_post(EVENT_CHANNEL_GEN, &evt);
					break;
				}

				case SIDE_SW_MODE_MIDI_LEARN: // Updated from MIDI_LEARN_OR_LFO_DISPLAY
					// This button now only handles MIDI learn functionality
					// MIDI learn is handled by encoder press while this is HELD.
					// No action needed on press - the actual MIDI learn logic is in
					// sw_encoder_update()
					break;

				default:
					// Most switch modes don't need action on press
					break;
			}
		} else if (state == SWITCH_RELEASED) {
			switch (gSIDE_SWITCHES[i].mode) {
				case SIDE_SW_MODE_ALL_VMAP_HOLD:
					// Restore original vmap for each encoder
					for (u8 e = 0; e < NUM_ENCODERS; e++) {
						struct encoder* enc = &gENCODERS[gRT.curr_bank][e];
						enc->vmap_active		= gSIDE_SWITCHES[i].prev_vmap_active[e];
						mf_draw_encoder(enc);
					}
					break;

				case SIDE_SW_MODE_MIDI_LEARN: // Updated from MIDI_LEARN_OR_LFO_DISPLAY
																			// If MIDI learn was active and this
																			// button is released, do nothing here
																			// (timeout or success handles it)
					// Restore normal display mode after adjusting LFO rate
					g_active_lfo_display_mode = LFO_DISPLAY_PRIO_NONE;
					for (u8 e = 0; e < NUM_ENCODERS; e++) {
						if (e < MAX_LFOS) {
							struct encoder* enc = &gENCODERS[gRT.curr_bank][e];
							if (lfo_get_waveform(e) != LFO_WAVE_NONE) {
								enc->update_display = 1; // Force display update to normal mode
							}
						}
					}
					break;

				default:
					// Most switch modes don't need action on release
					break;
			}
		}
	}
}

// // New IO Event Handler Implementation
// static int io_event_handler_im(void* evt_data) {
// 	struct io_event* evt = (struct io_event*)evt_data;

// 	switch (evt->type) {
// 		case EVT_IO_VMAP_NEEDS_MIDI_UPDATE: {
// 			u8 bank			= evt->data.vmap_midi_data.bank;
// 			u8 enc_idx	= evt->data.vmap_midi_data.encoder_idx;
// 			u8 vmap_idx = evt->data.vmap_midi_data.vmap_idx;

// 			// Validate indices
// 			if (bank >= NUM_ENC_BANKS || enc_idx >= NUM_ENCODERS ||
// 					vmap_idx >= NUM_VMAPS_PER_ENC) {
// 				// Optionally log an error
// 				return ERR_BAD_PARAM;
// 			}

// 			struct encoder* target_enc	= &gENCODERS[bank][enc_idx];
// 			struct virtmap* target_vmap = &target_enc->vmaps[vmap_idx];

// 			vmap_update(target_enc, target_vmap); // Call the existing vmap_update
// 			return SUCCESS;												// Event handled
// 		}

// 			// Handle other IO event types if necessary
// 			// case EVT_IO_ENCODER_ROTATION:
// 			//     // ...
// 			//     break;

// 		default:
// 			// This handler might not process all IO event types
// 			break;
// 	}
// 	return 0; // Return 0 if not handled or if successfully handled
// }

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
