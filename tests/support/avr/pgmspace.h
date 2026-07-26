/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2026) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*
	Host-build shim for <avr/pgmspace.h>.

	On AVR, PROGMEM places data in flash and the pgm_read_* macros issue LPM to
	read it back. The host has a single address space, so PROGMEM is a no-op and
	the accessors are plain dereferences.

	This shim is what lets a module keep its real PROGMEM annotations - and so
	stay correct on the device - while still being testable natively.
*/

#define PROGMEM /* nothing */

#define pgm_read_byte(addr)  (*(const unsigned char*)(addr))
#define pgm_read_word(addr)  (*(const unsigned short*)(addr))
#define pgm_read_dword(addr) (*(const unsigned long*)(addr))
#define pgm_read_ptr(addr)   (*(void* const*)(addr))
#define PSTR(s)              (s)
