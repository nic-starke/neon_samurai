/*
 * File: Display.c ( 13th November 2021 )
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

#include "Display.h"
#include "HardwareDescription.h"
#include "DMA.h"
#include "USART.h"

static volatile DisplayFrame DisplayBuffer[DISPLAY_BUFFER_SIZE][NUM_ENCODERS];

void Display_Init(void)
{
	memset(DisplayBuffer, LED_OFF, sizeof(DisplayBuffer));

	sDMA_ChannelConfig dmaConfig = 
    {
		.pChannel = DMA_GetChannelPointer(0),

		.BurstLength		  = DMA_CH_BURSTLEN_1BYTE_gc,
		.BytesPerTransfer	  = NUM_LED_SHIFT_REGISTERS, // 1 byte per SR
		.DoubleBufferMode	  = DMA_DBUFMODE_CH01_gc,	 // Channels 0 and 1 in double buffer mode
		.DstAddress			  = (uintptr_t)&USARTD0.DATA,
		.DstAddressingMode	  = DMA_CH_DESTDIR_FIXED_gc,
		.DstReloadMode		  = DMA_CH_DESTRELOAD_NONE_gc,
		.ErrInterruptPriority = DMA_CH_ERRINTLVL_MED_gc,
		.InterruptPriority	  = DMA_CH_TRNINTLVL_MED_gc,
		.Repeats			  = 1, // Single shot
		.SrcAddress			  = (uintptr_t)&DisplayBuffer[0][0],
		.SrcAddressingMode	  = DMA_CH_SRCDIR_INC_gc,		   // Auto increment through buffer array
		.SrcReloadMode		  = DMA_CH_SRCRELOAD_NONE_gc,	   // Handle address overflow manually in Display DMA ISR
		.TriggerSource		  = DMA_CH_TRIGSRC_USARTD0_DRE_gc, // USART SPI Master will trigger DMA transaction
	};

	DMA_InitChannel(&dmaConfig);

    sUSART_ModuleConfig usartConfig = 
    {
        .pUSART     = &USARTD0,
        .BaudRate   = 4000000,
        .DataOrder = MSB_FIRST,
        .SPIMode = SPI_MODE_0,
    };

    USART_InitModule(&usartConfig);
}