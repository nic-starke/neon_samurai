/*
 * File: DMA.c ( 6th November 2021 )
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

#include <avr/io.h>
#include <avr/interrupt.h>

#include "DMA.h"
#include "HardwareDefines.h"
#include "Interrupt.h"
#include "Utils.h"

void SYS_EnableDMA(void)
{
	vu8 oldSREG = IRQ_DisableGlobalInterrupts();

	// Reset and enable DMA controller
	DMA.CTRL = DMA_RESET_bm;
	DMA.CTRL = DMA_ENABLE_bm;

	IRQ_RestoreSREG(oldSREG);
}

void DMA_SetChannelConfig(DMA_CH_t* pDMA, sDMAChannelConfig* pConfig)
{
	vu8 oldSREG = IRQ_DisableGlobalInterrupts();

	// Source
	pDMA->SRCADDR0 = (pConfig->SrcAddr >> 0) & 0XFF;
	pDMA->SRCADDR1 = (pConfig->SrcAddr >> 8) & 0XFF;
	pDMA->SRCADDR2 = (pConfig->SrcAddr >> 16) & 0XFF;

	// CLR_BIT(pDMA->ADDRCTRL, DMA_CH_SRCDIR_gm);
	SET_BIT(pDMA->ADDRCTRL, pConfig->SrcAddrMode);

	// CLR_BIT(pDMA->ADDRCTRL, DMA_CH_SRCRELOAD_gm);
	SET_BIT(pDMA->ADDRCTRL, pConfig->SrcReloadMode);

	// Destination
	pDMA->DESTADDR0 = (pConfig->DstAddr >> 0) & 0XFF;
	pDMA->DESTADDR1 = (pConfig->DstAddr >> 8) & 0XFF;
	pDMA->DESTADDR2 = (pConfig->DstAddr >> 16) & 0XFF;

	// CLR_BIT(pDMA->ADDRCTRL, DMA_CH_DESTDIR_gm);
	SET_BIT(pDMA->ADDRCTRL, pConfig->DstAddrMode);

	// CLR_BIT(pDMA->ADDRCTRL, DMA_CH_DESTRELOAD_gm);
	SET_BIT(pDMA->ADDRCTRL, pConfig->DstReloadMode);

	// DMA Config
	SET_BIT(pDMA->TRIGSRC, pConfig->TriggerSource);

	// CLR_BIT(pDMA->CTRLA, DMA_CH_BURSTLEN_gm);
	SET_BIT(pDMA->CTRLA, pConfig->BurstLength);

	SET_BIT(pDMA->TRFCNT, pConfig->BlockTransferCount);

	if (pConfig->RepeatCount > 1)
	{
		CLR_BIT(pDMA->CTRLA, DMA_CH_SINGLE_bm);
		SET_BIT(pDMA->CTRLA, DMA_CH_REPEAT_bm);
	}
	else
	{
		CLR_BIT(pDMA->CTRLA, DMA_CH_REPEAT_bm);
		SET_BIT(pDMA->CTRLA, DMA_CH_SINGLE_bm);
	}
	pDMA->REPCNT = pConfig->RepeatCount;

	CLR_BIT(pDMA->CTRLB, DMA_CH_ERRINTLVL_gm | DMA_CH_TRNINTLVL_gm);
	SET_BIT(pDMA->CTRLB, (pConfig->IntPriority << DMA_CH_ERRINTLVL_gp) | (pConfig->IntPriority << DMA_CH_TRNINTLVL_gp));

	IRQ_RestoreSREG(oldSREG);
}

void DMA_Start(DMA_CH_t* pDMA)
{
	SET_BIT(pDMA->CTRLA, DMA_CH_ENABLE_bm);
}

void DMA_Stop(DMA_CH_t* pDMA)
{
	CLR_BIT(pDMA->CTRLA, DMA_CH_ENABLE_bm);
}

void DMA_EnableDoubleBuffer(DMA_DBUFMODE_t Mode)
{
    DMA.CTRL = DMA_CH_ENABLE_bm | Mode;	
}