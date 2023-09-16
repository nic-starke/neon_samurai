/*
 * File: GPIO.h ( 7th November 2021 )
 * Project: Muffin
 * Copyright 2021 - 2021 Nic Starke (mail@bxzn.one)
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

#include <avr/io.h>
#include <avr/portpins.h>

typedef enum
{
    INPUT_PORT,
    OUTPUT_PORT,
} ePortDirection;

typedef enum
{
    PORT_MODE_TOTEM_POLE,
    PORT_MODE_BUS_KEEPER,
    PORT_MODE_PULL_DOWN,
    PORT_MODE_PULL_UP,
    PORT_MODE_WIRED_OR,
    PORT_MODE_WIRED_OR_PULL_DOWN,
    PORT_MODE_WIRED_AND,
    PORT_MODE_WIRED_AND_PULL_UP,
    PORT_MODE_INVERT_PIN,
    PORT_MODE_SLEW_RATE_LIMIT,
} ePortMode;

typedef enum
{
    SENSE_MODE_BOTH_EDGES,
    SENSE_MODE_RISING,
    SENSE_MODE_FALLING,
    SENSE_MODE_LEVEL_LOW,
} eSenseMode;

typedef enum
{
    LOW,
    HIGH,
} eLogicLevel;

inline void GPIO_SetPinDir(PORT_t* pPort, u8 Pin, ePortDirection Dir);
inline void GPIO_SetPinLevel(PORT_t* pPrt, u8 Pin, eLogicLevel Level);
inline void GPIO_InitialisePin(PORT_t* pPort, u8 Pin, ePortDirection Dir, eLogicLevel InitialState);