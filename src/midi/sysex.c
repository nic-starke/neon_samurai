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
#include "system/project.h"

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
	size_t len;			 // Size of the destination field in live memory (gENCODERS/...)
	size_t wire_len; // Size of this param's mf_sysex_param_s union member on
										 // the wire (bank_idx/enc_idx/... prefix + data) - used
										 // to validate the received packet length, which is NOT
										 // the same as len above.
};

// Read-only device-info payload for MF_SYSEX_PARAM_DEVICE_INFO, populated
// once below. Lets a host tool detect firmware version/capability without
// hardcoding it - see mf_sysex_device_info_s in sysex.h.
static const mf_sysex_device_info_s device_info = {
		.fw_version_major			 = VERSION_MAJOR,
		.fw_version_minor			 = VERSION_MINOR,
		.fw_version_patch			 = VERSION_PATCH,
		.num_encoders					 = NUM_ENCODERS,
		.num_banks						 = NUM_ENC_BANKS,
		.num_vmaps_per_encoder = NUM_VMAPS_PER_ENC,
		.num_side_switches		 = NUM_SIDE_SWITCHES,
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

// wire_len = the wire-format prefix for this param's category (bank_idx/
// enc_idx/... - fixed per category, NOT sizeof(the whole mf_sysex_*_param_s
// union), which was the earlier, wrong version of this: that gave every
// vmap param the same size as its largest sibling (rgb), rejecting
// legitimately shorter ones (range/position/rb) as too short - plus the
// per-param data length (len, already correct: the destination field's
// real size in live memory, which is also how many data bytes this param
// actually carries on the wire).
#define ENC_PREFIX_LEN	2 // mf_sysex_encoder_param_s: bank_idx + enc_idx
#define VMAP_PREFIX_LEN 3 // mf_sysex_vmap_param_s: bank_idx + enc_idx + vmap_idx
#define SW_PREFIX_LEN		1 // mf_sysex_sideswitch_param_s: sw_idx (data is the len-tracked field)
#define BANK_PREFIX_LEN 0 // mf_sysex_bank_param_s: no prefix, just data

#define SYSEX_DATA_INFO(e, s, v, prefix_len)                                 \
	[e] = {offsetof(s, v), sizeof(((s*)0)->v),                                 \
				 (prefix_len) + sizeof(((s*)0)->v)}

// clang-format off
static const struct sysex_item_data_info sysex_data_info[MF_SYSEX_PARAM_NB] = {
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_ENCODER_DETENT, struct encoder, detent, ENC_PREFIX_LEN),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_ENCODER_DISPLAY_MODE, struct encoder, display.mode, ENC_PREFIX_LEN),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_ENCODER_VMAP_DISPLAY_MODE, struct encoder, display.virtmode, ENC_PREFIX_LEN),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_ENCODER_VMAP_MODE, struct encoder, vmap_mode, ENC_PREFIX_LEN),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_ENCODER_VMAP_ACTIVE, struct encoder, vmap_active, ENC_PREFIX_LEN),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_ENCODER_SWITCH_STATE, struct encoder, sw_state, ENC_PREFIX_LEN),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_ENCODER_SWITCH_MODE, struct encoder, sw_mode, ENC_PREFIX_LEN),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_ENCODER_SWITCH_PROTO, struct encoder, sw_cfg, ENC_PREFIX_LEN),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_VMAP_RANGE, struct virtmap, range, VMAP_PREFIX_LEN),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_VMAP_POSITION, struct virtmap, position, VMAP_PREFIX_LEN),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_VMAP_RGB, struct virtmap, rgb, VMAP_PREFIX_LEN),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_VMAP_RB, struct virtmap, rb, VMAP_PREFIX_LEN),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_VMAP_PROTO, struct virtmap, cfg, VMAP_PREFIX_LEN),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_SIDE_SWITCH, struct side_switch, mode, SW_PREFIX_LEN),
	SYSEX_DATA_INFO(MF_SYSEX_PARAM_ACTIVE_BANK, struct mf_rt, curr_bank, BANK_PREFIX_LEN),
	[MF_SYSEX_PARAM_DEVICE_INFO] = {0, sizeof(mf_sysex_device_info_s), 0},
};

// clang-format on

