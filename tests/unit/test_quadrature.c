/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "nstest.h"

#include "system/types.h"
#include "io/quadrature.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
	The decoder reads a two-channel gray code. Walking the ring
	(A,B) 00 -> 01 -> 11 -> 10 -> 00 is one direction, and walking it backwards
	is the other. Which of the two the hardware calls "clockwise" is a wiring
	fact, so these tests pin the direction rather than re-deriving it: an
	inverted table would still be self-consistent but would send every encoder
	the wrong way.
*/

// Ring positions as (ch_a, ch_b) pairs, in forward order.
static const u8 RING_A[4] = {0, 0, 1, 1};
static const u8 RING_B[4] = {0, 1, 1, 0};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void step_to(struct quadrature* ctx, int pos) {
	pos = ((pos % 4) + 4) % 4;
	quadrature_update(ctx, RING_A[pos], RING_B[pos]);
}

// Walk the ring and return the net direction reported over the whole walk.
static int walk(struct quadrature* ctx, int start, int steps, int stride) {
	int net = 0;
	for (int i = 1; i <= steps; ++i) {
		step_to(ctx, start + (i * stride));
		net += quadrature_direction(ctx);
	}
	return net;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Tests ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

TEST(idle_when_nothing_moves) {
	struct quadrature ctx = {0};

	step_to(&ctx, 0);
	for (int i = 0; i < 8; ++i) {
		step_to(&ctx, 0);
		CHECK_EQ(quadrature_direction(&ctx), 0);
	}
}

TEST(forward_ring_is_one_direction) {
	struct quadrature ctx = {0};

	step_to(&ctx, 0);
	// Four ring positions at two sub-steps per detent is two detents.
	CHECK_EQ(walk(&ctx, 0, 4, +1), 2);
}

TEST(reverse_ring_is_the_other_direction) {
	struct quadrature ctx = {0};

	step_to(&ctx, 0);
	CHECK_EQ(walk(&ctx, 0, 4, -1), -2);
}

TEST(directions_are_opposite) {
	struct quadrature fwd = {0};
	struct quadrature rev = {0};

	step_to(&fwd, 0);
	step_to(&rev, 0);

	CHECK_EQ(walk(&fwd, 0, 8, +1), -walk(&rev, 0, 8, -1));
}

TEST(a_full_revolution_returns_to_start) {
	struct quadrature ctx = {0};

	step_to(&ctx, 0);
	(void)walk(&ctx, 0, 4, +1);

	CHECK_EQ(ctx.rot, (u8)(((RING_B[0] & 1u) << 1) | (RING_A[0] & 1u)));
	CHECK_EQ(ctx.accum, 0);
}

TEST(sub_steps_bank_toward_a_detent) {
	struct quadrature ctx = {0};

	step_to(&ctx, 0);

	// One sub-step is not yet a detent.
	step_to(&ctx, 1);
	CHECK_EQ(quadrature_direction(&ctx), 0);
	CHECK(ctx.accum != 0);

	// The second completes it, and the accumulator resets.
	step_to(&ctx, 2);
	CHECK(quadrature_direction(&ctx) != 0);
	CHECK_EQ(ctx.accum, 0);
}

TEST(reversal_cancels_a_banked_sub_step) {
	struct quadrature ctx = {0};

	step_to(&ctx, 0);
	step_to(&ctx, 1); // bank one sub-step forward
	step_to(&ctx, 0); // and immediately take it back

	CHECK_EQ(quadrature_direction(&ctx), 0);
	CHECK_EQ(ctx.accum, 0);
}

TEST(an_illegal_jump_reports_no_movement) {
	struct quadrature ctx = {0};

	step_to(&ctx, 0);
	// Both channels changing at once cannot happen on a gray code ring, so it
	// carries no usable direction and must not be guessed at.
	step_to(&ctx, 2);
	CHECK_EQ(quadrature_direction(&ctx), 0);
}

TEST(channel_inputs_are_masked_to_one_bit) {
	struct quadrature clean = {0};
	struct quadrature noisy = {0};

	quadrature_update(&clean, 0, 0);
	quadrature_update(&noisy, 0xFE, 0xFE); // same low bits, junk above them

	quadrature_update(&clean, 1, 0);
	quadrature_update(&noisy, 0xFF, 0xFE);

	CHECK_EQ(clean.rot, noisy.rot);
	CHECK_EQ(quadrature_direction(&clean), quadrature_direction(&noisy));
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Main ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

NSTEST_MAIN(RUN(idle_when_nothing_moves); RUN(forward_ring_is_one_direction);
						RUN(reverse_ring_is_the_other_direction);
						RUN(directions_are_opposite);
						RUN(a_full_revolution_returns_to_start);
						RUN(sub_steps_bank_toward_a_detent);
						RUN(reversal_cancels_a_banked_sub_step);
						RUN(an_illegal_jump_reports_no_movement);
						RUN(channel_inputs_are_masked_to_one_bit);)
