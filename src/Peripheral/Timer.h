/*
 * File: Timer.h ( 21st November 2021 )
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
#include "Interrupt.h"
#include "Utility.h"

typedef enum
{
    TIMER_CHANNEL_A,
    TIMER_CHANNEL_B,
    TIMER_CHANNEL_C,
    TIMER_CHANNEL_D,
} eTimer_Channel;

typedef enum
{
    TIMER_TCC0,
    TIMER_TCC1,
    TIMER_TCC2,
    TIMER_TCD0,
    TIMER_TCD1,
    TIMER_TCD2,
    TIMER_TCE0,

    NUM_TIMER_PERIPHERALS,
} eTimer_Peripheral;

typedef struct
{
    eTimer_Peripheral Timer;
    TC0_t*            pTimer;
    TC_WGMODE_t       WaveformMode;
    TC_CLKSEL_t       ClockSource;
} sTimer_Type0Config;

// typedef struct
// {
//     eTimer_Peripheral Timer;
//     TC2_CLKSEL_t ClockSource;
//     TC2_BYTEM_t ByteMode;
//     TC2_HUNFINTLVL_t HighByteUnderflowInterrupPriority;
//     TC2_LUNFINTLVL_t LowByteUnderflowInterruptPriority;

//     eTimer_Channel CompareChannel;
//     union
//     {
//         TC2_LCMPAINTLVL_t LowByteCCA_InterruptPriority;
//         TC2_LCMPBINTLVL_t LowByteCCB_InterruptPriority;
//         TC2_LCMPCINTLVL_t LowByteCCC_InterruptPriority;
//         TC2_LCMPDINTLVL_t LowByteCCD_InterruptPriority;
//     } uChannelLowByteInterruptPriority;

//     TC2_CMDEN_t CommandEnable;
// } sTimer_Type2Config;

static inline void Timer_EnableChannelInterrupt(TC0_t* pTC, eTimer_Channel Channel, eInterruptPriority Priority)
{
    const u8 shift = Channel << 1;
    const u8 mask  = (TC0_CCAINTLVL_gm) << shift;
    pTC->INTCTRLB  = (pTC->INTCTRLB & ~mask) | (Priority << shift);
}

static inline void Timer_DisableChannelInterrupt(TC0_t* pTC, eTimer_Channel Channel)
{
    const u8 shift = Channel << 2;
    const u8 mask  = (TC0_CCAINTLVL_gm) << shift;
    CLR_REG(pTC->INTCTRLB, mask);
}

static inline void Timer_EnableOverflowInterrupt(TC0_t* pTC, eInterruptPriority Priority)
{
    pTC->INTCTRLA = (pTC->INTCTRLA & ~TC0_OVFINTLVL_gm) | Priority;
}

static inline void Timer_DisableOverflowInterrupt(TC0_t* pTC)
{
    CLR_REG(pTC->INTCTRLA, TC0_OVFINTLVL_gm);
}

void Timer_Type0Init(const sTimer_Type0Config* pConfig);