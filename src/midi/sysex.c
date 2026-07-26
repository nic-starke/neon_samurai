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

static u8							 sysex_param_index_len(u8 param_enum);
static struct encoder* encoder_from_indices(u8 bank, u8 enc);
static struct virtmap* virtmap_from_indices(u8 bank, u8 enc, u8 vmap);

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

	// Bound against the real capacity of the streaming buffer. This used to
	// compare against MF_SYSEX_MAX_PKT_SIZE, which excludes the F0/F7 framing
	// bytes the buffer also holds, so the largest legal message could never be
	// received.
	if ((uint)buffer_idx + len > sizeof(buffer)) {
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

	/*
		Streaming is complete, so there is a message in the buffer to process.

		Validation order matters here and is deliberate: every check below is
		ordered so that nothing is dereferenced or used as an index before it has
		been bounds-checked. The previous version read
		sysex_data_info[msg->param_enum] roughly 25 lines before checking that
		param_enum was in range, and indexed gENCODERS with wire-supplied bank and
		encoder values that were never checked at all.
	*/

	// The smallest valid frame is F0 + header + F7. Check this before treating
	// the buffer as a message at all.
	if (buffer_idx < (MF_SYSEX_MIN_PKT_SIZE + 2)) {
		ret = ERR_BAD_MSG;
		goto cleanup;
	}

	// Check sysex start and end framing bytes.
	if (buffer[0] != MIDI_STATUS_SYSTEM_EXCLUSIVE ||
			buffer[buffer_idx - 1] != MIDI_STATUS_END_OF_EXCLUSIVE) {
		ret = ERR_BAD_MSG;
		goto cleanup;
	}

	const mf_sysex_msg_s* msg = (const mf_sysex_msg_s*)&buffer[1];

	/*
		Validate the manufacturer ID (SAM).

		This previously used && between the three byte comparisons, so a message
		was only rejected when all three bytes were wrong - any sysex sharing a
		single byte with "SAM" was accepted and processed.
	*/
	if (msg->mf_id[0] != MIDI_MFR_ID_1 || msg->mf_id[1] != MIDI_MFR_ID_2 ||
			msg->mf_id[2] != MIDI_MFR_ID_3) {
		ret = ERR_BAD_MSG;
		goto cleanup;
	}

	// Check the command is valid.
	if (msg->cmd != MF_SYSEX_GET && msg->cmd != MF_SYSEX_SET &&
			msg->cmd != MF_SYSEX_STOP) {
		ret = ERR_BAD_MSG;
		goto cleanup;
	}

	// Check the parameter is in range BEFORE it is used to index sysex_data_info.
	if (msg->param_enum >= MF_SYSEX_PARAM_NB) {
		ret = ERR_BAD_PARAM;
		goto cleanup;
	}

	/*
		Now that param_enum is known good, check that the frame is exactly as long
		as this parameter requires: F0 + header + index prefix + payload + F7.
		This is the length validation the unused `expected_len` variable was
		reaching for.
	*/
	const u8 idx_len			= sysex_param_index_len(msg->param_enum);
	const u8 expected_len = (u8)(1 + MF_SYSEX_MIN_PKT_SIZE + idx_len +
															 sysex_data_info[msg->param_enum].len + 1);

	if (buffer_idx != expected_len) {
		ret = ERR_BAD_MSG;
		goto cleanup;
	}

	/*
		Resolved location of the addressed parameter. For SET this is the
		destination; for GET it is the source read back into the response.
	*/
	u8*			 param		 = NULL;
	const u8 param_len = sysex_data_info[msg->param_enum].len;

	switch (msg->param_enum) {
		case MF_SYSEX_PARAM_ENCODER_DETENT:
		case MF_SYSEX_PARAM_ENCODER_DISPLAY_MODE:
		case MF_SYSEX_PARAM_ENCODER_VMAP_DISPLAY_MODE:
		case MF_SYSEX_PARAM_ENCODER_VMAP_MODE:
		case MF_SYSEX_PARAM_ENCODER_VMAP_ACTIVE:
		case MF_SYSEX_PARAM_ENCODER_SWITCH_STATE:
		case MF_SYSEX_PARAM_ENCODER_SWITCH_MODE:
		case MF_SYSEX_PARAM_ENCODER_SWITCH_PROTO: {
			struct encoder* encoder =
					encoder_from_indices(msg->param.enc.bank_idx, msg->param.enc.enc_idx);

			if (encoder == NULL) {
				ret = ERR_BAD_PARAM;
				goto cleanup;
			}

			param = (u8*)encoder + sysex_data_info[msg->param_enum].offset;

			// GET must not modify device state. The write used to run for both
			// commands, so reading a parameter overwrote it.
			if (msg->cmd == MF_SYSEX_SET) {
				memcpy(param, &msg->param.enc.data, param_len);
			}
			break;
		}

		case MF_SYSEX_PARAM_VMAP_RANGE:
		case MF_SYSEX_PARAM_VMAP_POSITION:
		case MF_SYSEX_PARAM_VMAP_RGB:
		case MF_SYSEX_PARAM_VMAP_RB:
		case MF_SYSEX_PARAM_VMAP_PROTO: {
			struct virtmap* vmap =
					virtmap_from_indices(msg->param.vmap.bank_idx,
															 msg->param.vmap.enc_idx,
															 msg->param.vmap.vmap_idx);

			if (vmap == NULL) {
				ret = ERR_BAD_PARAM;
				goto cleanup;
			}

			param = (u8*)vmap + sysex_data_info[msg->param_enum].offset;

			if (msg->cmd == MF_SYSEX_SET) {
				memcpy(param, &msg->param.vmap.data, param_len);
			}
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
			/*
				Return the parameter itself. This previously echoed `ret` - a status
				code - so a GET never actually reported the value it was asked for.

				Payload bytes must stay 7-bit clean to remain valid inside a sysex
				frame, so each byte is masked. Parameters whose values can exceed 127
				are not currently representable and will need a split-nibble encoding;
				none of the presently defined parameters do.
			*/
			midi_event_s reply = {0};
			reply.type									 = MIDI_EVENT_SYSEX;
			reply.data.sysex_out.cmd		 = MF_SYSEX_GET_RESPONSE;
			reply.data.sysex_out.param	 = msg->param_enum;

			u8 n = param_len;

			if (n > MIDI_SYSEX_OUT_DATA_LEN_MAX) {
				n = MIDI_SYSEX_OUT_DATA_LEN_MAX;
			}

			if (param != NULL) {
				for (u8 i = 0; i < n; i++) {
					reply.data.sysex_out.data[i] = param[i] & 0x7F;
				}
				reply.data.sysex_out.data_len = n;
			}

			event_post(EVENT_CHANNEL_MIDI_OUT, &reply);
			break;
		}

		case MF_SYSEX_SET: {
			midi_event_s reply = {0};
			reply.type										= MIDI_EVENT_SYSEX;
			reply.data.sysex_out.cmd			= MF_SYSEX_SET_RESPONSE;
			reply.data.sysex_out.param		= msg->param_enum;
			reply.data.sysex_out.data_len = 1;
			reply.data.sysex_out.data[0]	= (u8)(ret == SUCCESS ? 0 : 1);
			event_post(EVENT_CHANNEL_MIDI_OUT, &reply);
			break;
		}

		case MF_SYSEX_STOP: {
			// Accepted by the validation above, so it must be handled here. It
			// previously fell through to the default and always returned an error.
			// No streaming operation exists to stop yet; acknowledge and reset.
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

/**
 * @brief Number of index bytes that precede the payload for a parameter.
 *
 * Encoder parameters are addressed by (bank, encoder); vmap parameters by
 * (bank, encoder, vmap). The remaining parameters carry no index prefix.
 */
static u8 sysex_param_index_len(u8 param_enum) {
	switch (param_enum) {
		case MF_SYSEX_PARAM_ENCODER_DETENT:
		case MF_SYSEX_PARAM_ENCODER_DISPLAY_MODE:
		case MF_SYSEX_PARAM_ENCODER_VMAP_DISPLAY_MODE:
		case MF_SYSEX_PARAM_ENCODER_VMAP_MODE:
		case MF_SYSEX_PARAM_ENCODER_VMAP_ACTIVE:
		case MF_SYSEX_PARAM_ENCODER_SWITCH_STATE:
		case MF_SYSEX_PARAM_ENCODER_SWITCH_MODE:
		case MF_SYSEX_PARAM_ENCODER_SWITCH_PROTO: return 2;

		case MF_SYSEX_PARAM_VMAP_RANGE:
		case MF_SYSEX_PARAM_VMAP_POSITION:
		case MF_SYSEX_PARAM_VMAP_RGB:
		case MF_SYSEX_PARAM_VMAP_RB:
		case MF_SYSEX_PARAM_VMAP_PROTO: return 3;

		default: return 0;
	}
}

/**
 * @brief Resolve a bank/encoder pair received over the wire.
 * @return Pointer to the encoder, or NULL if either index is out of range.
 */
static struct encoder* encoder_from_indices(u8 bank, u8 enc) {
	if (bank >= NUM_ENC_BANKS || enc >= NUM_ENCODERS) {
		return NULL;
	}

	return &gENCODERS[bank][enc];
}

/**
 * @brief Resolve a bank/encoder/vmap triple received over the wire.
 * @return Pointer to the virtmap, or NULL if any index is out of range.
 */
static struct virtmap* virtmap_from_indices(u8 bank, u8 enc, u8 vmap) {
	struct encoder* encoder = encoder_from_indices(bank, enc);

	if (encoder == NULL || vmap >= NUM_VMAPS_PER_ENC) {
		return NULL;
	}

	return &encoder->vmaps[vmap];
}
