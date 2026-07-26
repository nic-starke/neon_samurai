#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// This header used to rely on its includers having already pulled in the fixed
// width typedefs, so it could not be included on its own.
#include "system/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct quadrature {
	i8 dir;	 // Direction emitted by the last update: -1, 0 or +1
	u8 state; // Decoder state (private - see enum quad_state in quadrature.c)
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Processes the input from a quadrature encoder and returns
 * a direction.
 *
 * @param ctx Pointer to quadrature context.
 * @param ch_a Current value of channel A.
 * @param ch_b Current value of channel B.
 */
void quadrature_update(struct quadrature* ctx, uint ch_a, uint ch_b);

/**
 * @brief Get the last known direction of a quadrature encoder.
 *
 * @param ctx Pointer to quadrature context.
 * @return int 0 = stationary, 1 = CW, -1 = CCW.
 */
int quadrature_direction(struct quadrature* ctx);
