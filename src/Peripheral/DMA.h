/*
 * File: DMA.h ( 13th November 2021 )
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

#include "Utility.h"
#include "Types.h"
#include "Interrupt.h"

typedef struct
{
	DMA_CH_t*			pChannel;
	u8					Repeats; // <= 1 for single shot mode
	u16					BytesPerTransfer;
	DMA_CH_BURSTLEN_t	BurstLength;
	DMA_CH_TRIGSRC_t	TriggerSource;
	DMA_DBUFMODE_t		DoubleBufferMode;
	DMA_CH_TRNINTLVL_t	InterruptPriority;
	DMA_CH_ERRINTLVL_t	ErrInterruptPriority;
	uintptr_t			SrcAddress;
	DMA_CH_SRCDIR_t		SrcAddressingMode;
	DMA_CH_SRCRELOAD_t	SrcReloadMode;
	uintptr_t			DstAddress;
	DMA_CH_DESTDIR_t	DstAddressingMode;
	DMA_CH_DESTRELOAD_t DstReloadMode;
} sDMA_ChannelConfig;

static inline DMA_CH_t* DMA_GetChannelPointer(u8 ChannelNumber)
{
	(DMA_CH_t*)((uintptr_t)(&DMA.CH0) + (sizeof(DMA_CH_t) * ChannelNumber));
}

/**
 * @brief Start DMA transactions for channel
 *
 * @param pDMA Pointer to DMA channel
 */
static inline void DMA_EnableChannel(DMA_CH_t* pDMA)
{
	SET_BIT(pDMA->CTRLA, DMA_CH_ENABLE_bm);
}

/**
 * @brief Stop DMA transactions for channel
 *
 * @param pDMA  Pointer to DMA channel
 */
static inline void DMA_DisableChannel(DMA_CH_t* pDMA)
{
	CLR_BIT(pDMA->CTRLA, DMA_CH_ENABLE_bm);
}

/**
 * @brief Reset the DMA channel
 *
 * @param pDMA Pointer to DMA channel
 */
static inline void DMA_ResetChannel(DMA_CH_t* pDMA)
{
	SET_BIT(pDMA->CTRLA, DMA_CH_RESET_bm);
}

void DMA_Init(void);
void DMA_InitChannel(sDMA_ChannelConfig* pConfig);