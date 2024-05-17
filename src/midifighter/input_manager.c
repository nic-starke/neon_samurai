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

static int evt_handler_io(void* event);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

EVT_HANDLER(1, io_evt_handler, evt_handler_io);

static mf_encoder_s encoders[MF_NUM_ENC_BANKS][MF_NUM_ENCODERS];

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

int mf_input_init(void) {

	int ret = event_channel_subscribe(EVENT_CHANNEL_IO, &io_evt_handler);
	RETURN_ON_ERR(ret);

	hw_encoder_init();
	sw_encoder_init();

	return 0;
}

int mf_input_update(void) {
	hw_encoder_scan();
	sw_encoder_update();

	return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void sw_encoder_init(void) {
	// Initialise encoder devices and virtual parameter mappings
	midi_cc_e cc = MIDI_CC_MIN;
	for (uint b = 0; b < MF_NUM_ENC_BANKS; b++) {
		for (uint e = 0; e < MF_NUM_ENCODERS; e++) {
			mf_encoder_s* enc = &encoders[b][e];

			enc->display.mode			= DIS_MODE_MULTI_PWM;
			enc->display.virtmode = VIRTMAP_DISPLAY_OVERLAY;

			enc->virtmap.mode = VIRTMAP_MODE_TOGGLE;

			for (uint v = 0; v < MF_NUM_VIRTMAPS_PER_ENC; v++) {
				virtmap_s* map = &enc->virtmap.map[v];

				// const u16 inc				= ENC_RANGE / MF_NUM_VIRTMAPS_PER_ENC;
				// map->position.start = inc * v;
				// map->position.stop	= inc * (v + 1);
				map->position.start = ENC_MIN;
				map->position.stop	= ENC_MAX;
				map->range.lower		= MIDI_CC_MIN;
				map->range.upper		= MIDI_CC_MAX;
				map->next						= NULL;

				map->proto.type					= PROTOCOL_MIDI;
				map->proto.midi.channel = 0;
				map->proto.midi.mode		= MIDI_MODE_CC;
				map->proto.midi.data.cc = cc++;

				virtmap_assign(enc->virtmap.head, map);
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
		int dir = quadrature_direction(&mf_enc_quad[i]);
		encoder_update(&enc_ctx[active_bank][i], dir);
	}
}

static int evt_handler_io(void* event) {
	assert(event);
	io_event_s* e		 = (io_event_s*)event;
	virtmap_s*	vmap = e->dev->vmap;

	// Process the first vmap
	do {
		switch (e->type) {

			case EVT_IO_ENCODER_ROTATION: {
				evt_enc_rotation(e->dev);
				break;
			}

			default: return ERR_BAD_PARAM;
		}

		// Move to the next one
		vmap = vmap->next;

		// But only process the next one if it is not null and overlay mode is
		// active
	} while (vmap != NULL && e->dev->vmap_mode == VIRTMAP_MODE_OVERLAY);

	return 0;
}

static void evt_enc_rotation(input_dev_encoder_s* enc) {
	assert(enc);
	assert(vmap);

	// // Clamp the new value between min and max positions
	// if (dev->vmap_mode == VIRTMAP_MODE_TOGGLE) {
	// 	enc->curr_val =
	// 			CLAMP(newval, dev->vmap->position.start, dev->vmap->position.stop);
	// } else {
	// }
	`

			// If the encoder position is out of the operating range then ignore
			if (!(IN_RANGE(enc->curr_val, vmap->position.start,
										 vmap->position.stop))) {
		return;
	}

	switch (vmap->proto.type) {

		case PROTOCOL_MIDI: {

			switch (vmap->proto.midi.mode) {
				case MIDI_MODE_DISABLED: {
					return;
				}

				case MIDI_MODE_CC: {
					// Convert the encoder range to an 8-bit range after vmapping
					bool invert = (vmap->range.lower > vmap->range.upper);

					i32 val = convert_range(enc->curr_val, vmap->position.start,
																	vmap->position.stop, vmap->range.lower,
																	vmap->range.upper);

					if (invert) {
						val = MIDI_CC_MAX - val;
					}

					if (vmap->curr_value == val) {
						return;
					}

					vmap->prev_value = vmap->curr_value;
					vmap->curr_value = val;

					midi_event_s midi_evt;
					midi_evt.type						 = MIDI_EVENT_CC;
					midi_evt.data.cc.channel = vmap->proto.midi.channel;
					midi_evt.data.cc.control = vmap->proto.midi.data.cc;
					midi_evt.data.cc.value	 = val & MIDI_CC_MAX;
					event_post(EVENT_CHANNEL_MIDI_OUT, &midi_evt);
					return;
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
		default: return;
	}

	return;
}
