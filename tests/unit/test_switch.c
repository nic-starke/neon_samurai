/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "nstest.h"

#include "system/types.h"
#include "io/switch.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
	Feed the same reading in often enough for the debouncer to accept it.
	switch_xN_update() debounces as part of the update, so calling the debounce
	entry point again here would recompute the edge flags against the settled
	level and lose them.
*/
static void settle(struct switch_x16_ctx* ctx, u16 states) {
	for (int i = 0; i < SWITCH_DEBOUNCE_SAMPLES; ++i) {
		switch_x16_update(ctx, states);
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Tests ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

TEST(a_settled_press_is_reported) {
	struct switch_x16_ctx ctx = {0};

	settle(&ctx, 0x0000);
	CHECK_EQ(switch_x16_state(&ctx, 3), SWITCH_IDLE);

	settle(&ctx, 1u << 3);
	CHECK_EQ(switch_x16_state(&ctx, 3), SWITCH_PRESSED);
	CHECK_EQ(switch_x16_states(&ctx), 1u << 3);
}

TEST(a_settled_release_is_reported) {
	struct switch_x16_ctx ctx = {0};

	settle(&ctx, 1u << 7);
	CHECK_EQ(switch_x16_state(&ctx, 7), SWITCH_PRESSED);

	settle(&ctx, 0x0000);
	CHECK_EQ(switch_x16_state(&ctx, 7), SWITCH_IDLE);
}

TEST(one_noisy_sample_cannot_fake_a_press) {
	struct switch_x16_ctx ctx = {0};

	settle(&ctx, 0x0000);

	// A single stray high reading among otherwise idle samples.
	for (int i = 0; i < SWITCH_DEBOUNCE_SAMPLES; ++i) {
		switch_x16_update(&ctx, (i == 4) ? (1u << 2) : 0x0000);
	}

	CHECK_EQ(switch_x16_state(&ctx, 2), SWITCH_IDLE);
}

TEST(one_noisy_sample_cannot_fake_a_release) {
	struct switch_x16_ctx ctx = {0};

	settle(&ctx, 1u << 2);

	// A single stray low reading while the switch is genuinely held.
	for (int i = 0; i < SWITCH_DEBOUNCE_SAMPLES; ++i) {
		switch_x16_update(&ctx, (i == 4) ? 0x0000 : (1u << 2));
	}

	CHECK_EQ(switch_x16_state(&ctx, 2), SWITCH_PRESSED);
}

TEST(edges_are_reported_once) {
	struct switch_x16_ctx ctx = {0};

	settle(&ctx, 0x0000);

	settle(&ctx, 1u << 5);
	CHECK(switchx16_was_pressed(&ctx, 5));
	CHECK(!switchx16_was_released(&ctx, 5));

	// Still held - the edge has already been consumed.
	settle(&ctx, 1u << 5);
	CHECK(!switchx16_was_pressed(&ctx, 5));

	settle(&ctx, 0x0000);
	CHECK(switchx16_was_released(&ctx, 5));
	CHECK(!switchx16_was_pressed(&ctx, 5));
}

TEST(switches_are_independent) {
	struct switch_x16_ctx ctx = {0};

	settle(&ctx, 0x0000);
	settle(&ctx, (1u << 0) | (1u << 15));

	CHECK_EQ(switch_x16_state(&ctx, 0), SWITCH_PRESSED);
	CHECK_EQ(switch_x16_state(&ctx, 15), SWITCH_PRESSED);
	for (u8 i = 1; i < 15; ++i) {
		CHECK_EQ(switch_x16_state(&ctx, i), SWITCH_IDLE);
	}
}

TEST(the_top_bit_survives_the_shift) {
	struct switch_x16_ctx ctx = {0};

	// Bit 15 is where a u16 context would lose a switch to an int-width shift.
	settle(&ctx, 1u << 15);
	CHECK_EQ(switch_x16_state(&ctx, 15), SWITCH_PRESSED);
	CHECK(switchx16_was_pressed(&ctx, 15));
}

TEST(x8_debounces_the_same_way) {
	struct switch_x8_ctx ctx = {0};

	for (int i = 0; i < SWITCH_DEBOUNCE_SAMPLES; ++i) {
		switch_x8_update(&ctx, 0x00);
	}
	CHECK_EQ(switch_x8_state(&ctx, 1), SWITCH_IDLE);

	for (int i = 0; i < SWITCH_DEBOUNCE_SAMPLES; ++i) {
		switch_x8_update(&ctx, 1u << 1);
	}
	CHECK_EQ(switch_x8_state(&ctx, 1), SWITCH_PRESSED);
	CHECK(switchx8_was_pressed(&ctx, 1));

	// And a lone glitch is filtered out here too.
	for (int i = 0; i < SWITCH_DEBOUNCE_SAMPLES; ++i) {
		switch_x8_update(&ctx, (i == 2) ? 0x00 : (1u << 1));
	}
	CHECK_EQ(switch_x8_state(&ctx, 1), SWITCH_PRESSED);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Main ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

NSTEST_MAIN(RUN(a_settled_press_is_reported);
						RUN(a_settled_release_is_reported);
						RUN(one_noisy_sample_cannot_fake_a_press);
						RUN(one_noisy_sample_cannot_fake_a_release);
						RUN(edges_are_reported_once); RUN(switches_are_independent);
						RUN(the_top_bit_survives_the_shift);
						RUN(x8_debounces_the_same_way);)
