/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2025) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <stdlib.h>

#include "hal/adc.h"
#include "system/rng.h"
#include "hal/signature.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static uint32_t seed = 0; // Seed for the random number generator

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void rng_init(void) {
	NVM_PROD_SIGNATURES_t prod_sig;
	signature_read(&prod_sig);

	uint32_t s = ((uint32_t)prod_sig.COORDX0 << 24) |
							 ((uint32_t)prod_sig.COORDX1 << 16) |
							 ((uint32_t)prod_sig.COORDY0 << 8) | ((uint32_t)prod_sig.COORDY1);

	s ^= ((uint32_t)prod_sig.LOTNUM0 << 24) | ((uint32_t)prod_sig.LOTNUM1 << 16) |
			 ((uint32_t)prod_sig.LOTNUM2 << 8) | ((uint32_t)prod_sig.LOTNUM3);

	s ^= ((uint32_t)prod_sig.LOTNUM4 << 24) | ((uint32_t)prod_sig.LOTNUM5 << 16);

	// The raw sensor reading, not a converted temperature: the low bits carry
	// the conversion noise that a calibrated value rounds away, and it avoids
	// linking the soft-float library.
	adc_channel_config_internal(ADC_CH0, ADC_CH_MUXINT_TEMP);
	s ^= adc_get_sample(ADC_CH0);

	// srand() takes unsigned int, 16 bits on this target, so fold the upper
	// half in rather than letting it be truncated away.
	const unsigned int folded = (unsigned int)(s ^ (s >> 16));

	srand(folded);
	seed = folded;
}

uint32_t rng_get_seed(void) {
	return seed;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
