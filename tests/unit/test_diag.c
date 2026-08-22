/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "nstest.h"

#include "system/types.h"
#include "system/diag.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Tests ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

TEST(a_clean_run_records_nothing) {
	diag_reset();

	CHECK_EQ(diag_total(), 0);
	for (u8 i = 0; i < DIAG_NB; i++) {
		CHECK_EQ(diag_get((enum diag_counter)i), 0);
	}
}

TEST(counters_are_independent) {
	diag_reset();

	diag_count(DIAG_EVENT_DROPPED);
	diag_count(DIAG_EVENT_DROPPED);
	diag_count(DIAG_MIDI_TX_FAILED);

	CHECK_EQ(diag_get(DIAG_EVENT_DROPPED), 2);
	CHECK_EQ(diag_get(DIAG_MIDI_TX_FAILED), 1);
	CHECK_EQ(diag_get(DIAG_DISPLAY_FAILED), 0);
	CHECK_EQ(diag_get(DIAG_CFG_STORE_FAILED), 0);
	CHECK_EQ(diag_total(), 3);
}

TEST(a_counter_saturates_rather_than_wrapping) {
	diag_reset();

	// A wrapped counter can read as zero while the fault is still happening,
	// which is exactly the case the counter exists to make visible.
	for (u32 i = 0; i < 70000u; i++) {
		diag_count(DIAG_DISPLAY_FAILED);
	}

	CHECK_EQ(diag_get(DIAG_DISPLAY_FAILED), UINT16_MAX);
	CHECK(diag_get(DIAG_DISPLAY_FAILED) != 0);
}

TEST(the_total_survives_every_counter_saturating) {
	diag_reset();

	for (u8 c = 0; c < DIAG_NB; c++) {
		for (u32 i = 0; i < 70000u; i++) {
			diag_count((enum diag_counter)c);
		}
	}

	// Four saturated 16-bit counters exceed what one of them can hold, so the
	// total has to be wider than the counters themselves.
	CHECK_EQ(diag_total(), (u32)UINT16_MAX * DIAG_NB);
}

TEST(an_out_of_range_counter_is_ignored) {
	diag_reset();

	diag_count((enum diag_counter)DIAG_NB);
	diag_count((enum diag_counter)(DIAG_NB + 7));

	CHECK_EQ(diag_total(), 0);
	CHECK_EQ(diag_get((enum diag_counter)DIAG_NB), 0);
}

TEST(reset_clears_everything) {
	diag_reset();

	for (u8 i = 0; i < DIAG_NB; i++) {
		diag_count((enum diag_counter)i);
	}
	CHECK_EQ(diag_total(), DIAG_NB);

	diag_reset();
	CHECK_EQ(diag_total(), 0);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Main ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

NSTEST_MAIN(RUN(a_clean_run_records_nothing); RUN(counters_are_independent);
						RUN(a_counter_saturates_rather_than_wrapping);
						RUN(the_total_survives_every_counter_saturating);
						RUN(an_out_of_range_counter_is_ignored);
						RUN(reset_clears_everything);)
