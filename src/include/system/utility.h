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
#define IN_RANGE(x, min, max) (((x) >= min) && ((x) <= max))

// Clamp a value between a min and max
#define CLAMP(x, min, max)		((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

// Get the min value of two values
#define MIN(a, b)							((a) < (b) ? (a) : (b))

// Get the max value of two values
#define MAX(a, b)							((a) > (b) ? (a) : (b))

// Get the number of elements in an array
#define COUNTOF(a)						(sizeof(a) / sizeof(*(a)))

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

__attribute__((__gnu_inline__)) static inline i32
convert_range_i32(i32 c, i32 omin, i32 omax, i32 nmin, i32 nmax) {
	const i32 or = omax - omin;
	const i32 nr = nmax - nmin;

	return (((c - omin) * nr) / or) + nmin;
}

__attribute__((__gnu_inline__)) static inline i16
convert_range_i16(i16 c, i16 omin, i16 omax, i16 nmin, i16 nmax) {
	const i16 or = omax - omin;
	const i16 nr = nmax - nmin;

	return (((c - omin) * nr) / or) + nmin;
}
