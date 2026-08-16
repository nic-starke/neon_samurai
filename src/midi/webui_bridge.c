/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2026) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "midi/webui_bridge.h"
#include "event/event.h"
#include "event/io.h"
#include "midi/sysex.h"
#include "system/hardware.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int io_event_handler(void* evt);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

EVT_HANDLER(1, evt_io, io_event_handler);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int webui_bridge_init(void) {
	return event_channel_subscribe(EVENT_CHANNEL_IO, &evt_io);
}

void webui_bridge_set_streaming(bool enabled) {
	gRT.live_position_streaming = enabled;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int io_event_handler(void* evt) {
	struct io_event* io = (struct io_event*)evt;

	switch (io->type) {
		case EVT_IO_ENCODER_FIELD_CHANGED: {
			switch (io->field) {
				case IO_FIELD_VMAP_ACTIVE: {
					sysex_push_vmap_active(io->bank, io->enc, io->value);
					break;
				}

				default: break;
			}
			break;
		}

		default: break;
	}

	return 0;
}
