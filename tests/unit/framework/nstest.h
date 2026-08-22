/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
	A deliberately small test harness, so the unit suite has no dependency to
	fetch before it can run. Each test file is its own executable: it defines
	tests with TEST(), lists them inside NSTEST_MAIN, and CTest reports one
	result per file.

	A failing CHECK records the failure and returns from the enclosing test, so
	one broken assumption does not cascade into a page of unrelated noise. The
	remaining tests in the file still run.
*/

#define TEST(name) static void name(void)

#define NSTEST_MAIN(...)                                                       \
	int main(void) {                                                             \
		__VA_ARGS__                                                                \
		return nstest_report();                                                    \
	}

#define RUN(name)                                                              \
	do {                                                                         \
		nstest_current = #name;                                                    \
		nstest_tests++;                                                            \
		int before = nstest_failures;                                              \
		name();                                                                    \
		if (nstest_failures == before) {                                           \
			printf("  ok   %s\n", #name);                                            \
		}                                                                          \
	} while (0)

#define CHECK(cond)                                                            \
	do {                                                                         \
		nstest_checks++;                                                           \
		if (!(cond)) {                                                             \
			nstest_fail(__FILE__, __LINE__, #cond);                                  \
			return;                                                                  \
		}                                                                          \
	} while (0)

#define CHECK_EQ(actual, expected)                                             \
	do {                                                                         \
		nstest_checks++;                                                           \
		long long a_ = (long long)(actual);                                        \
		long long e_ = (long long)(expected);                                      \
		if (a_ != e_) {                                                            \
			nstest_fail_eq(__FILE__, __LINE__, #actual, a_, e_);                     \
			return;                                                                  \
		}                                                                          \
	} while (0)

#define CHECK_STR_EQ(actual, expected)                                         \
	do {                                                                         \
		nstest_checks++;                                                           \
		if (strcmp((actual), (expected)) != 0) {                                   \
			nstest_fail_str(__FILE__, __LINE__, #actual, (actual), (expected));      \
			return;                                                                  \
		}                                                                          \
	} while (0)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int				 nstest_failures = 0;
static int				 nstest_checks	 = 0;
static int				 nstest_tests		 = 0;
static const char* nstest_current	 = "";

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static inline void nstest_fail(const char* file, int line, const char* expr) {
	nstest_failures++;
	printf("  FAIL %s\n    %s:%d\n    expected true: %s\n", nstest_current, file,
				 line, expr);
}

static inline void nstest_fail_eq(const char* file, int line, const char* expr,
																	long long actual, long long expected) {
	nstest_failures++;
	printf("  FAIL %s\n    %s:%d\n    %s\n      actual   %lld\n      expected "
				 "%lld\n",
				 nstest_current, file, line, expr, actual, expected);
}

static inline void nstest_fail_str(const char* file, int line, const char* expr,
																	 const char* actual, const char* expected) {
	nstest_failures++;
	printf("  FAIL %s\n    %s:%d\n    %s\n      actual   \"%s\"\n      expected "
				 "\"%s\"\n",
				 nstest_current, file, line, expr, actual, expected);
}

static inline int nstest_report(void) {
	printf("%d test(s), %d check(s), %d failure(s)\n", nstest_tests,
				 nstest_checks, nstest_failures);
	return (nstest_failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
