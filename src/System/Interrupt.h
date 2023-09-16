/*
 * File: Interrupt.h ( 7th November 2021 )
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

#include <avr/interrupt.h>
#include <avr/cpufunc.h>

#include "Types.h"

/**
 * @brief Disables interrupts globally.
 *
 * @return u8 Current value of SREG
 */
static inline u8 IRQ_DisableGlobalInterrupts(void)
{
	u8 oldSREG = SREG;
	cli();
	return oldSREG;
}

/**
 * @brief Restores state of SREG
 *
 * @param newSREG New value for SREG.
 */
static inline void IRQ_RestoreSREG(vu8 newSREG)
{
	/* As this is an inline function the assignment operation may be reordered
	 * even with the volatile qualifier! A memory barrier is required, see
	 * https://www.nongnu.org/avr-libc/user-manual/optimization.html#optim_code_reorder
	 * for more information */
	_MemoryBarrier();
	SREG = newSREG;
}