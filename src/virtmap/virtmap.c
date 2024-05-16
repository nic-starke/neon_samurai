/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "virtmap/virtmap.h"
#include "core/core_error.h"
#include "event/event.h"
#include "event/events_io.h"
#include "event/events_midi.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int	evt_handler_io(void* event);
static void evt_enc_rotation(encoder_s* enc, virtmap_s* vmap);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

EVT_HANDLER(1, io_evt_handler, evt_handler_io);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int virtmap_manager_init(void) {

	int ret = event_channel_subscribe(EVENT_CHANNEL_IO, &io_evt_handler);
	RETURN_ON_ERR(ret);

	return 0;
}

int virtmap_assign(virtmap_s* vmap, iodev_s* dev) {
	assert(vmap);
	assert(dev);

	// If nothing assigned yet then assign.
	if (dev->vmap == NULL) {
		dev->vmap = vmap;
		return 0;
	}

	virtmap_s* v = dev->vmap;

	// Traverse the linked list of vmaps..
	while (v->next != NULL) {
		v = v->next;
	}

	// Assign to first empty slot
	v->next = vmap;
	return 0;
}

int virtmap_toggle(virtmap_s* vmap) {
	assert(vmap);

	virtmap_s* first = vmap;			 // first item in linked list
	virtmap_s* next	 = vmap->next; // second item in linked list
	first->next			 = NULL;			 // disconnect first and second
	vmap						 = next;			 // set original pointer to second item

	// find the end of the list
	while (next->next != NULL) {
		next = next->next;
	}

	// reconnect first item at end of the list
	next->next = first;

	return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int evt_handler_io(void* event) {
	assert(event);
	io_event_s* e		 = (io_event_s*)event;
	virtmap_s*	vmap = e->dev->vmap;

	// Process the first vmap
	do {
		switch (e->type) {

			case EVT_IO_ENCODER_ROTATION: {
				evt_enc_rotation((encoder_s*)e->dev->ctx, vmap);
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

static void evt_enc_rotation(encoder_s* enc, virtmap_s* vmap) {
	assert(enc);
	assert(vmap);

	// If the encoder position is out of the operating range then ignore
	if (!(IN_RANGE(enc->curr_val, vmap->position.start, vmap->position.stop))) {
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
