/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
	PRIORITY_OFF,
	PRIORITY_LOW,
	PRIORITY_MED,
	PRIORITY_HI,

	PRIORITY_NB,
} isr_priority_e;

typedef enum {
	ENDIAN_LSB,
	ENDIAN_MSB,
} endian_e;

typedef uint8_t		 u8;
typedef uint16_t	 u16;
typedef uint32_t	 u32;
typedef unsigned int uint;

typedef volatile uint8_t	  vu8;
typedef volatile uint16_t	  vu16;
typedef volatile uint32_t	  vu32;
typedef volatile unsigned int vuint;

typedef int8_t	i8;
typedef int16_t i16;
typedef int32_t i32;

typedef volatile int8_t	 vi8;
typedef volatile int16_t vi16;
typedef volatile int32_t vi32;

typedef float  f32;
typedef double f64;

typedef uintptr_t uptr;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
