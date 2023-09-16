/*
 * File: DMA.c ( 20th November 2021 )
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

#include "DMA.h"
#include "DataTypes.h"

/**
 * @brief Enable power to the DMA peripheral
 * 
 */
static inline void DMA_EnablePower(void)
{
    CLR_REG(PR.PRGEN, PR_DMA_bm);
}

/**
 * @brief Disable power to the DMA peripheral
 * 
 */
static inline void DMA_DisablePower(void)
{
    SET_REG(PR.PRGEN, PR_DMA_bm);
}

/**
 * @brief Enable the DMA controller
 * 
 */
static inline void DMA_EnableController(void)
{
    // SET_REG(DMA.CTRL, DMA_ENABLE_bm);
    DMA.CTRL = DMA_RESET_bm;
    DMA.CTRL = DMA_ENABLE_bm;
}

/**
 * @brief Disable the DMA controller
 * 
 */
static inline void DMA_DisableController(void)
{
    CLR_REG(DMA.CTRL, DMA_ENABLE_bm);
}

/**
 * @brief Reset the DMA controller
 * 
 */
static inline void DMA_ResetController(void) // TODO - do interrupts need to be disabled, does the DMA need to be disabled?
{
    SET_REG(DMA.CTRL, DMA_RESET_bm);
}

/**
 * @brief Set the double buffer mode for the DMA controller.
 * If enabled - this will "interlink" DMA channel 0 and 1, or channel 2 and 3
 * After the primary channel is complete the secondary channel will fire a dma transaction, which then re-enables the primary channel...
 * @param Mode 
 */
static inline void DMA_SetDoubleBufferMode(DMA_DBUFMODE_t Mode)
{
    DMA.CTRL = (DMA.CTRL & ~DMA_DBUFMODE_gm) | Mode;
}

/**
 * @brief Initialise the DMA controller peripheral.
 *  Must be called once during system power-up and before using any DMA
 * channels.
 * 
 * IRQs are disabled during the DMA initialisation.
 */
void DMA_Init(void)
{
    const u8 flags = IRQ_DisableInterrupts();

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
void DMA_InitChannel(const sDMA_ChannelConfig* pConfig)
{
    const u8 flags = IRQ_DisableInterrupts();

    DMA_DisableChannel(pConfig->pChannel);

    // Source
    pConfig->pChannel->SRCADDR0 = (pConfig->SrcAddress >> 0) & 0xFF;
    pConfig->pChannel->SRCADDR1 = (pConfig->SrcAddress >> 8) & 0xFF;
    pConfig->pChannel->SRCADDR2 = 0; //(pConfig->SrcAddress >> 16) & 0xFF;

    CLR_REG(pConfig->pChannel->ADDRCTRL, DMA_CH_SRCDIR_gm);
    SET_REG(pConfig->pChannel->ADDRCTRL, pConfig->SrcAddressingMode);

    CLR_REG(pConfig->pChannel->ADDRCTRL, DMA_CH_SRCRELOAD_gm);
    SET_REG(pConfig->pChannel->ADDRCTRL, pConfig->SrcReloadMode);

    // Destination
    pConfig->pChannel->DESTADDR0 = (pConfig->DstAddress >> 0) & 0xFF;
    pConfig->pChannel->DESTADDR1 = (pConfig->DstAddress >> 8) & 0xFF;
    pConfig->pChannel->DESTADDR2 = 0; //(pConfig->DstAddress >> 16) & 0xFF;

    CLR_REG(pConfig->pChannel->ADDRCTRL, DMA_CH_DESTDIR_gm);
    SET_REG(pConfig->pChannel->ADDRCTRL, pConfig->DstAddressingMode);

    CLR_REG(pConfig->pChannel->ADDRCTRL, DMA_CH_DESTRELOAD_gm);
    SET_REG(pConfig->pChannel->ADDRCTRL, pConfig->DstReloadMode);

    // DMA Config
    pConfig->pChannel->TRIGSRC = pConfig->TriggerSource;

    CLR_REG(pConfig->pChannel->CTRLA, DMA_CH_BURSTLEN_gm);
    SET_REG(pConfig->pChannel->CTRLA, pConfig->BurstLength);

    pConfig->pChannel->TRFCNT = pConfig->BytesPerTransfer;

    if (pConfig->Repeats > 1)
    {
        pConfig->pChannel->REPCNT = pConfig->Repeats;
        CLR_REG(pConfig->pChannel->CTRLA, DMA_CH_SINGLE_bm);
        SET_REG(pConfig->pChannel->CTRLA, DMA_CH_REPEAT_bm);
    }
    else
    {
        CLR_REG(pConfig->pChannel->CTRLA, DMA_CH_REPEAT_bm);
        SET_REG(pConfig->pChannel->CTRLA, DMA_CH_SINGLE_bm);
    }

    CLR_REG(pConfig->pChannel->CTRLB, DMA_CH_ERRINTLVL_gm | DMA_CH_TRNINTLVL_gm);
    SET_REG(pConfig->pChannel->CTRLB,
            (pConfig->ErrInterruptPriority << DMA_CH_ERRINTLVL_gp) | (pConfig->InterruptPriority << DMA_CH_TRNINTLVL_gp));

    DMA_SetDoubleBufferMode(pConfig->DoubleBufferMode);
    IRQ_EnableInterrupts(flags);
}