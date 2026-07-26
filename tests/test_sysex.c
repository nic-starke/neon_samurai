/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2026) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*
	Tests for the sysex message parser - the only place the firmware parses
	untrusted input.

	The parser is reached by #including sysex.c so the tests can call the static
	midi_in_handler() directly and inspect the static stream buffer. This is a
	deliberate choice for a module whose entire surface is one static function.

	Build with ASan+UBSan (see tests/CMakeLists.txt): several of the regression
	tests below rely on the sanitizer to catch an out-of-bounds access, rather
	than on an assertion that could itself be fooled.
*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "support/test.h"
#include "support/stubs.h"

#include "midi/sysex.c" // NOLINT - intentional, see above

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Globals ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

TEST_GLOBALS;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Helpers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Frame layout: F0 | mf_id[3] | cmd | param_enum | <indices+payload> | F7
#define FRAME_HDR_LEN (1 + 3 + 1 + 1)

/**
 * @brief Feed a complete sysex frame through the parser as USB-MIDI packets.
 *
 * Chunks the frame the way midi_lufa.c would: 3-byte SYSEX_START packets until
 * fewer than four bytes remain, then the matching END packet.
 *
 * @return The parser's return code for the final (completing) packet.
 */
static int feed_frame(const u8* frame, u8 len) {
	int ret = 0;
	u8	i		= 0;

	while (i < len) {
		u8					 remaining = (u8)(len - i);
		midi_event_s e				 = {0};
		e.type								 = MIDI_EVENT_SYSEX;

		u8 n;
		if (remaining > 3) {
			e.data.sysex_in.type = SYSEX_TYPE_START_3BYTE;
			n										 = 3;
		} else if (remaining == 3) {
			e.data.sysex_in.type = SYSEX_TYPE_END_3BYTE;
			n										 = 3;
		} else if (remaining == 2) {
			e.data.sysex_in.type = SYSEX_TYPE_END_2BYTE;
			n										 = 2;
		} else {
			e.data.sysex_in.type = SYSEX_TYPE_END_1BYTE;
			n										 = 1;
		}

		for (u8 k = 0; k < n; k++) {
			e.data.sysex_in.data[k] = frame[i + k];
		}

		ret = midi_in_handler(&e);
		i	 = (u8)(i + n);
	}

	return ret;
}

/** @brief Build an encoder-parameter frame: header + bank + enc + 1 payload. */
static u8 build_enc_frame(u8* out, u8 cmd, u8 param, u8 bank, u8 enc, u8 val) {
	u8 i	 = 0;
	out[i++] = MIDI_STATUS_SYSTEM_EXCLUSIVE;
	out[i++] = MIDI_MFR_ID_1;
	out[i++] = MIDI_MFR_ID_2;
	out[i++] = MIDI_MFR_ID_3;
	out[i++] = cmd;
	out[i++] = param;
	out[i++] = bank;
	out[i++] = enc;
	out[i++] = val;
	out[i++] = MIDI_STATUS_END_OF_EXCLUSIVE;
	return i;
}

