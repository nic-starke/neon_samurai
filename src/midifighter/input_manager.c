/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <stdio.h>

#include "sys/config.h"
#include "sys/error.h"
#include "sys/print.h"
#include "sys/time.h"
#include "input/encoder.h"
#include "event/event.h"
#include "event/io.h"
#include "event/midi.h"
#include "event/sys.h"

#include "midifighter/midifighter.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void sw_encoder_init(void);
static void sw_encoder_update(void);
static int	midi_in_handler(void* evt);
static void print_dir(uint enc_idx, int dir);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static sys_config_s config = {
		.enc_dead_time			= DEFAULT_ENC_PLAYDEAD_TIME,
		.midi_throttle_time = DEFAULT_MIDI_THROTTLE_TIME,
};

EVT_HANDLER(1, evt_midi, midi_in_handler);

static mf_encoder_s encoders[MF_NUM_ENC_BANKS][MF_NUM_ENCODERS];
static virtmap_s vmaps[MF_NUM_ENC_BANKS][MF_NUM_ENCODERS][MF_NUM_VMAPS_PER_ENC];

// PROGMEM static const midifighter_encoder_s default_config = {
// 		.enabled					= true,
// 		.detent						= false,
// 		.encoder_ctx			= {0},
// 		.hwenc_id					= 0,
// 		.led_style				= DIS_MODE_SINGLE,
// 		.led_detent.value = 0,
// 		.led_rgb.value		= 0,
// 		.midi.channel			= 0,
// 		.midi.mode				= MIDI_MODE_CC,
// 		.midi.data.cc			= 0,
// };

static uint active_bank = 0;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void mf_input_init(void) {
	hw_encoder_init();
	hw_switch_init();
	sw_encoder_init();

	event_channel_subscribe(EVENT_CHANNEL_MIDI_IN, &evt_midi);
}

