/*
 * File: Interrupt.h ( 20th November 2021 )
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

#include <avr/interrupt.h>
#include <avr/cpufunc.h>

#include "DataTypes.h"

typedef enum
{
    PRIORITY_OFF,
    PRIORITY_LOW,
    PRIORITY_MED,
    PRIORITY_HI,
} eInterruptPriority;

static inline u8 IRQ_DisableInterrupts(void)
{
    vu8 flags = SREG;
    cli();
    return flags;
}

static inline void IRQ_EnableInterrupts(vu8 Flags)
{
    _MemoryBarrier();
    SREG = Flags;
}