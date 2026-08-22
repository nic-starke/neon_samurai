/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2026) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
	The two channels of a quadrature encoder form a Gray code: exactly one bit
	changes per transition, so the four states sit on a ring

		(A,B)  00 -> 01 -> 11 -> 10 -> 00

	Give each state its index on that ring and a transition becomes a signed
	difference: +1 one way, -1 the other, 0 for no change, and 2 for a jump of
	two states, which is only reachable by missing a sample or by contact
	bounce and so is discarded.

	QUAD_DELTA below is that difference precomputed for all sixteen
	(previous, current) pairs.

	The detents are half a Gray cycle apart, so a step is reported once two
	sub-steps have accumulated in the same direction. Bounce dithers the
	accumulator around zero without ever reaching the threshold, which is what
	rejects it.
*/

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"

#include "io/quadrature.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define QUAD_SUBSTEPS_PER_STEP (2)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Indexed by (previous << 2) | current, where each state is (ch_b << 1) | ch_a.
// Kept in SRAM rather than PROGMEM - this is polled for every encoder on every
// scan, and an lpm would cost more than the 16 bytes are worth.
static const i8 QUAD_DELTA[16] = {
		0, -1, +1, 0, +1, 0, 0, -1, -1, 0, 0, +1, 0, +1, -1, 0,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void quadrature_update(struct quadrature* ctx, uint ch_a, uint ch_b) {
	assert(ctx);

	const u8 state = (u8)(((ch_b & 1u) << 1) | (ch_a & 1u));
	const i8 delta = QUAD_DELTA[(ctx->rot << 2) | state];

	ctx->rot = state;
	ctx->dir = DIR_ST;

	if (delta == 0) {
		return;
	}

	ctx->accum = (i8)(ctx->accum + delta);

	if (ctx->accum >= QUAD_SUBSTEPS_PER_STEP) {
		ctx->dir	 = DIR_CW;
		ctx->accum = 0;
	} else if (ctx->accum <= -QUAD_SUBSTEPS_PER_STEP) {
		ctx->dir	 = DIR_CCW;
		ctx->accum = 0;
	}
}

inline int quadrature_direction(struct quadrature* ctx) {
	assert(ctx);

	if (ctx->dir == DIR_CW) {
		return 1;
	} else if (ctx->dir == DIR_CCW) {
		return -1;
	}

	return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
