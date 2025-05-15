/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2025) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

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
	// Read the production signature row
	NVM_PROD_SIGNATURES_t prod_sig;
	signature_read(&prod_sig);

	// Use the wafer coordinates and lot number as the seed
	uint32_t s = (prod_sig.COORDX0 << 24) | (prod_sig.COORDX1 << 16) |
							 (prod_sig.COORDY0 << 8) | (prod_sig.COORDY1);
	s ^= (prod_sig.LOTNUM0 << 24) | (prod_sig.LOTNUM1 << 16) |
			 (prod_sig.LOTNUM2 << 8) | (prod_sig.LOTNUM3);
	s ^= (prod_sig.LOTNUM4 << 24) | (prod_sig.LOTNUM5 << 16);

	// Read the temperature sensor value
	float temperature = adc_read_temperature_float();

	// Combine the temperature with the seed
	s ^= (uint32_t)(temperature * 100); // Scale temperature to an integer

	// Seed the random number generator
	srand(s);

	// Store the seed for later use
	seed = s;
}

uint32_t rng_get_seed(void) {
	return seed;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
