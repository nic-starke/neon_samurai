/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "core/core_error.h"
#include "virtmap/virtmanager.h"
#include "virtmap/virtmap.h"

#include "event/event.h"
#include "event/events_io.h"
#include "event/events_midi.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void evt_handler_io(void* event);
static void evt_handler_midi(void* event);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

EVT_HANDLER(1, io_evt_handler, evt_handler_io);
EVT_HANDLER(1, midi_in_evt_handler, evt_handler_midi);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int virtmanager_init(void) {
	int ret = event_channel_subscribe(EVENT_CHANNEL_IO, &io_evt_handler);
	RETURN_ON_ERR(ret);

	ret = event_channel_subscribe(EVENT_CHANNEL_MIDI_IN, &midi_in_evt_handler);
	RETURN_ON_ERR(ret);

	return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void evt_handler_io(void* event) {
	assert(event);

	io_event_s* e = (io_event_s*)event;

	switch (e->event_id) {

		case EVT_IO_ENCODER_ROTATION: break;

		case EVT_IO_ENCODER_SWITCH: break;

		case EVT_IO_BUTTON: break;

		default: return;
	}
}

static void evt_handler_midi(void* event) {
	assert(event);

	midi_event_s* e = (midi_event_s*)event;

	switch (e->event_id) {

		case MIDI_EVENT_CC: break;

		default: return;
	}
}
