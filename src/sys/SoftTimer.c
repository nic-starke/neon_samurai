/*
 * File: Timer.c ( 28th November 2021 )
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

#include <avr/io.h>
#include <util/atomic.h>

#include "system/types.h"
#include "system/Interrupt.h"
#include "system/SoftTimer.h"
#include "Peripheral/Timer.h"

#define SOFT_TIMER (TCD0)

static volatile uint32_t
    mCurrentTick; // The current system tick count - interval is 1ms

/**
 * @brief Initialise the SoftTimer module.
 * This only needs to be called once during systme initialisation.
 * WARNING - Do not call this to try and initialise an instance of a soft timer!
 *
 */
void SoftTimer_Init(void) {
  sTimer_Type0Config timerConfig = {
      .pTimer       = &SOFT_TIMER,
      .ClockSource  = TC_CLKSEL_DIV1_gc,
      .Timer        = TIMER_TCD0,
      .WaveformMode = TC_WGMODE_NORMAL_gc,
  };

  Timer_Type0Init(&timerConfig);
  timerConfig.pTimer->PER =
      (uint16_t)32000; // system clock rate is 32 MHz - 32k is used here to
                       // obtain a 1ms timer interrupt
  Timer_EnableOverflowInterrupt(&SOFT_TIMER, PRIORITY_LOW);
}

/**
 * @brief The soft timer interrupt handler.
 *
 */
ISR(TCD0_OVF_vect) {
  ++mCurrentTick;
}

/**
 * @brief Get the current system uptime in milliseconds.
 *
 * @return uint32_t The uptime in milliseconds.
 */
uint32_t Millis(void) {
  uint32_t m = 0;
  // atomic block required due to 32 bit read (which is not single-cycle)
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    m = mCurrentTick;
  }
  return m;
}
