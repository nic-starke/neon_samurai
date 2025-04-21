/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2025) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* Reads the production signature row (calibration data) from NVM. */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include <avr/io.h>
#include <avr/cpufunc.h>
#include "hal/signature.h"
#include "system/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PRODUCTION_SIGNATURE_SIZE sizeof(NVM_PROD_SIGNATURES_t)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static uint8_t nvm_read_prod_sig_byte(uint16_t address);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void signature_read(NVM_PROD_SIGNATURES_t* prod_sig) {
	uint8_t* dest_ptr = (uint8_t*)prod_sig; // Treat the struct as a byte array
	uint16_t i;

	for (i = 0; i < PRODUCTION_SIGNATURE_SIZE; ++i) {
		dest_ptr[i] = nvm_read_prod_sig_byte(i);
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Reads a single byte from the production signature row (calibration row).
 *
 * @param address The byte offset within the production signature row (0-63).
 * @return uint8_t The byte read from the specified address.
 */
static uint8_t nvm_read_prod_sig_byte(uint16_t address) {
	uint8_t byte_value;

	// Wait until NVM controller is ready (not busy).
	while (NVM.STATUS & NVM_NVMBUSY_bm) {
		; // Wait
	}

	// Set the NVM command to read the calibration/production signature row.
	// This requires Configuration Change Protection (CCP).
	_PROTECTED_WRITE(NVM.CMD, NVM_CMD_READ_CALIB_ROW_gc);

	// Use inline assembly to perform the Load Program Memory (LPM) instruction.
	// The Z-pointer register (R31:R30) must point to the address (byte offset)
	// within the signature row to be read.
	__asm__ __volatile__ (
		"lpm %0, Z\n"
		: "=r" (byte_value)  // Output: store result in byte_value register
		: "z" (address)      // Input: load 'address' into Z-pointer (R31:R30)
	);

	// After the LPM instruction, it's good practice to clear the NVM command.
	// This also requires CCP.
	_PROTECTED_WRITE(NVM.CMD, NVM_CMD_NO_OPERATION_gc);

	return byte_value;
}
