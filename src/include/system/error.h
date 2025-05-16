/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <assert.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Checks the value of an error code, jumps to l if it is not == success.
 * @warning DO NOT PASS/CALL FUNCTIONS WITH THIS MACRO.
 * @param s Error/status code to check.
 * @param l Label to goto.
 */
#define EXIT_ON_ERR(s, l)                                                      \
	do {                                                                         \
		if (s != SUCCESS) {                                                        \
			goto l;                                                                  \
		}                                                                          \
	} while (0)

/**
 * @brief Checks the value of an error code and returns if it is not == success.
 * @warning DO NOT PASS/CALL FUNCTIONS WITH THIS MACRO.
 * @param s Error/status code to check.
 */
#define RETURN_ON_ERR(s)                                                       \
	do {                                                                         \
		if (s != SUCCESS) {                                                        \
			return s;                                                                \
		}                                                                          \
	} while (0)

/**
 * @brief Checks if a pointer is NULL, jumps to l if true.
 * @warning DO NOT PASS/CALL FUNCTIONS WITH THIS MACRO.
 * @param p Pointer to check against null.
 * @param l Label to goto.
 */
#define EXIT_IF_NULL(p, l)                                                     \
	do {                                                                         \
		if (p == NULL) {                                                           \
			goto l;                                                                  \
		}                                                                          \
	} while (0)

/**
 * @brief Checks if a pointer is null, returns ERR_NULL_PTR if true.
 * @warning DO NOT PASS/CALL FUNCTIONS WITH THIS MACRO.
 * @param p Pointer to check against null.
 * @param l Label to goto.
 */
#define RETURN_ERR_IF_NULL(p)                                                  \
	do {                                                                         \
		if (p == NULL) {                                                           \
			return ERR_NULL_PTR;                                                     \
		}                                                                          \
	} while (0)

/**
 * @brief Checks if a pointer is null, returns if true.
 * @warning DO NOT PASS/CALL FUNCTIONS WITH THIS MACRO.
 * @param p Pointer to check against null.
 */
#define RETURN_IF_NULL(p)                                                      \
	do {                                                                         \
		if (p == NULL) {                                                           \
			return;                                                                  \
		}                                                                          \
	} while (0)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum return_code {
	SUCCESS							= 0,
	ERR_BAD_PARAM				= -1,
	ERR_NULL_PTR				= -2,
	ERR_UNSUPPORTED			= -3,
	ERR_STUB						= -4,
	ERR_NO_MEM					= -5,
	ERR_DUPLICATE				= -6,
	ERR_NOT_IMPLEMENTED = -7,
	ERR_BAD_MSG					= -8,
	ERR_NO_ANIMATION		= -9,
	ERR_ANIMATION_BUSY		= -10,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
