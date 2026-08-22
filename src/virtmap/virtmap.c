/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2026) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <assert.h>

#include "virtmap/virtmap.h"
#include "midi/midi.h"
#include "midi/midi_cc.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void vmap_apply_mode_range(struct virtmap* vmap) {
	assert(vmap);

	i16 lower;
	i16 upper;

	switch (vmap->cfg.type) {
		case PROTOCOL_MIDI: {
			switch (vmap->cfg.midi.mode) {
				case MIDI_MODE_CC_14: {
					lower = MIDI_CC_14B_MIN;
					upper = MIDI_CC_14B_MAX;
					break;
				}

				case MIDI_MODE_CC:
				case MIDI_MODE_REL_CC:
				case MIDI_MODE_NOTE: {
					lower = MIDI_CC_MIN;
					upper = MIDI_CC_MAX;
					break;
				}

				case MIDI_MODE_DISABLED:
				default: return;
			}
			break;
		}

		case PROTOCOL_OSC:
		case PROTOCOL_NONE:
		default: return;
	}

	// A descending range means the caller wanted the value inverted; widening
	// it must not silently flip the direction back.
	if (vmap->range.lower > vmap->range.upper) {
		vmap->range.lower = upper;
		vmap->range.upper = lower;
	} else {
		vmap->range.lower = lower;
		vmap->range.upper = upper;
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
