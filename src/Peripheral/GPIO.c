/*
 * File: GPIO.c ( 7th November 2021 )
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

#include <avr/io.h>
#include "GPIO.h"
#include "Peripheral.h"

#define PIN_MASK(pin)   (1U << (pin & 0x07))

inline void GPIO_SetPinDir(PORT_t* pPort, u8 Pin, ePortDirection Dir)
{
    switch(Dir)
    {
        case INPUT_PORT:
            pPort->DIRSET = PIN_MASK(Pin);
            break;
        case OUTPUT_PORT:
            pPort->DIRCLR = PIN_MASK(Pin);
            break;

        default:
            break;
    }
}

inline void GPIO_SetPinLevel(PORT_t* pPort, u8 Pin, eLogicLevel Level)
{
    if (Level == HIGH)
    {
        pPort->OUTSET = PIN_MASK(Pin);
    }
    else
    {
        pPort->OUTCLR = PIN_MASK(Pin);
    }
}

inline void GPIO_InitialisePin(PORT_t* pPort, u8 Pin, ePortDirection Dir, eLogicLevel InitialState)
{
    GPIO_SetPinDir(pPort, Pin, Dir);
    GPIO_SetPinLevel(pPort, Pin, InitialState);
}