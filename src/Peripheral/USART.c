/*
 * File: USART.c ( 20th November 2021 )
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

#include "USART.h"
#include "Utility.h"
#include "Interrupt.h"

static inline u8 GetBitMask(USART_t* pUSART)
{
	if (pUSART == &USARTC0 || pUSART == &USARTD0 || pUSART == &USARTE0)
	{
		return PR_USART0_bm;
	}
	else if (pUSART == &USARTC1 || pUSART == &USARTD1)
	{
		return PR_USART1_bm;
	}
}

static inline void EnablePower(USART_t* pUSART)
{
	CLR_BIT(PR.PRPC, GetBitMask(pUSART));
}

static inline void DisablePower(USART_t* pUSART)
{
	SET_BIT(PR.PRPC, GetBitMask(pUSART));
}

static inline void DisableTX(USART_t* pUSART)
{
	CLR_BIT(pUSART->CTRLB, USART_TXEN_bm);
}

static inline void EnableTX(USART_t* pUSART)
{
	SET_BIT(pUSART->CTRLB, USART_TXEN_bm);
}

static inline void DisableRX(USART_t* pUSART)
{
	CLR_BIT(pUSART->CTRLB, USART_RXEN_bm);
}

static inline void EnableRX(USART_t* pUSART)
{
	SET_BIT(pUSART->CTRLB, USART_RXEN_bm);
}

static inline void SetMode(USART_t* pUSART, USART_CMODE_t Mode)
{
	pUSART->CTRLC = (pUSART->CTRLC & (~USART_CMODE_gm)) | Mode;
}

// ----------------- //

void USART_Init(void)
{
	// u8 flags = IRQ_DisableInterrupts();

	// No global init required.

	// IRQ_EnableInterrupts(flags);
}

void USART_InitModule(sUSART_ModuleConfig* pConfig)
{
	u8 flags = IRQ_DisableInterrupts();

	EnablePower(pConfig->pUSART);

	DisableRX(pConfig->pUSART);
	DisableTX(pConfig->pUSART);

	// GPIO Config here

	SetMode(pConfig->pUSART, USART_CMODE_MSPI_gc); // Mode is fixed to SPI Master for this driver

	IRQ_EnableInterrupts(flags);
}