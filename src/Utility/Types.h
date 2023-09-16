/*
 * File: Types.h ( 20th November 2021 )
 * Project: Muffin
 * Copyright 2021 Nic Starke (mail@bxzn.one)
 * -----
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#pragma once

#include <stdbool.h>
#include <stdint-gcc.h>
#include <stddef.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;

typedef volatile uint8_t  vu8;
typedef volatile uint16_t vu16;
typedef volatile uint32_t vu32;

typedef volatile int8_t  vs8;
typedef volatile int16_t vs16;
typedef volatile int32_t vs32;

typedef enum
{
    DO_MSB_FIRST,
    DO_LSB_FIRST,
} eDataOrder;

typedef enum
{
    LOW,
    HIGH,
} eLogicLevel;