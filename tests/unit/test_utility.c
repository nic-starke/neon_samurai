/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "nstest.h"

#include "system/types.h"
#include "system/utility.h"
#include "midi/midi_cc.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Tests ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

TEST(the_endpoints_map_exactly) {
	// A knob at either stop must produce the exact limit of the MIDI range,
	// otherwise a control can never reach full off or full on.
	CHECK_EQ(convert_range_i16(0, 0, 255, 0, 127), 0);
	CHECK_EQ(convert_range_i16(255, 0, 255, 0, 127), 127);

	CHECK_EQ(convert_range_i16(0, 0, 255, 0, 16383), 0);
	CHECK_EQ(convert_range_i16(255, 0, 255, 0, 16383), 16383);
}

TEST(the_full_encoder_span_stays_inside_the_seven_bit_range) {
	for (i16 pos = 0; pos <= 255; ++pos) {
		i16 val = convert_range_i16(pos, 0, 255, 0, MIDI_CC_MAX);
		CHECK(val >= 0);
		CHECK(val <= MIDI_CC_MAX);
	}
}

TEST(the_full_encoder_span_stays_inside_the_fourteen_bit_range) {
	// This is the case that overflows if the intermediate product is computed
	// in 16 bits: 255 * 16383 does not fit.
	for (i16 pos = 0; pos <= 255; ++pos) {
		i16 val = convert_range_i16(pos, 0, 255, 0, MIDI_CC_14B_MAX);
		CHECK(val >= 0);
		CHECK(val <= MIDI_CC_14B_MAX);
	}
}

TEST(the_mapping_never_goes_backwards) {
	i16 previous = -1;
	for (i16 pos = 0; pos <= 255; ++pos) {
		i16 val = convert_range_i16(pos, 0, 255, 0, MIDI_CC_14B_MAX);
		CHECK(val >= previous);
		previous = val;
	}
}

TEST(a_descending_range_inverts_the_value) {
	// Range endpoints given high-to-low are how an inverted control is stored.
	CHECK_EQ(convert_range_i16(0, 0, 255, 127, 0), 127);
	CHECK_EQ(convert_range_i16(255, 0, 255, 127, 0), 0);

	i16 previous = 128;
	for (i16 pos = 0; pos <= 255; ++pos) {
		i16 val = convert_range_i16(pos, 0, 255, 127, 0);
		CHECK(val <= previous);
		previous = val;
	}
}

TEST(a_sub_range_is_respected) {
	// A control limited to part of the MIDI range must never leave it.
	for (i16 pos = 0; pos <= 255; ++pos) {
		i16 val = convert_range_i16(pos, 0, 255, 40, 80);
		CHECK(val >= 40);
		CHECK(val <= 80);
	}
	CHECK_EQ(convert_range_i16(0, 0, 255, 40, 80), 40);
	CHECK_EQ(convert_range_i16(255, 0, 255, 40, 80), 80);
}

TEST(a_zero_width_source_span_does_not_divide_by_zero) {
	// Reachable from stored configuration, so it must produce an answer rather
	// than trap.
	CHECK_EQ(convert_range_i16(0, 100, 100, 0, 127), 0);
	CHECK_EQ(convert_range_i16(50, 100, 100, 20, 80), 20);
	CHECK_EQ(convert_range_i32(50, 100, 100, 20, 80), 20);
}

TEST(a_zero_width_target_range_pins_the_output) {
	// A control configured to a single value should emit only that value.
	for (i16 pos = 0; pos <= 255; ++pos) {
		CHECK_EQ(convert_range_i16(pos, 0, 255, 64, 64), 64);
	}
}

TEST(the_wide_variant_handles_values_beyond_sixteen_bits) {
	CHECK_EQ(convert_range_i32(0, 0, 100000, 0, 1000), 0);
	CHECK_EQ(convert_range_i32(100000, 0, 100000, 0, 1000), 1000);
	CHECK_EQ(convert_range_i32(50000, 0, 100000, 0, 1000), 500);
}

TEST(clamp_min_and_max_agree_on_the_boundaries) {
	CHECK_EQ(CLAMP(5, 0, 10), 5);
	CHECK_EQ(CLAMP(-1, 0, 10), 0);
	CHECK_EQ(CLAMP(11, 0, 10), 10);
	CHECK_EQ(CLAMP(0, 0, 10), 0);
	CHECK_EQ(CLAMP(10, 0, 10), 10);

	CHECK_EQ(MIN(3, 9), 3);
	CHECK_EQ(MAX(3, 9), 9);
	CHECK(IN_RANGE(5, 0, 10));
	CHECK(IN_RANGE(0, 0, 10));
	CHECK(IN_RANGE(10, 0, 10));
	CHECK(!IN_RANGE(11, 0, 10));
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Main ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

NSTEST_MAIN(RUN(the_endpoints_map_exactly);
						RUN(the_full_encoder_span_stays_inside_the_seven_bit_range);
						RUN(the_full_encoder_span_stays_inside_the_fourteen_bit_range);
						RUN(the_mapping_never_goes_backwards);
						RUN(a_descending_range_inverts_the_value);
						RUN(a_sub_range_is_respected);
						RUN(a_zero_width_source_span_does_not_divide_by_zero);
						RUN(a_zero_width_target_range_pins_the_output);
						RUN(the_wide_variant_handles_values_beyond_sixteen_bits);
						RUN(clamp_min_and_max_agree_on_the_boundaries);)
