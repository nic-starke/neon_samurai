/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "sys/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
	MIDI_MSG_CHANNEL_VOICE,
	MIDI_MSG_CHANNEL_MODE,
	MIDI_MSG_SYSTEM_EXCLUSIVE,
	MIDI_MSG_SYSTEM_COMMON,
	MIDI_MSG_SYSTEM_REALTIME,
	MIDI_MSG_INVALID,

	MIDI_MSG_NB,
} midi_msg_type_e;

// Status byte values for various message types
typedef enum {
	MIDI_STATUS_NOTE_OFF				= 0x80,
	MIDI_STATUS_NOTE_ON					= 0x90,
	MIDI_STATUS_POLY_KEY_PRESSURE		= 0xA0,
	MIDI_STATUS_CONTROL_CHANGE			= 0xB0,
	MIDI_STATUS_PROGRAM_CHANGE			= 0xC0,
	MIDI_STATUS_CHANNEL_PRESSURE		= 0xD0,
	MIDI_STATUS_PITCH_BEND				= 0xE0,
	MIDI_STATUS_SYSTEM_EXCLUSIVE		= 0xF0,
	MIDI_STATUS_TIME_CODE_QUARTER_FRAME = 0xF1,
	MIDI_STATUS_SONG_POSITION_POINTER	= 0xF2,
	MIDI_STATUS_SONG_SELECT				= 0xF3,
	MIDI_STATUS_TUNE_REQUEST			= 0xF6,
	MIDI_STATUS_END_OF_EXCLUSIVE		= 0xF7,
	MIDI_STATUS_TIMING_CLOCK			= 0xF8,
	MIDI_STATUS_START					= 0xFA,
	MIDI_STATUS_CONTINUE				= 0xFB,
	MIDI_STATUS_STOP					= 0xFC,
	MIDI_STATUS_ACTIVE_SENSING			= 0xFE,
	MIDI_STATUS_SYSTEM_RESET			= 0xFF,
} midi_status_byte_e;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
