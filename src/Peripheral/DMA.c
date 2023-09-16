/*
 * File: DMA.c ( 20th November 2021 )
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

#include "DMA.h"
#include "Types.h"

static inline void DMA_EnablePower(void)
{
	CLR_BIT(PR.PRGEN, PR_DMA_bm);
}

static inline void DMA_DisablePower(void)
{
	SET_BIT(PR.PRGEN, PR_DMA_bm);
}

static inline void DMA_EnableController(void)
{
	SET_BIT(DMA.CTRL, DMA_ENABLE_bm);
}

static inline void DMA_DisableController(void)
{
	CLR_BIT(DMA.CTRL, DMA_ENABLE_bm);
}

static inline void DMA_ResetController(void)
{
	SET_BIT(DMA.CTRL, DMA_RESET_bm);
}

static inline void DMA_SetDoubleBufferMode(DMA_DBUFMODE_t Mode)
{
	DMA.CTRL = (DMA.CTRL & ~DMA_CH_ENABLE_bm) | Mode;
}

/**
 * @brief Must be called once during system power-up and before using any DMA channels.
 *
 */
void DMA_Init(void)
{
	u8 flags = IRQ_DisableInterrupts();

	DMA_EnablePower();
	DMA_ResetController();
	DMA_EnableController();

	IRQ_EnableInterrupts(flags);
}

/**
 * @brief Enables and configures a DMA channel.
 * This does not start DMA transactions.
 *
 * @param pConfig Pointer to DMA channel configuration.
 */
void DMA_InitChannel(sDMA_ChannelConfig* pConfig)
{
	u8 flags = IRQ_DisableInterrupts();

	// Source
	pConfig->pChannel->SRCADDR0 = (pConfig->SrcAddress >> 0) & 0XFF;
	pConfig->pChannel->SRCADDR1 = (pConfig->SrcAddress >> 8) & 0XFF;
	pConfig->pChannel->SRCADDR2 = (pConfig->SrcAddress >> 16) & 0XFF;

	// CLR_BIT(pConfig->pChannel->ADDRCTRL, DMA_CH_SRCDIR_gm);
	SET_BIT(pConfig->pChannel->ADDRCTRL, pConfig->SrcAddressingMode);

	// CLR_BIT(pConfig->pChannel->ADDRCTRL, DMA_CH_SRCRELOAD_gm);
	SET_BIT(pConfig->pChannel->ADDRCTRL, pConfig->SrcReloadMode);

	// Destination
	pConfig->pChannel->DESTADDR0 = (pConfig->DstAddress >> 0) & 0XFF;
	pConfig->pChannel->DESTADDR1 = (pConfig->DstAddress >> 8) & 0XFF;
	pConfig->pChannel->DESTADDR2 = (pConfig->DstAddress >> 16) & 0XFF;

	// CLR_BIT(pConfig->pChannel->ADDRCTRL, DMA_CH_DESTDIR_gm);
	SET_BIT(pConfig->pChannel->ADDRCTRL, pConfig->DstAddressingMode);

	// CLR_BIT(pConfig->pChannel->ADDRCTRL, DMA_CH_DESTRELOAD_gm);
	SET_BIT(pConfig->pChannel->ADDRCTRL, pConfig->DstReloadMode);

	// DMA Config
	SET_BIT(pConfig->pChannel->TRIGSRC, pConfig->TriggerSource);

	// CLR_BIT(pConfig->pChannel->CTRLA, DMA_CH_BURSTLEN_gm);
	SET_BIT(pConfig->pChannel->CTRLA, pConfig->BurstLength);

	SET_BIT(pConfig->pChannel->TRFCNT, pConfig->BytesPerTransfer);

	if (pConfig->Repeats > 1)
	{
		CLR_BIT(pConfig->pChannel->CTRLA, DMA_CH_SINGLE_bm);
		SET_BIT(pConfig->pChannel->CTRLA, DMA_CH_REPEAT_bm);
	}
	else
	{
		CLR_BIT(pConfig->pChannel->CTRLA, DMA_CH_REPEAT_bm);
		SET_BIT(pConfig->pChannel->CTRLA, DMA_CH_SINGLE_bm);
	}
	pConfig->pChannel->REPCNT = pConfig->Repeats;

	CLR_BIT(pConfig->pChannel->CTRLB, DMA_CH_ERRINTLVL_gm | DMA_CH_TRNINTLVL_gm);
	SET_BIT(pConfig->pChannel->CTRLB,
			(pConfig->ErrInterruptPriority << DMA_CH_ERRINTLVL_gp) | (pConfig->InterruptPriority << DMA_CH_TRNINTLVL_gp));

	DMA_SetDoubleBufferMode(pConfig->DoubleBufferMode);

	IRQ_EnableInterrupts(flags);
}