/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <stdio.h>

#include "system/config.h"
#include "system/error.h"
#include "system/print.h"
#include "system/time.h"
#include "io/encoder.h"
#include "event/event.h"
#include "event/io.h"
#include "event/midi.h"
#include "event/sys.h"

#include "system/hardware.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void sw_encoder_init(void);
static void sw_encoder_update(void);
static void vmap_update(struct encoder* enc, struct virtmap* map);
static int	midi_in_handler(void* evt);
static void print_dir(uint enc_idx, int dir);
static void rgb_init(void);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

EVT_HANDLER(1, evt_midi, midi_in_handler);

struct encoder gENCODERS[NUM_ENC_BANKS][NUM_ENCODERS];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void input_init(void) {
	hw_encoder_init();
	hw_switch_init();
	sw_encoder_init();
	event_channel_subscribe(EVENT_CHANNEL_MIDI_IN, &evt_midi);
}

void input_update(void) {
	hw_encoder_scan();
	sw_encoder_update();
}

bool is_reset_pressed(void) {
	return hw_enc_switch_state(2) == SWITCH_PRESSED &&
				 hw_enc_switch_state(3) == SWITCH_PRESSED;
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
			// enc->display.mode			= DIS_MODE_MULTI;
			enc->display.virtmode = VIRTMAP_DISPLAY_OVERLAY;
			enc->vmap_mode				= VIRTMAP_MODE_TOGGLE;
			enc->vmap_active			= 0;
			enc->sw_mode					= SW_MODE_VMAP_CYCLE;
			enc->sw_state					= SWITCH_IDLE;

			// Defaults
			// Row 1 (idx = 0,1,2,3) = pan encoder (detent true) (rgb = light blue)
			// Row 2 (idx = 4,5,6,7) = filter encoder (detent true) (rgb = turqouise)
			// Row 3 (idx = 8,9,10,11) = send encoder (detent false) (rgb = navy)
			// Row 4 (idx = 12,13,14,15) = volume filter encoder (detent false) (rgb =
			// purple)

			if (enc->idx < 4) {
				enc->detent = true;
				enc->display.mode = DIS_MODE_MULTI_PWM;
			} else if (enc->idx < 8) {
				enc->detent = true;
				enc->display.mode = DIS_MODE_MULTI;
			} else if (enc->idx < 12) {
				enc->detent = false;
				enc->display.mode = DIS_MODE_MULTI_PWM;
			} else {
				enc->detent = false;
				enc->display.mode = DIS_MODE_MULTI;
			}

			for (uint v = 0; v < NUM_VMAPS_PER_ENC; v++) {
				struct virtmap* map				= &enc->vmaps[v];
				map->position.start		= ENC_MIN;
				map->position.stop		= ENC_MAX;
				map->range.lower			= MIDI_CC_MIN;
				map->range.upper			= MIDI_CC_MAX;
				map->cfg.midi.mode		= MIDI_MODE_CC;
				map->cfg.type					= PROTOCOL_MIDI;
				map->cfg.midi.channel = 0;
				map->cfg.midi.cc			= cc++;

				// Assign RGB based on encoder index
				if (enc->idx < 4) {
					map->rgb.red	 = 0x00;
					map->rgb.green = 0x1F;
					map->rgb.blue	 = 0x1F;
				} else if (enc->idx < 8) {
					map->rgb.red	 = 0x00;
					map->rgb.green = 0x1F;
					map->rgb.blue	 = 0x0F;
				} else if (enc->idx < 12) {
					map->rgb.red	 = 0x00;
					map->rgb.green = 0x00;
					map->rgb.blue	 = 0x1F;
				} else {
					map->rgb.red	 = 0x1F;
					map->rgb.green = 0x00;
					map->rgb.blue	 = 0x1F;
				}

				map->rgb.red	 = (map->rgb.red + 20) * v % RGB_MAX_VAL;
				map->rgb.green = (map->rgb.green - 5) * v % RGB_MAX_VAL;
				map->rgb.blue	 = (map->rgb.blue + 12) * v % RGB_MAX_VAL;

				if (enc->detent) {
					map->curr_pos = ENC_MID;
					// Assign RB based on encoder index
					if (enc->idx < 4) {
						map->rb.red	 = 0x1F;
						map->rb.blue = 0x00;
					} else if (enc->idx < 8) {
						map->rb.red	 = 0x1F;
						map->rb.blue = 0x0F;
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
	for (uint i = 0; i < NUM_ENCODERS; i++) {
		struct encoder* enc = &gENCODERS[gRT.curr_bank][i];

		enc->sw_state = hw_enc_switch_state(enc->idx);

		if (enc->sw_state == SWITCH_PRESSED) {
			switch (enc->sw_mode) {
				case SW_MODE_NONE: {
					break;
				}

				case SW_MODE_VMAP_CYCLE: {
					enc->vmap_active = (enc->vmap_active + 1) % NUM_VMAPS_PER_ENC;
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

		int dir = quadrature_direction(enc->quad_ctx);
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

static void vmap_update(struct encoder* enc, struct virtmap* vmap) {
	i16 newpos = vmap->curr_pos + enc->enc_ctx.velocity;
	newpos		 = CLAMP(newpos, ENC_MIN, ENC_MAX);
	newpos		 = CLAMP(newpos, vmap->position.start, vmap->position.stop);

	if ((vmap->curr_pos == newpos) ||
			!(IN_RANGE(newpos, vmap->position.start, vmap->position.stop))) {
		return;
	}

	vmap->curr_pos = (u8)newpos;

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
						u16 newpos = (u16)convert_range_i16(
								midi->data.cc.value, vmap->range.lower, vmap->range.upper,
								vmap->position.start, vmap->position.stop);

						vmap->curr_pos = newpos;
					}
				}
			}

			break;
		}
	}

	return 0;
}
