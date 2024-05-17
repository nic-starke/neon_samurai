/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "sys/error.h"
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

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

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
			enc->virtmap.head			= NULL;

			for (uint v = 0; v < MF_NUM_VMAPS_PER_ENC; v++) {
				virtmap_s* map = &vmaps[b][e][v];

				map->position.start			= ENC_MIN;
				map->position.stop			= ENC_MAX;
				map->range.lower				= MIDI_CC_MIN;
				map->range.upper				= MIDI_CC_MAX;
				map->proto.type					= PROTOCOL_MIDI;
				map->proto.midi.channel = 0;
				map->proto.midi.mode		= MIDI_MODE_CC;
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

static void sw_encoder_update(void) {
	for (uint i = 0; i < MF_NUM_ENCODERS; i++) {
		mf_encoder_s* enc		= &encoders[active_bank][i];
		int						dir		= quadrature_direction(enc->quad_ctx);
		bool					moved = encoder_update(&enc->enc_ctx, dir);

		if (moved == false)
			continue;

		virtmap_s* vmap = enc->virtmap.head;

		// If vmap mode is "toggle" then this means that only 1 vmap is currently
		// active care must be taken to ensure that the raw encoder position is not
		// outside the range of the vmap, therefore it is necessary to clamp the
		// encoder position to the range of the vmap

		if (enc->virtmap.mode == VIRTMAP_MODE_TOGGLE) {
			encoder_clamp(&enc->enc_ctx, vmap->position.start, vmap->position.stop);
		}

		do {

			// If the encoder is in overlay mode then that means that multiple vmaps
			// are active. They may operate in different arcs, therefore we check if
			// the encoder is in the operating arc of the current vmap, and skip this
			// vmap if the encoder is outside this range.
			if (!(IN_RANGE(enc->enc_ctx.curr_val, vmap->position.start,
										 vmap->position.stop))) {
				continue;
			}

			switch (vmap->proto.type) {

				case PROTOCOL_MIDI: {
					switch (vmap->proto.midi.mode) {
						case MIDI_MODE_DISABLED: {
							break;
						}

						case MIDI_MODE_CC: {
							// Convert the encoder range to an 8-bit range after vmapping
							bool invert = (vmap->range.lower > vmap->range.upper);

							i32 val = convert_range(enc->enc_ctx.curr_val,
																			vmap->position.start, vmap->position.stop,
																			vmap->range.lower, vmap->range.upper);

							if (invert) {
								val = MIDI_CC_MAX - val;
							}

							if (vmap->curr_value == val) {
								break;
							}

							vmap->prev_value = vmap->curr_value;
							vmap->curr_value = val;

							midi_event_s midi_evt;
							midi_evt.type						 = MIDI_EVENT_CC;
							midi_evt.data.cc.channel = vmap->proto.midi.channel;
							midi_evt.data.cc.control = vmap->proto.midi.data.cc;
							midi_evt.data.cc.value	 = val & MIDI_CC_MAX;
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
