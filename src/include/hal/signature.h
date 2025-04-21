#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2025) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h> // For NVM_PROD_SIGNATURES_t

/**
 * @brief Reads the production signature row from NVM into the provided struct.
 *
 * @param prod_sig Pointer to the destination NVM_PROD_SIGNATURES_t structure.
 */
void signature_read(NVM_PROD_SIGNATURES_t* prod_sig);
