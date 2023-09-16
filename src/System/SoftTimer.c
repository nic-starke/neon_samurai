/*
 * File: Timer.c ( 28th November 2021 )
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

#include <avr/io.h>
#include <util/atomic.h>

#include "DataTypes.h"
#include "Interrupt.h"
#include "SoftTimer.h"
#include "Timer.h"

#define SOFT_TIMER (TCD0)

static vu32 mCurrentTick;

void SoftTimer_Init(void)
{
    sTimer_Type0Config timerConfig = {
        .pTimer       = &SOFT_TIMER,
        .ClockSource  = TC_CLKSEL_DIV1_gc,
        .Timer        = TIMER_TCD0,
        .WaveformMode = TC_WGMODE_NORMAL_gc,
    };

    Timer_Type0Init(&timerConfig);
    timerConfig.pTimer->PER = (u16)32000;
    Timer_EnableOverflowInterrupt(&SOFT_TIMER, PRIORITY_LOW);
}

ISR(TCD0_OVF_vect)
{
    ++mCurrentTick;
}

u32 Millis(void)
{
    u32 m = 0;
    // atomic block required due to 32 bit read (which is not single-cycle)
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        m = mCurrentTick;
    }
    return m;
}