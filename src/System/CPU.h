/*
 * File: CPU.h ( 13th November 2021 )
 * Project: Muffin
 * Copyright 2021 Nic Starke (mail@bxzn.one)
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
#include <avr/interrupt.h>
#include <avr/cpufunc.h>

static inline void EnableInterrupts(void)
{
	sei();
}

static inline void DisableInterrupts(void)
{
	cli();
}

static inline volatile uint8_t IRQ_Save(void)
{
	volatile uint8_t irqFlags = SREG;
	DisableInterrupts();
	return irqFlags;
}

static inline void IRQ_Restore(volatile uint8_t IRQFlags)
{
	_MemoryBarrier();
	SREG = IRQFlags;
}