void mf_input_update(void) {
	hw_encoder_scan();
	sw_encoder_update();
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void sw_encoder_init(void) {
	// Initialise encoder devices and virtual parameter mappings
	midi_cc_e cc = MIDI_CC_MIN;
	for (uint b = 0; b < MF_NUM_ENC_BANKS; b++) {
		for (uint e = 0; e < MF_NUM_ENCODERS; e++) {
			mf_encoder_s* enc = &encoders[b][e];

			encoder_init(&enc->enc_ctx);
			enc->idx							= e;
			enc->quad_ctx					= &mf_enc_quad[e];
			enc->display.mode			= DIS_MODE_MULTI_PWM;
			enc->display.virtmode = VIRTMAP_DISPLAY_OVERLAY;
			enc->virtmap.mode			= VIRTMAP_MODE_TOGGLE;
			// enc->virtmap.mode			= VIRTMAP_MODE_OVERLAY;
			enc->virtmap.head			= NULL;
			enc->sw_mode					= SW_MODE_VMAP_CYCLE;

			for (uint v = 0; v < MF_NUM_VMAPS_PER_ENC; v++) {
				virtmap_s* map = &vmaps[b][e][v];

				if (e == 0 && v == 0) {
					map->position.start	 = ENC_MIN;
					map->position.stop	 = ENC_MAX;
					map->range.lower		 = MIDI_CC_14B_MIN;
					map->range.upper		 = MIDI_CC_14B_MAX;
					map->proto.midi.mode = MIDI_MODE_CC_14;
				} else {
					map->position.start	 = ENC_MIN;
					map->position.stop	 = ENC_MAX;
					map->range.lower		 = MIDI_CC_MIN;
					map->range.upper		 = MIDI_CC_MAX;
					map->proto.midi.mode = MIDI_MODE_CC;
				}

				map->proto.type					= PROTOCOL_MIDI;
				map->proto.midi.channel = 0;
				map->proto.midi.data.cc = cc++;

				virtmap_assign(&enc->virtmap.head, map);
			}

			// Generate an event to update the display...?
			// io_event_s evt;
			// evt.type = EVT_IO_ENCODER_ROTATION;
			// evt.ctx	 = enc;
			// return event_post(EVENT_CHANNEL_IO, &evt);
		}
	}
}

static u32 last_update = 0;

static void sw_encoder_update(void) {
	bool update_enc = false;
	u32	 timenow		= systime_ms();

	// throttle update function to 1ms frequency
	if ((timenow - last_update) > 1) {
		update_enc	= true;
		last_update = timenow;
	}

	for (uint i = 0; i < MF_NUM_ENCODERS; i++) {
		mf_encoder_s* enc = &encoders[active_bank][i];

		enc->sw_state = hw_enc_switch_state(enc->idx);

		if (enc->sw_state == SWITCH_PRESSED) {
			switch (enc->sw_mode) {
				case SW_MODE_NONE: {
					break;
				}

				case SW_MODE_VMAP_CYCLE: {
					virtmap_toggle(&enc->virtmap.head);
					mf_draw_encoder(enc);
					break;
				}

				case SW_MODE_VMAP_HOLD: {
					// ?
					break;
				}

				case SW_MODE_RESET_ON_PRESS: {
					enc->virtmap.head->curr_pos = 0;
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
					enc->virtmap.head->curr_pos = 0;
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

		if (update_enc) {
			int dir = quadrature_direction(enc->quad_ctx);
			encoder_update(&enc->enc_ctx, dir);
		}

		virtmap_s* vmap = enc->virtmap.head;

		do {

			i32 newpos = vmap->curr_pos + enc->enc_ctx.velocity;
			newpos		 = CLAMP(newpos, ENC_MIN, ENC_MAX);
			newpos		 = CLAMP(newpos, vmap->position.start, vmap->position.stop);

			if ((vmap->curr_pos == newpos) ||
					!(IN_RANGE(newpos, vmap->position.start, vmap->position.stop))) {
				continue;
			}

			vmap->curr_pos = newpos;
#warning "this bypasses everything, remove it after debugging
			print_dir(i, enc->enc_ctx.direction);
			break;

			// bool skip = ((timenow - vmap->last_update) <
			// config.midi_throttle_time); if (skip) { 	continue;
			// }

			vmap->last_update = systime_ms();

			switch (vmap->proto.type) {

				case PROTOCOL_MIDI: {
					switch (vmap->proto.midi.mode) {
						case MIDI_MODE_DISABLED: {
							break;
						}

						case MIDI_MODE_CC: {
							bool invert = (vmap->range.lower > vmap->range.upper);

							i32 val = convert_range(vmap->curr_pos, vmap->position.start,
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
							midi_evt.data.cc.channel = vmap->proto.midi.channel;
							midi_evt.data.cc.control = vmap->proto.midi.data.cc;
							midi_evt.data.cc.value	 = val & MIDI_CC_MAX;
							event_post(EVENT_CHANNEL_MIDI_OUT, &midi_evt);
							break;
						}

						case MIDI_MODE_CC_14: {
							bool invert = (vmap->range.lower > vmap->range.upper);

							i32 val = convert_range(vmap->curr_pos, vmap->position.start,
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
							midi_evt.data.cc.channel = vmap->proto.midi.channel;
							midi_evt.data.cc.control = vmap->proto.midi.data.cc;
							midi_evt.data.cc.value	 = (val >> 7) & 0x7F;
							event_post(EVENT_CHANNEL_MIDI_OUT, &midi_evt);

							// Then the LSB
							midi_evt.data.cc.control = vmap->proto.midi.data.cc + 32;
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

			vmap = vmap->next;
		} while ((enc->virtmap.mode == VIRTMAP_MODE_OVERLAY) && (vmap != NULL));

		mf_draw_encoder(enc);
	}
}

static int midi_in_handler(void* evt) {
	midi_event_s* midi = (midi_event_s*)evt;

	switch (midi->type) {
		case MIDI_EVENT_CC: {
			for (uint b = 0; b < MF_NUM_ENC_BANKS; b++) {
				for (uint e = 0; e < MF_NUM_ENCODERS; e++) {
					mf_encoder_s* enc	 = &encoders[b][e];
					virtmap_s*		vmap = enc->virtmap.head;

					while (vmap != NULL) {
						// Check if the vmap matches the incoming midi
						if (vmap->proto.type != PROTOCOL_MIDI) {
							goto NEXT;
						} else if (vmap->proto.midi.channel != midi->data.cc.channel) {
							goto NEXT;
						} else if (vmap->proto.midi.data.cc != midi->data.cc.control) {
							goto NEXT;
						}

						u32 timenow = systime_ms();

						// do not update if the encoder is moving.
						if (enc->enc_ctx.velocity != 0) {
							goto NEXT;
						}

						else if ((timenow - vmap->last_update) < config.enc_dead_time) {
							// println_pmem("skipped");
							goto NEXT;
						}

						u16 newpos = convert_range(midi->data.cc.value, vmap->range.lower,
																			 vmap->range.upper, vmap->position.start,
																			 vmap->position.stop);

						vmap->curr_pos = newpos;

					NEXT:
						vmap = vmap->next;
					}
				}
			}

			break;
		}
	}

	return 0;
}

static void print_dir(uint enc_idx, int dir) {
	char										 buf[20]	 = {0};
	static const char* const formatstr = "ed[%d][%d]";
	sprintf(buf, formatstr, enc_idx, dir);
	println(buf);
}
