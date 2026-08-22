#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2026) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
	EEPROM access for this part, replacing the avr-libc eeprom_* family.

	Two reasons it exists. avr-libc leaves the CCP timed sequence that triggers
	each NVM command unguarded, so an interrupt landing in that four-cycle
	window voids the unlock and the command is dropped with no indication. And
	it issues one Erase & Write Page per byte, so writing a block costs a full
	page erase-and-write for every byte in it.

	These routines protect the CCP sequence and work a page at a time.
*/

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Byte offset of an EEMEM object or member within the EEPROM.
#define EE_ADDR(member) ((u16)(uintptr_t)&(member))

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Read from the EEPROM.
 *
 * @param addr Byte offset within the EEPROM.
 * @param dst Destination buffer.
 * @param len Number of bytes.
 */
void hal_eeprom_read(u16 addr, void* dst, u16 len);

/**
 * @brief Write to the EEPROM, skipping bytes that already hold the value.
 *
 * @param addr Byte offset within the EEPROM.
 * @param src Source buffer.
 * @param len Number of bytes.
 * @return int SUCCESS, or ERR_BAD_PARAM if the range leaves the EEPROM.
 */
int hal_eeprom_update(u16 addr, const void* src, u16 len);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
