/*
 * File: Timer.h ( 28th November 2021 )
 * Project: Muffin
 * Copyright 2021 Nicolaus Starke  
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

#include "DataTypes.h"

typedef enum
{
    TIMER_STOPPED,
    TIMER_RUNNING,

    NUM_TIMER_STATES,
} eSoftTimer_State;

typedef struct
{
    u32              StartTime;
    eSoftTimer_State State;
} sSoftTimer;

void SoftTimer_Init(void);
u32  Millis(void);

/**
 * @brief Start a soft timer.
 * 
 * @param pTimer A pointer to the soft timer object
 */
static inline void SoftTimer_Start(sSoftTimer* pTimer)
{
    pTimer->State     = TIMER_RUNNING;
    pTimer->StartTime = Millis();
}

/**
 * @brief Get the current elapsed time in milliseconds of a timer.
 * 
 * @param pTimer A pointer to the soft timer object.
 * @return u32 The elapsed time in milliseconds.
 */
static inline u32 SoftTimer_Elapsed(sSoftTimer* pTimer)
{
    return Millis() - pTimer->StartTime;
}

/**
 * @brief Stops a soft timer.
 * 
 * @param pTimer A pointer to the soft timer object.
 * @return u32 The total elapsed time of the timer, in milliseconds.
 */
static inline u32 SoftTimer_Stop(sSoftTimer* pTimer)
{
    if (pTimer->State == TIMER_RUNNING)
    {
        pTimer->State = TIMER_STOPPED;
    }

    return SoftTimer_Elapsed(pTimer);
}