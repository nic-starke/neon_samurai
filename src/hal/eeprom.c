/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2026) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string.h>
#include <avr/io.h>
#include <util/atomic.h>

#include "hal/eeprom.h"
#include "system/error.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define EE_MAPPED(addr) ((volatile u8*)(MAPPED_EEPROM_START + (addr)))

// page_update() masks the address with EEPROM_PAGE_SIZE - 1 and carries the
// chunk length in a u8.
_Static_assert((EEPROM_PAGE_SIZE & (EEPROM_PAGE_SIZE - 1)) == 0,
							 "EEPROM page size must be a power of two");
_Static_assert(EEPROM_PAGE_SIZE <= 255, "a page must fit a u8 chunk length");

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void nvm_wait(void);
static void nvm_exec(u8 command);
static void page_update(u16 addr, const u8* src, u8 len);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void hal_eeprom_read(u16 addr, void* dst, u16 len) {
	assert(dst);

	if ((addr >= EEPROM_SIZE) || (len > (u16)(EEPROM_SIZE - addr))) {
		return;
	}

	nvm_wait();
	NVM.CTRLB |= NVM_EEMAPEN_bm;

	memcpy(dst, (const void*)EE_MAPPED(addr), len);
}

int hal_eeprom_update(u16 addr, const void* src, u16 len) {
	assert(src);

	if ((addr >= EEPROM_SIZE) || (len > (u16)(EEPROM_SIZE - addr))) {
		return ERR_BAD_PARAM;
	}

	const u8* bytes = (const u8*)src;

	while (len > 0) {
		const u8 offset = (u8)(addr & (EEPROM_PAGE_SIZE - 1));
		u8			 chunk	= (u8)(EEPROM_PAGE_SIZE - offset);

		if (chunk > len) {
			chunk = (u8)len;
		}

		page_update(addr, bytes, chunk);

		addr += chunk;
		bytes += chunk;
		len -= chunk;
	}

	nvm_wait();
	NVM.CMD = NVM_CMD_NO_OPERATION_gc;

	return SUCCESS;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void nvm_wait(void) {
	while (NVM.STATUS & NVM_NVMBUSY_bm) {
		;
	}
}

/*
	CCP unlocks protected registers for four cycles only. An interrupt taken
	between the unlock and the CMDEX write costs the unlock, and the command is
	then discarded without setting any error flag - the write simply does not
	happen. Only these two instructions need the guard, so interrupt latency is
	unaffected by the milliseconds the write itself takes.
*/
static void nvm_exec(u8 command) {
	nvm_wait();
	NVM.CMD = command;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		CCP				= CCP_IOREG_gc;
		NVM.CTRLA = NVM_CMDEX_bm;
	}
}

/*
	The page buffer tags each byte written into it, and Erase & Write only
	touches tagged locations, so a partial page leaves the rest of that page
	alone.
*/
static void page_update(u16 addr, const u8* src, u8 len) {
	nvm_wait();
	NVM.CTRLB |= NVM_EEMAPEN_bm;

	if (memcmp((const void*)EE_MAPPED(addr), src, len) == 0) {
		return;
	}

	// Stale tags from an earlier partial load would be programmed too.
	nvm_exec(NVM_CMD_ERASE_EEPROM_BUFFER_gc);
	nvm_wait();

	NVM.CMD = NVM_CMD_LOAD_EEPROM_BUFFER_gc;

	for (u8 i = 0; i < len; ++i) {
		*EE_MAPPED(addr + i) = src[i];
	}

	NVM.ADDR0 = (u8)(addr & 0xFF);
	NVM.ADDR1 = (u8)((addr >> 8) & 0x1F);
	NVM.ADDR2 = 0;

	nvm_exec(NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc);
	nvm_wait();
}
