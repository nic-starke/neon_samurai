/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2026) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*
	Minimal assertion helpers for the host test build.

	Deliberately dependency-free: no network fetch at configure time, nothing to
	vendor, and the whole thing is readable in one sitting. If the suite outgrows
	this, Unity (ThrowTheSwitch) is the natural next step and the submodule
	already carries a cmake module for it.
*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <stdio.h>
#include <string.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Globals ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

extern int tests_run;
extern int tests_failed;
extern int current_test_failed;

#define TEST_GLOBALS                                                           \
	int tests_run						= 0;                                                 \
	int tests_failed				= 0;                                                 \
	int current_test_failed = 0

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Assertions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define CHECK(cond)                                                            \
	do {                                                                         \
		if (!(cond)) {                                                             \
			printf("    FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);               \
			current_test_failed = 1;                                                 \
		}                                                                          \
	} while (0)

#define CHECK_EQ_INT(actual, expected)                                         \
	do {                                                                         \
		long _a = (long)(actual);                                                  \
		long _e = (long)(expected);                                                \
		if (_a != _e) {                                                            \
			printf("    FAIL %s:%d: %s == %s (got %ld, want %ld)\n", __FILE__,       \
						 __LINE__, #actual, #expected, _a, _e);                            \
			current_test_failed = 1;                                                 \
		}                                                                          \
	} while (0)

#define RUN_TEST(fn)                                                           \
	do {                                                                         \
		current_test_failed = 0;                                                   \
		tests_run++;                                                               \
		printf("  %-52s", #fn);                                                    \
		fflush(stdout);                                                            \
		fn();                                                                      \
		if (current_test_failed) {                                                 \
			tests_failed++;                                                          \
			printf("  <-- FAILED\n");                                                \
		} else {                                                                   \
			printf("ok\n");                                                          \
		}                                                                          \
	} while (0)

#define TEST_SUMMARY()                                                         \
	(printf("\n%d run, %d failed\n", tests_run, tests_failed),                    \
	 tests_failed == 0 ? 0 : 1)
