/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2026) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*
	Tests for the quadrature decoder.

	Two jobs here. The behavioural tests pin down the contract: half-step
	resolution, direction polarity, and bounce rejection. The equivalence test
	then proves the current decoder produces the same output as the GPL-3 table
	it replaced, for every reachable input sequence - which is what makes the
	licence swap safe to land without access to hardware.

	The reference below is a transcription of the previous behaviour used solely
	as a test oracle. It is not compiled into the firmware.
*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "support/test.h"

#include "encoder/quadrature.c" // NOLINT - reaches the module under test

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Globals ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

TEST_GLOBALS;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Reference oracle ~~~~~~~~~~~~~~~~~~~~~~~~ */

enum ref_state {
	REF_START,
	REF_CCW,
	REF_CW,
	REF_MIDDLE,
	REF_MID_CW,
	REF_MID_CCW,
	REF_NB,
};

#define REF_DIR_CW	(0x10)
#define REF_DIR_CCW (0x20)

static const u8 ref_states[REF_NB][4] = {
		{REF_MIDDLE, REF_CW, REF_CCW, REF_START},
		{REF_MIDDLE | REF_DIR_CCW, REF_START, REF_CCW, REF_START},
		{REF_MIDDLE | REF_DIR_CW, REF_CW, REF_START, REF_START},
		{REF_MIDDLE, REF_MID_CCW, REF_MID_CW, REF_START},
		{REF_MIDDLE, REF_MIDDLE, REF_MID_CW, REF_START | REF_DIR_CW},
		{REF_MIDDLE, REF_MID_CCW, REF_MIDDLE, REF_START | REF_DIR_CCW},
};

struct ref_ctx {
	u8 dir;
	u8 rot;
};

static int ref_update(struct ref_ctx* c, uint ch_a, uint ch_b) {
	uint val = (ch_b << 1) | ch_a;
	c->rot	 = ref_states[c->rot & 0x0F][val];
	c->dir	 = c->rot & 0x30;

	if (c->dir == REF_DIR_CW) {
		return 1;
	}
	if (c->dir == REF_DIR_CCW) {
		return -1;
	}
	return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Helpers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Channel values around the cycle. val = (B << 1) | A.
#define V_DETENT (3) // 11 - mechanical rest
#define V_CW1		 (1) // 10
#define V_MID		 (0) // 00
#define V_CW2		 (2) // 01

static int step(struct quadrature* q, u8 val) {
	quadrature_update(q, (uint)(val & 1u), (uint)((val >> 1) & 1u));
	return quadrature_direction(q);
}

/** @brief Feed a value sequence, return the summed direction output. */
static int run_seq(const u8* vals, int n) {
	struct quadrature q = {0};
	int							total = 0;

	// Sync to the physical rest position first, as the hardware scan does.
	(void)step(&q, V_DETENT);

	for (int i = 0; i < n; i++) {
		total += step(&q, vals[i]);
	}
	return total;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Behaviour ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// One full cycle must produce two counts (half-step resolution). Changing this
// changes the feel of every encoder on the device.
static void test_full_cw_cycle_emits_two_counts(void) {
	const u8 cycle[] = {V_CW1, V_MID, V_CW2, V_DETENT};
	CHECK_EQ_INT(run_seq(cycle, 4), 2);
}

static void test_full_ccw_cycle_emits_two_counts(void) {
	const u8 cycle[] = {V_CW2, V_MID, V_CW1, V_DETENT};
	CHECK_EQ_INT(run_seq(cycle, 4), -2);
}

static void test_three_cw_cycles_emit_six_counts(void) {
	const u8 cycle[] = {V_CW1, V_MID, V_CW2, V_DETENT, V_CW1, V_MID, V_CW2,
											V_DETENT, V_CW1, V_MID, V_CW2, V_DETENT};
	CHECK_EQ_INT(run_seq(cycle, 12), 6);
}

// A partial movement that returns the way it came must produce nothing.
static void test_bounce_at_detent_is_rejected(void) {
	const u8 seq[] = {V_CW1, V_DETENT, V_CW1, V_DETENT, V_CW1, V_DETENT};
	CHECK_EQ_INT(run_seq(seq, 6), 0);
}

static void test_bounce_at_mid_is_rejected(void) {
	const u8 seq[] = {V_CW1, V_MID, V_CW1, V_MID, V_CW1, V_MID};
	// Reaching V_MID the first time is a genuine half-step; the jitter after it
	// must not add more.
	CHECK_EQ_INT(run_seq(seq, 6), 1);
}

static void test_stationary_emits_nothing(void) {
	const u8 seq[] = {V_DETENT, V_DETENT, V_DETENT, V_DETENT};
	CHECK_EQ_INT(run_seq(seq, 4), 0);
}

static void test_reversal_mid_cycle_nets_out(void) {
	// Half a cycle clockwise, then back.
	const u8 seq[] = {V_CW1, V_MID, V_CW1, V_DETENT};
	CHECK_EQ_INT(run_seq(seq, 4), 0);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Equivalence ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
	Exhaustive comparison against the replaced decoder.

	Walks every sequence of channel values up to a fixed depth and requires the
	new decoder and the reference to agree on every single emission - not merely
	on the total. 4^8 = 65536 sequences, each 8 transitions long, covers every
	reachable state pair many times over.
*/
static void test_matches_previous_decoder_exhaustively(void) {
	const int depth		 = 8;
	int				mismatches = 0;
	long			checked		 = 0;

	for (long code = 0; code < 65536L; code++) {
		struct quadrature q = {0};
		struct ref_ctx		r = {0};

		// Both start from the same synced position.
		(void)step(&q, V_DETENT);
		(void)ref_update(&r, 1, 1);

		long c = code;
		for (int i = 0; i < depth; i++) {
			u8 val = (u8)(c & 3);
			c >>= 2;

			int got	 = step(&q, val);
			int want = ref_update(&r, (uint)(val & 1u), (uint)((val >> 1) & 1u));

			checked++;
			if (got != want) {
				if (mismatches < 3) {
					printf("\n    seq=0x%04lx step=%d val=%u: got %d, reference %d", code,
								 i, val, got, want);
				}
				mismatches++;
			}
		}
	}

	printf("\n    (%ld emissions compared) ", checked);
	CHECK_EQ_INT(mismatches, 0);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Main ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int main(void) {
	printf("quadrature decoder\n");

	RUN_TEST(test_full_cw_cycle_emits_two_counts);
	RUN_TEST(test_full_ccw_cycle_emits_two_counts);
	RUN_TEST(test_three_cw_cycles_emit_six_counts);
	RUN_TEST(test_bounce_at_detent_is_rejected);
	RUN_TEST(test_bounce_at_mid_is_rejected);
	RUN_TEST(test_stationary_emits_nothing);
	RUN_TEST(test_reversal_mid_cycle_nets_out);
	RUN_TEST(test_matches_previous_decoder_exhaustively);

	return TEST_SUMMARY();
}
