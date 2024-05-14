/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <math.h>

#include "core/core_error.h"
#include "core/core_types.h"
#include "core/core_utility.h"
#include "protocol/midi/midi.h"
#include "virtmap/virtmap.h"
#include "io/io_device.h"
#include "event/events_midi.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int process_encoder_rotation(virtmap_s*, iodev_s*);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int (*process_functions[DEV_TYPE_NB])(virtmap_s*, iodev_s* dev) = {
		[DEV_TYPE_ENCODER]				= process_encoder_rotation,
		[DEV_TYPE_ENCODER_SWITCH] = NULL,
		[DEV_TYPE_SWITCH]					= NULL,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int virtmap_update(virtmap_s* vmap, iodev_s* dev) {
	// assert(vmap);
	// assert(dev);

	// return process_functions[dev->type](vmap, (void*)&dev->ctx);
	return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int process_encoder_rotation(virtmap_s* vmap, iodev_s* dev) {
	// assert(vmap);
	// assert(dev);
	// encoder_s* ctx = dev->ctx.encoder;

	// // If the encoder position is out of the operating range then ignore
	// if (!(IN_RANGE(ctx->curr_val, vmap->position.start, vmap->position.stop)))
	// { 	return 0;
	// }

	// // const u16 enc_range = ENC_MAX - ENC_MIN;
	// // const i32 new_range = vmap->range.upper - vmap->range.lower;

	// // f32 val = (((ctx->curr_val - ENC_MIN) * new_range) / enc_range) +
	// vmap->range.lower;

	// switch (vmap->proto.type) {

	// 	case PROTOCOL_MIDI: {

	// 		switch(vmap->proto.midi.mode) {
	// 			case MIDI_MODE_DISABLED: {
	// 				return 0;
	// 			}

	// 			case MIDI_MODE_CC: {
	// 				// Convert the encoder range to an 8-bit range after vmapping
	// 				bool invert = (vmap->range.lower > vmap->range.upper);

	// 				u8 val = (((u32)ctx->curr_val * MIDI_CC_RANGE) / ENC_RANGE);

	// 				if (invert) {
	// 					val = MIDI_CC_MAX - val;
	// 				}

	// 				midi_event_s midi_evt;
	// 				midi_evt.event_id				 = MIDI_EVENT_CC;
	// 				midi_evt.data.cc.channel = vmap->proto.midi.channel;
	// 				midi_evt.data.cc.control = vmap->proto.midi.data.cc;
	// 				midi_evt.data.cc.value	 = val;
	// 				// enc->midi.prev_val			 = val;
	// 				event_post(EVENT_CHANNEL_MIDI_OUT, &midi_evt);
	// 				break;
	// 			}

	// 			case MIDI_MODE_REL_CC: {
	// 				 break;
	// 			}
	// 			case MIDI_MODE_NOTE: {
	// 				 break;
	// 			}
	// 		}

	// 		break;
	// 	}

	// 	case PROTOCOL_OSC: {
	// 		break;
	// 	}

	// 	case PROTOCOL_NONE: return 0;
	// 	default: return ERR_BAD_PARAM;
	// }

	return 0;
}