static enum stream_state stream_state = STREAM_IDLE;
// Buffer to stream incoming sysex, +2 for start and end sysex bytes
static u8								 buffer[MF_SYSEX_MAX_PKT_SIZE + 2];
static u8								 buffer_idx = 0;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int mf_sysex_init(void) {
	return event_channel_subscribe(EVENT_CHANNEL_MIDI_IN, &evt_midi);
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

	// Bound against the buffer's actual capacity (sizeof(buffer), which
	// includes the F0/F7 framing bytes) rather than MF_SYSEX_MAX_PKT_SIZE
	// (mf_id+cmd+param_enum+data only, no framing) - the latter is 2 bytes
	// too small and previously caused any message using the largest wire
	// param variant (mf_sysex_vmap_param_s, 9 bytes - VMAP_RANGE/POSITION/
	// RGB/RB/PROTO) to be silently dropped with ERR_NO_MEM before the F7
	// terminator byte even arrived.
	if (buffer_idx + len > sizeof(buffer)) {
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

	// Check if sysex start and end bytes are correct
	if (buffer[0] != MIDI_STATUS_SYSTEM_EXCLUSIVE ||
			buffer[buffer_idx - 1] != MIDI_STATUS_END_OF_EXCLUSIVE) {
		ret = ERR_BAD_MSG;
		goto cleanup;
	}

	// Validate the manufacturer ID is correct (SAM). Reject on any byte
	// mismatch - using && here would only reject if all three bytes were
	// simultaneously wrong, which accepts almost anything.
	if (msg->mf_id[0] != MIDI_MFR_ID_1 || msg->mf_id[1] != MIDI_MFR_ID_2 ||
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

	// Check the received packet size matches what this parameter expects -
	// this was previously computed but never actually checked, so an
	// undersized data field would fall through to memcpy() below and read
	// past the message. buffer_idx counts the F0/F7 framing bytes too (see
	// the streaming loop above), so the expected total is: F0(1) + mf_id(3)
	// + cmd(1) + param_enum(1) + this param's wire struct + F7(1).
	u8 expected_len = 1 + 3 + 1 + 1 + sysex_data_info[msg->param_enum].wire_len + 1;
	if (buffer_idx != expected_len) {
		ret = ERR_BAD_MSG;
		goto cleanup;
	}

	// Resolve the live-memory location and length of the target parameter.
	// Both GET (read out) and SET (write in) operate through this pointer;
	// the two commands are distinguished further down. Side-switch and
	// active-bank params are resolved directly on gSIDE_SWITCHES/gRT rather
	// than through gENCODERS.
	void* param			= NULL;
	u8		param_len = sysex_data_info[msg->param_enum].len;

	switch (msg->param_enum) {
		case MF_SYSEX_PARAM_ENCODER_DETENT:
		case MF_SYSEX_PARAM_ENCODER_DISPLAY_MODE:
		case MF_SYSEX_PARAM_ENCODER_VMAP_DISPLAY_MODE:
		case MF_SYSEX_PARAM_ENCODER_VMAP_MODE:
		case MF_SYSEX_PARAM_ENCODER_VMAP_ACTIVE:
		case MF_SYSEX_PARAM_ENCODER_SWITCH_STATE:
		case MF_SYSEX_PARAM_ENCODER_SWITCH_MODE:
		case MF_SYSEX_PARAM_ENCODER_SWITCH_PROTO: {
			u8 bank = msg->param.enc.bank_idx;
			u8 enc	= msg->param.enc.enc_idx;
			if (bank >= NUM_ENC_BANKS || enc >= NUM_ENCODERS) {
				ret = ERR_BAD_PARAM;
				break;
			}
			struct encoder* encoder = &gENCODERS[bank][enc];
			param =
					(void*)((u8*)encoder + sysex_data_info[msg->param_enum].offset);
			if (msg->cmd == MF_SYSEX_SET) {
				memcpy(param, (const void*)&msg->param.enc.data, param_len);
			}
			break;
		}

		case MF_SYSEX_PARAM_VMAP_RANGE:
		case MF_SYSEX_PARAM_VMAP_POSITION:
		case MF_SYSEX_PARAM_VMAP_RGB:
		case MF_SYSEX_PARAM_VMAP_RB:
		case MF_SYSEX_PARAM_VMAP_PROTO: {
			u8 bank_idx = msg->param.vmap.bank_idx;
			u8 enc_idx	= msg->param.vmap.enc_idx;
			u8 vmap_idx = msg->param.vmap.vmap_idx;
			if (bank_idx >= NUM_ENC_BANKS || enc_idx >= NUM_ENCODERS ||
					vmap_idx >= NUM_VMAPS_PER_ENC) {
				ret = ERR_BAD_PARAM;
				break;
			}
			struct virtmap* vmap = &gENCODERS[bank_idx][enc_idx].vmaps[vmap_idx];
			param = (void*)((u8*)vmap + sysex_data_info[msg->param_enum].offset);
			if (msg->cmd == MF_SYSEX_SET) {
				memcpy(param, (const void*)&msg->param.vmap.data, param_len);
			}
			break;
		}

		case MF_SYSEX_PARAM_SIDE_SWITCH: {
			u8 sw_idx = msg->param.sw.sw_idx;
			if (sw_idx >= NUM_SIDE_SWITCHES) {
				ret = ERR_BAD_PARAM;
				break;
			}
			struct side_switch* sw = &gSIDE_SWITCHES[sw_idx];
			param = (void*)((u8*)sw + sysex_data_info[msg->param_enum].offset);
			if (msg->cmd == MF_SYSEX_SET) {
				memcpy(param, (const void*)&msg->param.sw.data, param_len);
			}
			break;
		}

		case MF_SYSEX_PARAM_ACTIVE_BANK: {
			// Reject an out-of-range bank on SET before it's applied - gRT.curr_bank
			// indexes gENCODERS[bank][...] everywhere else in the firmware, so an
			// invalid value here would corrupt encoder lookups system-wide.
			if (msg->cmd == MF_SYSEX_SET && msg->param.bank.data >= NUM_ENC_BANKS) {
				ret = ERR_BAD_PARAM;
				break;
			}
			param = (void*)((u8*)&gRT + sysex_data_info[msg->param_enum].offset);
			if (msg->cmd == MF_SYSEX_SET) {
				memcpy(param, (const void*)&msg->param.bank.data, param_len);
			}
			break;
		}

		case MF_SYSEX_PARAM_DEVICE_INFO: {
			// Read-only - reject any attempt to write it. The cast away from
			// const is safe here: this case can only reach the GET reply path
			// below (SET returns ERR_UNSUPPORTED above), which only reads
			// through param, never writes.
			if (msg->cmd == MF_SYSEX_SET) {
				ret = ERR_UNSUPPORTED;
				break;
			}
			param = (void*)(uintptr_t)&device_info;
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
									.data_len = param_len,
							},
			};
			memcpy(reply.data.sysex_out.data, param, param_len);
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
									.data			= {ret},
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
