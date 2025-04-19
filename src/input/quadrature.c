/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "sys/types.h"
#include "event/io.h"

#include "input/quadrature.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
	QUAD_START,
	QUAD_CCW,
	QUAD_CW,
	QUAD_MIDDLE,
	QUAD_MID_CW,
	QUAD_MID_CCW,

	QUAD_NB,
} quad_state_e;

typedef enum {
	DIR_ST	= 0x00, // Stationary
	DIR_CW	= 0x10, // Clockwise
	DIR_CCW = 0x20, // Counter-clockwise
} encoder_dir_e;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Rotory decoder based on
 * https://github.com/buxtronix/arduino/tree/master/libraries/Rotary
 * Copyright 2011 Ben Buxton.
 * Licenced under the GNU GPL Version 3.
 * Contact: bb@cactii.net
 */
static const quad_state_e quad_states[QUAD_NB][4] = {
		// Current Quadrature GrayCode
		{QUAD_MIDDLE, QUAD_CW, QUAD_CCW, QUAD_START},
		{QUAD_MIDDLE | DIR_CCW, QUAD_START, QUAD_CCW, QUAD_START},
		{QUAD_MIDDLE | DIR_CW, QUAD_CW, QUAD_START, QUAD_START},
		{QUAD_MIDDLE, QUAD_MID_CCW, QUAD_MID_CW, QUAD_START},
		{QUAD_MIDDLE, QUAD_MIDDLE, QUAD_MID_CW, QUAD_START | DIR_CW},
		{QUAD_MIDDLE, QUAD_MID_CCW, QUAD_MIDDLE, QUAD_START | DIR_CCW},
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void quadrature_update(quadrature_s* ctx, uint ch_a, uint ch_b) {
	assert(ctx);

	uint val = (ch_b << 1) | ch_a;
	ctx->rot = quad_states[ctx->rot & 0x0F][val];
	ctx->dir = ctx->rot & 0x30;
}

inline int quadrature_direction(quadrature_s* ctx) {
	assert(ctx);

	if (ctx->dir == DIR_CW) {
		return 1;
	} else if (ctx->dir == DIR_CCW) {
		return -1;
	}

	return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
