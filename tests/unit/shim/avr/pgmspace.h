/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/*
	Host stand-in for avr-libc's pgmspace.h.

	Program space is a separate address space on the AVR and reads from it go
	through LPM. A host has one flat address space, so PROGMEM becomes nothing
	and the reads become ordinary dereferences. Tables under test keep their
	real contents and their real indexing - only where they live changes.
*/
#include <stdint.h>

#define PROGMEM
#define PGM_P			 const char*
#define PSTR(s)		 (s)

#define pgm_read_byte(p)	(*(const uint8_t*)(p))
#define pgm_read_word(p)	(*(const uint16_t*)(p))
#define pgm_read_dword(p) (*(const uint32_t*)(p))
