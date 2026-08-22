/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"
#include "system/diag.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static u16 counters[DIAG_NB];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void diag_count(enum diag_counter counter) {
	if (counter >= DIAG_NB) {
		return;
	}

	if (counters[counter] < UINT16_MAX) {
		counters[counter]++;
	}
}

u16 diag_get(enum diag_counter counter) {
	if (counter >= DIAG_NB) {
		return 0;
	}

	return counters[counter];
}

u32 diag_total(void) {
	u32 total = 0;

	for (u8 i = 0; i < (u8)DIAG_NB; i++) {
		total += counters[i];
	}

	return total;
}

void diag_reset(void) {
	for (u8 i = 0; i < (u8)DIAG_NB; i++) {
		counters[i] = 0;
	}
}
