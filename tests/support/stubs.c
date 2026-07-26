/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2026) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*
	Fakes for the firmware globals and event-system entry points that modules
	under test depend on.

	The sysex parser needs exactly three: gENCODERS, event_post() and
	event_channel_subscribe(). Posted events are recorded so tests can assert on
	what the parser sent back to the host.
*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string.h>

#include "support/stubs.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Globals ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct encoder gENCODERS[NUM_ENC_BANKS][NUM_ENCODERS];

int										 stub_posted_count		 = 0;
int										 stub_last_post_channel = -1;
midi_sysex_out_event_s stub_last_sysex				 = {0};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void stub_reset(void) {
	stub_posted_count			 = 0;
	stub_last_post_channel = -1;
	memset(&stub_last_sysex, 0, sizeof(stub_last_sysex));
}

int event_post(enum event_ch ch, void* event) {
	stub_posted_count++;
	stub_last_post_channel = (int)ch;

	if (ch == EVENT_CHANNEL_MIDI_OUT && event != NULL) {
		const midi_event_s* e = (const midi_event_s*)event;
		if (e->type == MIDI_EVENT_SYSEX) {
			stub_last_sysex = e->data.sysex_out;
		}
	}

	return 0;
}

int event_channel_subscribe(enum event_ch						 ch,
														struct event_ch_handler* new_handler) {
	(void)ch;
	(void)new_handler;
	return 0;
}
