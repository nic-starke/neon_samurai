/*
 * File: DMA.h ( 6th November 2021 )
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

#pragma once

#include <avr/io.h>

#include "Types.h"

typedef enum
{
	DMA_INT_PRIO_OFF = 0x00,
	DMA_INT_PRIO_LO	 = 0x01,
	DMA_INT_PRIO_MED = 0x02,
	DMA_INT_PRIO_HI	 = 0x03,

	NUM_DMA_INT_PRIORITIES,
} eDMAIntPriority;

typedef struct
{
	u16					BlockTransferCount;
	u8					RepeatCount; // 1 = single shot mode
	eDMAIntPriority		IntPriority;
	DMA_CH_TRIGSRC_t	TriggerSource;
	u32					SrcAddr;
	u32					DstAddr;
	DMA_CH_SRCDIR_t		SrcAddrMode;
	DMA_CH_DESTDIR_t	DstAddrMode;
	DMA_CH_SRCRELOAD_t	SrcReloadMode;
	DMA_CH_DESTRELOAD_t DstReloadMode;
	DMA_CH_BURSTLEN_t	BurstLength;
} sDMAChannelConfig;

void SYS_EnableDMA(void);
void DMA_SetChannelConfig(DMA_CH_t* pDMA, sDMAChannelConfig* pConfig);
void DMA_Start(DMA_CH_t* pDMA);
void DMA_Stop(DMA_CH_t* pDMA);
void DMA_EnableDoubleBuffer(DMA_DBUFMODE_t Mode);

static inline DMA_CH_t* DMA_GetChannelPtr(u8 ChannelNum)
{
	return ((DMA_CH_t*)((uintptr_t)(&DMA.CH0) + (sizeof(DMA_CH_t) * ChannelNum)));
}