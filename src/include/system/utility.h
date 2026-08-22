/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <assert.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Static assertions, b = boolean expression, s = error message on failure
#define STATIC_ASSERT(b, s)		_Static_assert(b, s)

// Check if a value is within a range (inclusive)
#define IN_RANGE(x, min, max) (((x) >= (min)) && ((x) <= (max)))

// Clamp a value between a min and max
#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

// Get the min value of two values
#if !defined(MIN)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

// Get the max value of two values
#if !defined(MAX)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

// Get the number of elements in an array
#define COUNTOF(a) (sizeof(a) / sizeof(*(a)))

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
	A source span of zero has no scale factor to apply and would divide by zero.
	The span is reachable from stored configuration, so it has to be handled
	rather than assumed away: every input collapses onto the bottom of the new
	range, which is the one answer that stays inside it.
*/

__attribute__((__gnu_inline__)) static inline i32
convert_range_i32(i32 c, i32 omin, i32 omax, i32 nmin, i32 nmax) {
	const i32 or = omax - omin;
	const i32 nr = nmax - nmin;

	if (or == 0) {
		return nmin;
	}

	return (((c - omin) * nr) / or) + nmin;
}

__attribute__((__gnu_inline__)) static inline i16
convert_range_i16(i16 c, i16 omin, i16 omax, i16 nmin, i16 nmax) {
	const i16 or = omax - omin;
	const i16 nr = nmax - nmin;

	if (or == 0) {
		return nmin;
	}

	// The product needs 32 bits: both spans can reach 255, and 255 * 255
	// overflows a signed 16-bit intermediate.
	return (i16)((((i32)(c - omin) * nr) / or) + nmin);
}
