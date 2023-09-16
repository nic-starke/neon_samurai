/*
 * File: GPIO.h ( 20th November 2021 )
 * Project: Muffin
 * Copyright 2021 bxzn (mail@bxzn.one)
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

#include "DataTypes.h"
#include "Utility.h"

typedef enum
{
    GPIO_INPUT,
    GPIO_OUTPUT,
} eGPIO_Direction;

#define PIN_MASK(x) (1u << ((x)&0x07))

static inline void GPIO_SetPinDirection(PORT_t* Port, u8 Pin, eGPIO_Direction Direction)
{
    if (Direction == GPIO_INPUT)
    {
        Port->DIRCLR = PIN_MASK(Pin);
    }
    else if (Direction == GPIO_OUTPUT)
    {
        Port->DIRSET = PIN_MASK(Pin);
    }
}

static inline void GPIO_SetPinLevel(PORT_t* Port, u8 Pin, eLogicLevel Level)
{
    if (Level == HIGH)
    {
        Port->OUTSET = PIN_MASK(Pin);
    }
    else if (Level == LOW)
    {
        Port->OUTCLR = PIN_MASK(Pin);
    }
}

static inline void GPIO_SetPinMode(PORT_t* pPort, u8 Pin, PORT_OPC_t Mode)
{
    vu8* regCTRL = (&pPort->PIN0CTRL + PIN_MASK(Pin));

    *regCTRL &= PORT_ISC_gm;
    SET_REG(*regCTRL, Mode);
}

static inline u8 GPIO_GetPinLevel(PORT_t* pPort, u8 Pin)
{
    return pPort->IN & PIN_MASK(Pin);
}