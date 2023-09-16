/*
 * File: DMA.h ( 13th November 2021 )
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

#pragma once

#include <avr/io.h>

#include "DataTypes.h"
#include "Interrupt.h"
#include "Utility.h"

typedef struct
{
    DMA_CH_t*           pChannel;
    u8                  Repeats; // <= 1 for single shot mode
    u16                 BytesPerTransfer;
    DMA_CH_BURSTLEN_t   BurstLength;
    DMA_CH_TRIGSRC_t    TriggerSource;
    DMA_DBUFMODE_t      DoubleBufferMode;
    eInterruptPriority  InterruptPriority;
    eInterruptPriority  ErrInterruptPriority;
    uintptr_t           SrcAddress;
    DMA_CH_SRCDIR_t     SrcAddressingMode;
    DMA_CH_SRCRELOAD_t  SrcReloadMode;
    uintptr_t           DstAddress;
    DMA_CH_DESTDIR_t    DstAddressingMode;
    DMA_CH_DESTRELOAD_t DstReloadMode;
} sDMA_ChannelConfig;

static inline DMA_CH_t* DMA_GetChannelPointer(u8 ChannelNumber)
{
    return (DMA_CH_t*)((uintptr_t)(&DMA.CH0) + (sizeof(DMA_CH_t) * ChannelNumber));
}

static inline void DMA_EnableChannel(DMA_CH_t* pDMA)
{
    SET_REG(pDMA->CTRLA, DMA_CH_ENABLE_bm);
}

static inline void DMA_DisableChannel(DMA_CH_t* pDMA)
{
    CLR_REG(pDMA->CTRLA, DMA_CH_ENABLE_bm);
}

static inline void DMA_ResetChannel(DMA_CH_t* pDMA)
{
    SET_REG(pDMA->CTRLA, DMA_CH_RESET_bm);
}

static inline bool DMA_ChannelBusy(u8 ChannelNumber)
{
    vu8 busy = DMA.STATUS;

    busy &= (1 << ChannelNumber) | (1 << (ChannelNumber + 4));
    return (bool)busy;
}

static inline void DMA_SetChannelSourceAddress(DMA_CH_t* pDMA, uintptr_t Address)
{
    pDMA->SRCADDR0 = (Address >> 0) & 0xFF;
    pDMA->SRCADDR1 = (Address >> 8) & 0xFF;
    // pDMA->SRCADDR2 = (Address >> 16) & 0xFF;
    pDMA->SRCADDR2 = 0; // On xmega this must be explicitly set to zero otherwise DMA transactions will go out-of-bounds (runaway).
}

static inline void DMA_SetChannelInterrupts(DMA_CH_t* pDMA, eInterruptPriority TransactionIntPriority, eInterruptPriority ErrorIntPriority)
{
    CLR_REG(pDMA->CTRLB, DMA_CH_ERRINTLVL_gm | DMA_CH_TRNINTLVL_gm);
    SET_REG(pDMA->CTRLB, (ErrorIntPriority << DMA_CH_ERRINTLVL_gp) | (TransactionIntPriority << DMA_CH_TRNINTLVL_gp));
}

void DMA_Init(void);
void DMA_InitChannel(const sDMA_ChannelConfig* pConfig);