static void reset_state(void) {
	buffer_idx	 = 0;
	stream_state = STREAM_IDLE;
	memset(buffer, 0, sizeof(buffer));
	memset(gENCODERS, 0, sizeof(gENCODERS));
	stub_reset();
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Tests ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void test_set_encoder_detent_applies_value(void) {
	reset_state();
	u8 f[16];
	u8 n = build_enc_frame(f, MF_SYSEX_SET, MF_SYSEX_PARAM_ENCODER_DETENT, 1, 5, 1);

	int ret = feed_frame(f, n);

	CHECK_EQ_INT(ret, SUCCESS);
	CHECK_EQ_INT(gENCODERS[1][5].detent, 1);
	CHECK_EQ_INT(stub_posted_count, 1);
	CHECK_EQ_INT(stub_last_post_channel, EVENT_CHANNEL_MIDI_OUT);
}

// Regression: bank/enc came straight off the wire and were used to index
// gENCODERS with no bounds check, so a crafted message wrote to arbitrary RAM.
static void test_set_rejects_out_of_range_bank(void) {
	reset_state();
	u8 f[16];
	u8 n = build_enc_frame(f, MF_SYSEX_SET, MF_SYSEX_PARAM_ENCODER_DETENT, 200, 0, 1);

	int ret = feed_frame(f, n);

	CHECK_EQ_INT(ret, ERR_BAD_PARAM);
	CHECK_EQ_INT(stub_posted_count, 0);
}

static void test_set_rejects_out_of_range_encoder(void) {
	reset_state();
	u8 f[16];
	u8 n = build_enc_frame(f, MF_SYSEX_SET, MF_SYSEX_PARAM_ENCODER_DETENT, 0, 250, 1);

	int ret = feed_frame(f, n);

	CHECK_EQ_INT(ret, ERR_BAD_PARAM);
	CHECK_EQ_INT(stub_posted_count, 0);
}

static void test_set_rejects_out_of_range_vmap(void) {
	reset_state();

	// vmap frame: header + bank + enc + vmap + 2 payload bytes (range) + F7
	u8 f[16];
	u8 i	 = 0;
	f[i++] = MIDI_STATUS_SYSTEM_EXCLUSIVE;
	f[i++] = MIDI_MFR_ID_1;
	f[i++] = MIDI_MFR_ID_2;
	f[i++] = MIDI_MFR_ID_3;
	f[i++] = MF_SYSEX_SET;
	f[i++] = MF_SYSEX_PARAM_VMAP_RANGE;
	f[i++] = 0;	 // bank
	f[i++] = 0;	 // enc
	f[i++] = 99; // vmap - out of range
	f[i++] = 10;
	f[i++] = 20;
	f[i++] = MIDI_STATUS_END_OF_EXCLUSIVE;

	int ret = feed_frame(f, i);

	CHECK_EQ_INT(ret, ERR_BAD_PARAM);
}

// Regression: the manufacturer ID check used && between the three byte
// comparisons, so a message was only rejected when ALL THREE bytes were wrong.
// Any sysex sharing one byte with "SAM" was accepted and acted upon.
static void test_rejects_frame_with_one_matching_mfr_byte(void) {
	reset_state();
	u8 f[16];
	u8 n = build_enc_frame(f, MF_SYSEX_SET, MF_SYSEX_PARAM_ENCODER_DETENT, 0, 0, 1);

	// Keep byte 0 correct ('S'), corrupt the other two.
	f[2] = 0x00;
	f[3] = 0x00;

	int ret = feed_frame(f, n);

	CHECK_EQ_INT(ret, ERR_BAD_MSG);
	CHECK_EQ_INT(gENCODERS[0][0].detent, 0);
}

static void test_rejects_wrong_manufacturer_id(void) {
	reset_state();
	u8 f[16];
	u8 n = build_enc_frame(f, MF_SYSEX_SET, MF_SYSEX_PARAM_ENCODER_DETENT, 0, 0, 1);
	f[1] = 0x7D;
	f[2] = 0x7D;
	f[3] = 0x7D;

	int ret = feed_frame(f, n);

	CHECK_EQ_INT(ret, ERR_BAD_MSG);
	CHECK_EQ_INT(gENCODERS[0][0].detent, 0);
}

// Regression: sysex_data_info[] was indexed ~25 lines before param_enum was
// bounds-checked, reading past the end of the table.
static void test_rejects_out_of_range_param_enum(void) {
	reset_state();
	u8 f[16];
	u8 n = build_enc_frame(f, MF_SYSEX_SET, 0x7E, 0, 0, 1);

	int ret = feed_frame(f, n);

	CHECK(ret != SUCCESS);
}

static void test_rejects_bad_command(void) {
	reset_state();
	u8 f[16];
	u8 n = build_enc_frame(f, 0x55, MF_SYSEX_PARAM_ENCODER_DETENT, 0, 0, 1);

	int ret = feed_frame(f, n);

	CHECK_EQ_INT(ret, ERR_BAD_MSG);
}

static void test_rejects_missing_start_byte(void) {
	reset_state();
	u8 f[16];
	u8 n = build_enc_frame(f, MF_SYSEX_SET, MF_SYSEX_PARAM_ENCODER_DETENT, 0, 0, 1);
	f[0] = 0x41; // not F0

	int ret = feed_frame(f, n);

	CHECK_EQ_INT(ret, ERR_BAD_MSG);
}

// Regression: no length validation existed. `expected_len` was computed and
// then discarded, followed by an empty `switch (msg->param_enum) {}`.
static void test_rejects_short_frame_for_parameter(void) {
	reset_state();

	// A detent frame with the payload byte omitted.
	u8 f[16];
	u8 i	 = 0;
	f[i++] = MIDI_STATUS_SYSTEM_EXCLUSIVE;
	f[i++] = MIDI_MFR_ID_1;
	f[i++] = MIDI_MFR_ID_2;
	f[i++] = MIDI_MFR_ID_3;
	f[i++] = MF_SYSEX_SET;
	f[i++] = MF_SYSEX_PARAM_ENCODER_DETENT;
	f[i++] = 0;
	f[i++] = 0;
	f[i++] = MIDI_STATUS_END_OF_EXCLUSIVE;

	int ret = feed_frame(f, i);

	CHECK_EQ_INT(ret, ERR_BAD_MSG);
}

static void test_rejects_runt_frame(void) {
	reset_state();
	u8 f[2] = {MIDI_STATUS_SYSTEM_EXCLUSIVE, MIDI_STATUS_END_OF_EXCLUSIVE};

	int ret = feed_frame(f, sizeof(f));

	CHECK_EQ_INT(ret, ERR_BAD_MSG);
}

// Regression: the memcpy that applies a value ran for GET as well as SET, so
// reading a parameter overwrote it with whatever the request carried.
static void test_get_does_not_modify_state(void) {
	reset_state();
	gENCODERS[0][3].detent = 1;

	u8 f[16];
	u8 n = build_enc_frame(f, MF_SYSEX_GET, MF_SYSEX_PARAM_ENCODER_DETENT, 0, 3, 0);

	int ret = feed_frame(f, n);

	CHECK_EQ_INT(ret, SUCCESS);
	CHECK_EQ_INT(gENCODERS[0][3].detent, 1); // unchanged by the GET
}

// Regression: GET echoed the status code instead of the requested value.
static void test_get_returns_parameter_value(void) {
	reset_state();
	gENCODERS[2][7].detent = 1;

	u8 f[16];
	u8 n = build_enc_frame(f, MF_SYSEX_GET, MF_SYSEX_PARAM_ENCODER_DETENT, 2, 7, 0);

	int ret = feed_frame(f, n);

	CHECK_EQ_INT(ret, SUCCESS);
	CHECK_EQ_INT(stub_posted_count, 1);
	CHECK_EQ_INT(stub_last_sysex.cmd, MF_SYSEX_GET_RESPONSE);
	CHECK_EQ_INT(stub_last_sysex.param, MF_SYSEX_PARAM_ENCODER_DETENT);
	CHECK_EQ_INT(stub_last_sysex.data_len, 1);
	CHECK_EQ_INT(stub_last_sysex.data[0], 1);
}

// Regression: STOP passed command validation and then always fell through to
// the default case, which returned an error.
static void test_stop_command_is_accepted(void) {
	reset_state();
	u8 f[16];
	u8 n = build_enc_frame(f, MF_SYSEX_STOP, MF_SYSEX_PARAM_ENCODER_DETENT, 0, 0, 0);

	int ret = feed_frame(f, n);

	CHECK_EQ_INT(ret, SUCCESS);
}

// Regression: the overflow guard compared against MF_SYSEX_MAX_PKT_SIZE, which
// excludes the F0/F7 bytes the buffer also holds. Feeding a long run of packets
// must never write past the buffer - ASan is the real assertion here.
static void test_oversized_stream_does_not_overflow_buffer(void) {
	reset_state();

	for (int i = 0; i < 64; i++) {
		midi_event_s e			 = {0};
		e.type							 = MIDI_EVENT_SYSEX;
		e.data.sysex_in.type = SYSEX_TYPE_START_3BYTE;
		e.data.sysex_in.data[0] = 0x11;
		e.data.sysex_in.data[1] = 0x22;
		e.data.sysex_in.data[2] = 0x33;
		(void)midi_in_handler(&e);
	}

	CHECK(buffer_idx <= sizeof(buffer));
}

static void test_ignores_non_sysex_events(void) {
	reset_state();
	midi_event_s e = {0};
	e.type				 = MIDI_EVENT_CC;

	int ret = midi_in_handler(&e);

	CHECK_EQ_INT(ret, 0);
	CHECK_EQ_INT(stub_posted_count, 0);
}

// A rejected frame must leave the parser ready for the next one.
static void test_parser_recovers_after_bad_frame(void) {
	reset_state();

	u8 bad[16];
	u8 bn = build_enc_frame(bad, MF_SYSEX_SET, MF_SYSEX_PARAM_ENCODER_DETENT, 200, 0, 1);
	(void)feed_frame(bad, bn);

	stub_reset();

	u8 good[16];
	u8 gn = build_enc_frame(good, MF_SYSEX_SET, MF_SYSEX_PARAM_ENCODER_DETENT, 0, 1, 1);
	int ret = feed_frame(good, gn);

	CHECK_EQ_INT(ret, SUCCESS);
	CHECK_EQ_INT(gENCODERS[0][1].detent, 1);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Main ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int main(void) {
	printf("sysex parser\n");

	RUN_TEST(test_set_encoder_detent_applies_value);
	RUN_TEST(test_set_rejects_out_of_range_bank);
	RUN_TEST(test_set_rejects_out_of_range_encoder);
	RUN_TEST(test_set_rejects_out_of_range_vmap);
	RUN_TEST(test_rejects_frame_with_one_matching_mfr_byte);
	RUN_TEST(test_rejects_wrong_manufacturer_id);
	RUN_TEST(test_rejects_out_of_range_param_enum);
	RUN_TEST(test_rejects_bad_command);
	RUN_TEST(test_rejects_missing_start_byte);
	RUN_TEST(test_rejects_short_frame_for_parameter);
	RUN_TEST(test_rejects_runt_frame);
	RUN_TEST(test_get_does_not_modify_state);
	RUN_TEST(test_get_returns_parameter_value);
	RUN_TEST(test_stop_command_is_accepted);
	RUN_TEST(test_oversized_stream_does_not_overflow_buffer);
	RUN_TEST(test_ignores_non_sysex_events);
	RUN_TEST(test_parser_recovers_after_bad_frame);

	return TEST_SUMMARY();
}
