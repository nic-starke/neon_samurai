/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2026) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*
	Half-step quadrature decoder.

	A quadrature encoder presents two channels whose 2-bit value follows a Gray
	code, so exactly one bit changes per step. Writing the channel pair as
	val = (B << 1) | A, one full mechanical detent walks the cycle

	    3 (11) --> 1 (10) --> 0 (00) --> 2 (01) --> 3 (11)

	clockwise, and the reverse counter-clockwise. Positions 11 and 00 are the two
	stable rest points; 10 and 01 are the transitional points between them.

	Decoding needs memory of which transitional point was passed, because that is
	what distinguishes direction: arriving at 00 having passed 10 is clockwise,
	arriving at 00 having passed 01 is counter-clockwise. That gives six states -
	the two rest points, plus "heading somewhere, having passed X" for each of the
	four combinations:

	    AT_REST      at 11
	    REST_VIA_CW  left 11, passed 10   -> reaching 00 emits CW
	    REST_VIA_CCW left 11, passed 01   -> reaching 00 emits CCW
	    AT_MID       at 00
	    MID_VIA_CW   left 00, passed 01   -> reaching 11 emits CW
	    MID_VIA_CCW  left 00, passed 10   -> reaching 11 emits CCW

	A step is emitted on arrival at a rest point, so a full cycle yields two
	counts (half-step) - the resolution the rest of the firmware expects.
	Reversing before reaching the far rest point returns to the state it came
	from and emits nothing, which is what rejects contact bounce.

	The transition tables below are a direct transcription of that reasoning; the
	comment on each row states the position the row represents.

	Provenance: this replaces a state table taken from Ben Buxton's Rotary
	library, which is GPL-3 licensed and so could not be redistributed under this
	project's MIT licence. The state machine here was re-derived from the
	Gray-code cycle above - and arrives at the same transitions, because for a
	given cycle and half-step resolution they are determined by the encoder's
	physics rather than chosen. tests/test_quadrature.c proves the two are
	behaviourally identical across all 524288 emissions of an exhaustive
	eight-deep walk of every input sequence.
*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/pgmspace.h>

#include "system/types.h"
#include "event/io.h"

#include "io/quadrature.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Channel values, val = (B << 1) | A.
#define V_MID		 (0) // 00 - stable
#define V_CW		 (1) // 10 - transitional
#define V_CCW		 (2) // 01 - transitional
#define V_REST	 (3) // 11 - stable, mechanical detent

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum quad_state {
	QUAD_AT_REST,			 // at 11
	QUAD_REST_VIA_CW,	 // left 11 via 10 - reaching 00 is clockwise
	QUAD_REST_VIA_CCW, // left 11 via 01 - reaching 00 is counter-clockwise
	QUAD_AT_MID,			 // at 00
	QUAD_MID_VIA_CW,	 // left 00 via 01 - reaching 11 is clockwise
	QUAD_MID_VIA_CCW,	 // left 00 via 10 - reaching 11 is counter-clockwise

	QUAD_NB,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
	Next state for [current state][observed value].

	Read a row as "I am here; where does seeing each value put me". Any value
	that is neither a legal continuation nor the current position means a sample
	was missed or the contacts glitched, so the machine resynchronises to the
	nearest rest point rather than guessing a direction.

	PROGMEM: const alone leaves a table in SRAM on AVR.
*/
static const u8 QUAD_NEXT[QUAD_NB][4] PROGMEM = {
		//                   V_MID              V_CW               V_CCW              V_REST
		/* AT_REST      */ {QUAD_AT_MID, QUAD_REST_VIA_CW, QUAD_REST_VIA_CCW, QUAD_AT_REST},
		/* REST_VIA_CW  */ {QUAD_AT_MID, QUAD_REST_VIA_CW, QUAD_AT_REST, QUAD_AT_REST},
		/* REST_VIA_CCW */ {QUAD_AT_MID, QUAD_AT_REST, QUAD_REST_VIA_CCW, QUAD_AT_REST},
		/* AT_MID       */ {QUAD_AT_MID, QUAD_MID_VIA_CCW, QUAD_MID_VIA_CW, QUAD_AT_REST},
		/* MID_VIA_CW   */ {QUAD_AT_MID, QUAD_AT_MID, QUAD_MID_VIA_CW, QUAD_AT_REST},
		/* MID_VIA_CCW  */ {QUAD_AT_MID, QUAD_MID_VIA_CCW, QUAD_AT_MID, QUAD_AT_REST},
};

/*
	Direction emitted by [current state][observed value].

	Non-zero only on the four transitions that complete a half-step: arriving at
	00 from a known direction, and arriving at 11 from a known direction.
*/
static const i8 QUAD_EMIT[QUAD_NB][4] PROGMEM = {
		//                   V_MID  V_CW  V_CCW  V_REST
		/* AT_REST      */ {0, 0, 0, 0},
		/* REST_VIA_CW  */ {+1, 0, 0, 0},
		/* REST_VIA_CCW */ {-1, 0, 0, 0},
		/* AT_MID       */ {0, 0, 0, 0},
		/* MID_VIA_CW   */ {0, 0, 0, +1},
		/* MID_VIA_CCW  */ {0, 0, 0, -1},
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void quadrature_update(struct quadrature* ctx, uint ch_a, uint ch_b) {
	assert(ctx);

	const u8 val = (u8)(((ch_b & 1u) << 1) | (ch_a & 1u));
	const u8 st	 = (ctx->state < QUAD_NB) ? ctx->state : (u8)QUAD_AT_REST;

	ctx->dir	 = (i8)pgm_read_byte(&QUAD_EMIT[st][val]);
	ctx->state = pgm_read_byte(&QUAD_NEXT[st][val]);
}

int quadrature_direction(struct quadrature* ctx) {
	assert(ctx);
	return ctx->dir;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
