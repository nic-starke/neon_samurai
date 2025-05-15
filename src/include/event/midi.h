/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"
#include "event/event.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define MIDI_SYSEX_OUT_DATA_LEN_MAX 8

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

extern struct event_channel midi_in_event_ch;
extern struct event_channel midi_out_event_ch;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum midi_event {
	MIDI_EVENT_CC,
	MIDI_EVENT_SYSEX,

	MIDI_EVENT_NB,
};

enum midi_sysex_type {
	SYSEX_TYPE_1BYTE,
	SYSEX_TYPE_END_1BYTE = SYSEX_TYPE_1BYTE,
	SYSEX_TYPE_2BYTE,
	SYSEX_TYPE_3BYTE,
	SYSEX_TYPE_START_3BYTE,
	SYSEX_TYPE_END_2BYTE,
	SYSEX_TYPE_END_3BYTE,

	SYSEX_TYPE_NB,

	SYSEX_TYPE_INVALID = 0xFF,
};

typedef struct __attribute__((packed)) {
	u8 channel;
	u8 control;
	u8 value;
} midi_cc_event_s;

typedef struct __attribute__((packed)) {
	u8 type; // enum midi_sysex_type
	u8 data[3];
} midi_sysex_in_event_s;

typedef struct __attribute__((packed)) {
	u8 cmd;
	u8 param;
	u8 data_len;
	u8 data[MIDI_SYSEX_OUT_DATA_LEN_MAX];
} midi_sysex_out_event_s;

typedef struct __attribute__((packed)) {
	u8 type;
	union {
		midi_cc_event_s				 cc;
		midi_sysex_in_event_s	 sysex_in;
		midi_sysex_out_event_s sysex_out;
	} data;
} midi_event_s;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
