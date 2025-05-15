/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string.h>

#include "midi/sysex.h"
#include "event/midi.h"

// Test sequence:
// [sysex start] [mfid] [cmd] [param] [data] [sysex end]
// 1 byte        3 bytes 1 byte 1 byte  1 byte  1 byte
// Test sequence to disable detent for enc[0][0] is:
// f0 53 41 4d 02 00 00 00 00 f7
// [header] 	 	[set] [param] [bank_idx] [enc_idx] [val] [footer]
// f0 53 41 4d   02    00     00  				00 				00 			f7
// Test sequence to enable detent for enc[0][0] is:
// f0 53 41 4d 02 00 00 00 01 f7

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum stream_state {
	STREAM_IDLE,
	STREAM_RECEIVING,
	STREAM_COMPLETE,
	STREAM_ERROR,

	STREAM_NB,
};

struct sysex_type_streamer_def {
	u8								len_data;
	enum stream_state next_state;
};

struct sysex_item_data_info {
	size_t offset;
	size_t len;
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int midi_in_handler(void* evt);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

EVT_HANDLER(2, evt_midi, midi_in_handler);

static u8 sysex_data_len[SYSEX_TYPE_NB] = {
		[SYSEX_TYPE_1BYTE] = 1,			[SYSEX_TYPE_2BYTE] = 2,
		[SYSEX_TYPE_3BYTE] = 3,			[SYSEX_TYPE_START_3BYTE] = 3,
		[SYSEX_TYPE_END_2BYTE] = 2, [SYSEX_TYPE_END_3BYTE] = 3,
};

static const enum stream_state sysex_next_state[SYSEX_TYPE_NB] = {
		[SYSEX_TYPE_1BYTE]			 = STREAM_COMPLETE,
		[SYSEX_TYPE_2BYTE]			 = STREAM_COMPLETE,
		[SYSEX_TYPE_3BYTE]			 = STREAM_COMPLETE,
		[SYSEX_TYPE_START_3BYTE] = STREAM_RECEIVING,
		[SYSEX_TYPE_END_2BYTE]	 = STREAM_COMPLETE,
		[SYSEX_TYPE_END_3BYTE]	 = STREAM_COMPLETE,
};

#define SYSEX_DATA_INFO(e, s, v) [e] = {offsetof(s, v), sizeof(((s*)0)->v)}

// clang-format off
static const struct sysex_item_data_info sysex_data_info[MF_SYSEX_PARAM_NB] = {
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_ENCODER_DETENT, struct encoder, detent),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_ENCODER_DISPLAY_MODE, struct encoder, display.mode),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_ENCODER_VMAP_DISPLAY_MODE, struct encoder, display.virtmode),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_ENCODER_VMAP_MODE, struct encoder, vmap_mode),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_ENCODER_VMAP_ACTIVE, struct encoder, vmap_active),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_ENCODER_SWITCH_STATE, struct encoder, sw_state),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_ENCODER_SWITCH_MODE, struct encoder, sw_mode),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_ENCODER_SWITCH_PROTO, struct encoder, sw_cfg),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_VMAP_RANGE, struct virtmap, range),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_VMAP_POSITION, struct virtmap, position),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_VMAP_RGB, struct virtmap, rgb),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_VMAP_RB, struct virtmap, rb),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_VMAP_PROTO, struct virtmap, cfg),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_SIDE_SWITCH, struct mf_rt, curr_bank),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_ACTIVE_BANK, struct mf_rt, curr_bank),
};

// clang-format on

