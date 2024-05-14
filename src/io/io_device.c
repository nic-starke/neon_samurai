/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "core/core_error.h"
#include "io/io_device.h"
#include "io/encoder/encoder_quadrature.h"
#include "protocol/midi/midi.h"
#include "event/event.h"
#include "event/events_midi.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int process_encoder_rotation(iodev_s* dev, virtmap_s* vmap);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static iodev_s* devices[DEV_TYPE_NB] = {0};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int iodev_init(iodev_type_e type, iodev_s* dev, void* ctx, uint index) {
	assert(dev);
	assert(ctx);

	if (type >= DEV_TYPE_NB) {
		return ERR_BAD_PARAM;
	}

	dev->ctx	= ctx;
	dev->idx	= index;
	dev->vmap = NULL;

	switch (type) {
		case DEV_TYPE_ENCODER: {
			return encoder_init(ctx);
		}

		case DEV_TYPE_ENCODER_SWITCH: {
			break;
		}

		case DEV_TYPE_SWITCH: {
			break;
		}

		default: return ERR_BAD_PARAM;
	}

	return 0;
}

int iodev_register_arr(iodev_type_e type, iodev_s* arr) {
	assert(arr);

	if (type >= DEV_TYPE_NB) {
		return ERR_BAD_PARAM;
	}

	// Check if this device type was already registered.
	if (devices[type] != NULL) {
		return ERR_DUPLICATE;
	}

	devices[type] = arr;

	return 0;
}

int iodev_assign_virtmap(iodev_s* dev, virtmap_s* virtmap) {
	assert(dev);
	assert(virtmap);

	// If nothing assigned yet then assign.
	if (dev->vmap == NULL) {
		dev->vmap = virtmap;
		return 0;
	}

	virtmap_s* v = dev->vmap;

	// Traverse the linked list of vmaps..
	while (v->next != NULL) {
		v = v->next;
	}

	// Assign to first empty slot
	v->next = virtmap;
	return 0;
}

int iodev_update(iodev_type_e type, iodev_s* dev) {
	assert(dev);
	assert(dev.vmap);

	switch (type) {
		case DEV_TYPE_ENCODER: {

			process_encoder_rotation(dev, dev->vmap);

			virtmap_s* v = dev->vmap->next;

			while (v != NULL) {
				process_encoder_rotation(dev, v);
				v = v->next;
			}
		}
		case DEV_TYPE_ENCODER_SWITCH: {
			break;
		}
		case DEV_TYPE_SWITCH: {
			break;
		}

		default: return ERR_BAD_PARAM;
	}

	return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int process_encoder_rotation(iodev_s* dev, virtmap_s* vmap) {
	assert(dev);
	assert(vmap);

	encoder_s* ctx = (encoder_s*)dev->ctx;

	// If the encoder position is out of the operating range then ignore
	if (!(IN_RANGE(ctx->curr_val, vmap->position.start, vmap->position.stop))) {
		return 0;
	}

	// const u16 enc_range = ENC_MAX - ENC_MIN;
	// const i32 new_range = vmap->range.upper - vmap->range.lower;

	// f32 val = (((ctx->curr_val - ENC_MIN) * new_range) / enc_range) +
	// vmap->range.lower;

	switch (vmap->proto.type) {

		case PROTOCOL_MIDI: {

			switch (vmap->proto.midi.mode) {
				case MIDI_MODE_DISABLED: {
					return 0;
				}

				case MIDI_MODE_CC: {
					// Convert the encoder range to an 8-bit range after vmapping
					bool invert = (vmap->range.lower > vmap->range.upper);

					u8 val = (((u32)ctx->curr_val * MIDI_CC_RANGE) / ENC_RANGE);

					if (invert) {
						val = MIDI_CC_MAX - val;
					}

					if (val == vmap->proto.midi.prev_val) {
						return 0;
					}

					vmap->proto.midi.prev_val = val;
					midi_event_s midi_evt;
					midi_evt.type						 = MIDI_EVENT_CC;
					midi_evt.data.cc.channel = vmap->proto.midi.channel;
					midi_evt.data.cc.control = vmap->proto.midi.data.cc;
					midi_evt.data.cc.value	 = val & MIDI_CC_MAX;
					return event_post(EVENT_CHANNEL_MIDI_OUT, &midi_evt);
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

		case PROTOCOL_NONE: return 0;
		default: return ERR_BAD_PARAM;
	}

	return 0;
}
