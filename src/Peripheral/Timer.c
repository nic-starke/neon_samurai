/*
 * File: Timer.c ( 21st November 2021 )
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

#include <avr/pgmspace.h>

#include "DataTypes.h"
#include "Interrupt.h"
#include "Timer.h"
#include "Utility.h"

static inline u8 GetTimerBitmask(eTimer_Peripheral Timer)
{
    switch (Timer)
    {
        case TIMER_TCC0:
        case TIMER_TCC2:
        case TIMER_TCD0:
        case TIMER_TCD2:
        case TIMER_TCE0:
        {
            return PR_TC0_bm;
        }
        case TIMER_TCC1:
        case TIMER_TCD1:
        {
            return PR_TC1_bm;
        }

        default: return 0;
    }

    return 0;
}

static inline u8 GetTimerPowerPort(eTimer_Peripheral Timer)
{
    switch (Timer)
    {
        case TIMER_TCC0:
        case TIMER_TCC1:
        case TIMER_TCC2:
        {
            return PR.PRPC;
        }

        case TIMER_TCD0:
        case TIMER_TCD1:
        case TIMER_TCD2:
        {
            return PR.PRPD;
        }

        case TIMER_TCE0:
        {
            return PR.PRPE;
        }

        default:
        {
            return 0;
        }
    }

    return 0;
}

static inline void EnablePower(eTimer_Peripheral Timer)
{
    vu8      pp      = GetTimerPowerPort(Timer);
    const u8 bitmask = GetTimerBitmask(Timer);

    SET_REG(pp, bitmask);
}

static inline void DisablePower(eTimer_Peripheral Timer)
{
    vu8      pp      = GetTimerPowerPort(Timer);
    const u8 bitmask = GetTimerBitmask(Timer);

    SET_REG(pp, bitmask);
}

static inline void SetWGM(TC0_t* pTC, TC_WGMODE_t Mode)
{
    pTC->CTRLB = (pTC->CTRLB & ~TC0_WGMODE_gm) | Mode;
}

static inline void SetPrescaler(TC0_t* pTC, TC_CLKSEL_t ClockSel)
{
    pTC->CTRLA = (pTC->CTRLA & ~TC0_CLKSEL_gm) | ClockSel;
}

void Timer_Type0Init(const sTimer_Type0Config* pConfig)
{
    const u8 flags = IRQ_DisableInterrupts();
    EnablePower(pConfig->Timer);

    SetWGM(pConfig->pTimer, pConfig->WaveformMode);
    SetPrescaler(pConfig->pTimer, pConfig->ClockSource);

    IRQ_EnableInterrupts(flags);
}