static enum stream_state stream_state = STREAM_IDLE;
// Buffer to stream incoming sysex, +2 for start and end sysex bytes
static u8								 buffer[MF_SYSEX_MAX_PKT_SIZE + 2];
static u8								 buffer_idx = 0;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int mf_sysex_init(void) {
	event_channel_subscribe(EVENT_CHANNEL_MIDI_IN, &evt_midi);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int midi_in_handler(void* evt) {
	int						ret	 = 0;
	midi_event_s* midi = (midi_event_s*)evt;

	if (midi->type != MIDI_EVENT_SYSEX) {
		return 0; // Ignore non-sysex events
	} else if (midi->data.sysex_in.type == SYSEX_TYPE_INVALID) {
		ret = ERR_BAD_PARAM;
		goto cleanup;
	}

	// Get the number of bytes in the sysex packet
	u8 len = sysex_data_len[midi->data.sysex_in.type];

	if (buffer_idx + len > MF_SYSEX_MAX_PKT_SIZE) {
		ret = ERR_NO_MEM;
		goto cleanup;
	}

	// Copy the sysex data into the streaming buffer
	for (int i = 0; i < len; i++) {
		buffer[buffer_idx++] = midi->data.sysex_in.data[i];
	}

	// Update the stream state based on the sysex message type
	stream_state = sysex_next_state[midi->data.sysex_in.type];

	if (stream_state != STREAM_COMPLETE) {
		return 0;
	}

	// If streaming is complete then we have a message in the buffer to process.
	const mf_sysex_msg_s* msg = (mf_sysex_msg_s*)&buffer[1];

	// Validate that the received data conforms
	// Check if the number of bytes received is too small
	if (buffer_idx < MF_SYSEX_MIN_PKT_SIZE) {
		ret = ERR_BAD_MSG;
		goto cleanup;
	}

	// Check the expected length based on the parameter enum data length
	u8 expected_len =
			MF_SYSEX_MIN_PKT_SIZE + sysex_data_info[msg->param_enum].len;

	switch (msg->param_enum) {}

	// Check if sysex start and end bytes are correct
	if (buffer[0] != MIDI_STATUS_SYSTEM_EXCLUSIVE ||
			buffer[buffer_idx - 1] != MIDI_STATUS_END_OF_EXCLUSIVE) {
		ret = ERR_BAD_MSG;
		goto cleanup;
	}

	// Validate the manufacturer ID is correct (SAM)
	if (msg->mf_id[0] != MIDI_MFR_ID_1 && msg->mf_id[1] != MIDI_MFR_ID_2 &&
			msg->mf_id[2] != MIDI_MFR_ID_3) {
		ret = ERR_BAD_MSG;
		goto cleanup;
	}

	// Check the command is valid
	if (msg->cmd != MF_SYSEX_GET && msg->cmd != MF_SYSEX_SET &&
			msg->cmd != MF_SYSEX_STOP) {
		ret = ERR_BAD_MSG;
		goto cleanup;
	}

	// Check the parameter is valid
	if (msg->param_enum >= MF_SYSEX_PARAM_NB) {
		ret = ERR_BAD_PARAM;
		goto cleanup;
	}

	switch (msg->param_enum) {
		case MF_SYSEX_PARAM_ENCODER_DETENT:
		case MF_SYSEX_PARAM_ENCODER_DISPLAY_MODE:
		case MF_SYSEX_PARAM_ENCODER_VMAP_DISPLAY_MODE:
		case MF_SYSEX_PARAM_ENCODER_VMAP_MODE:
		case MF_SYSEX_PARAM_ENCODER_VMAP_ACTIVE:
		case MF_SYSEX_PARAM_ENCODER_SWITCH_STATE:
		case MF_SYSEX_PARAM_ENCODER_SWITCH_MODE:
		case MF_SYSEX_PARAM_ENCODER_SWITCH_PROTO: {
			u8							bank		= msg->param.enc.bank_idx;
			u8							enc			= msg->param.enc.enc_idx;
			struct encoder* encoder = &gENCODERS[bank][enc];
			void*						param =
					(void*)((u8*)encoder + sysex_data_info[msg->param_enum].offset);
			memcpy(param, (const void*)&msg->param.enc.data,
						 sysex_data_info[msg->param_enum].len);
			break;
		}

		case MF_SYSEX_PARAM_VMAP_RANGE:
		case MF_SYSEX_PARAM_VMAP_POSITION:
		case MF_SYSEX_PARAM_VMAP_RGB:
		case MF_SYSEX_PARAM_VMAP_RB:
		case MF_SYSEX_PARAM_VMAP_PROTO: {
			u8							bank_idx = msg->param.vmap.bank_idx;
			u8							enc_idx	 = msg->param.vmap.enc_idx;
			u8							vmap_idx = msg->param.vmap.vmap_idx;
			struct virtmap* vmap		 = &gENCODERS[bank_idx][enc_idx].vmaps[vmap_idx];
			void*						param =
					(void*)((u8*)vmap + sysex_data_info[msg->param_enum].offset);
			memcpy(param, (const void*)&msg->param.vmap.data,
						 sysex_data_info[msg->param_enum].len);
			break;
		}

		case MF_SYSEX_PARAM_SIDE_SWITCH: {
			break;
		}

		case MF_SYSEX_PARAM_ACTIVE_BANK: {
			break;
		}

		default: {
			ret = ERR_BAD_PARAM;
		}
	}

	if (ret != 0) {
		goto cleanup;
	}

	switch (msg->cmd) {
		case MF_SYSEX_GET: {
			midi_event_s reply = {
					.type = MIDI_EVENT_SYSEX,
					.data.sysex_out =
							{
									.cmd			= MF_SYSEX_GET_RESPONSE,
									.param		= msg->param_enum,
									.data_len = 1,
									.data			= ret,
							},
			};
			event_post(EVENT_CHANNEL_MIDI_OUT, &reply);
			break;
		}

		case MF_SYSEX_SET: {
			midi_event_s reply = {
					.type = MIDI_EVENT_SYSEX,
					.data.sysex_out =
							{
									.cmd			= MF_SYSEX_SET_RESPONSE,
									.param		= msg->param_enum,
									.data_len = 1,
									.data			= ret,
							},
			};
			event_post(EVENT_CHANNEL_MIDI_OUT, &reply);
			break;
		}

		default: {
			ret = ERR_BAD_MSG;
			goto cleanup;
		}
	}

cleanup:
	buffer_idx = 0;
	memset(buffer, 0, sizeof(buffer));
	stream_state = STREAM_IDLE;
	return ret;
}
