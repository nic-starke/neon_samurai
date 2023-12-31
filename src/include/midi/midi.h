/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "core/core_types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
	MIDI_MODE_DISABLED,
	MIDI_MODE_CC,
	MIDI_MODE_REL_CC,
	MIDI_MODE_NOTE,
} midi_mode_e;

typedef struct {
	midi_mode_e mode;
	u8					channel;
	union {
		u8 cc;
	} data;
	u16 prev_val;
} midi_device_s;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */

int midi_init(void);
int midi_update(void);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
