/*
 * File: Display.c ( 31st October 2021 )
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
#include <string.h>

#include "Display.h"
#include "DMA.h"
#include "Types.h"
#include "HardwareDefines.h"

#define DISPLAY_FRAME_SIZE (NUM_LED_SHIFTREG)

#define DMA_CHANNEL (0)

// #define DISPLAY_CC_VAL			(DISPLAY_REFRESH_RATE)		// n x 8 uS update

// #define SR_ENABLE				IOPORT_CREATE_PIN(PORTD, 0)
// #define SR_CLK					IOPORT_CREATE_PIN(PORTD, 1)
// #define SR_DATA					IOPORT_CREATE_PIN(PORTD, 3)
// #define SR_LATCH				IOPORT_CREATE_PIN(PORTD, 4)
// #define SR_RESET				IOPORT_CREATE_PIN(PORTD, 5)

// #define USART_SPI				(USARTD0)
// #define USART_SPI_BAUDRATE      (4000000)
// #define USART_SPI_MODE          (0)			// Sample on rising edge.
// #define USART_SPI_DATA_ORDER    (1)			// MSB First.

static volatile Frame DisplayBuffer[DISPLAY_BUF_SIZE][NUM_ENCODERS];

void Display_Init(void)
{
	memset(&DisplayBuffer, LED_OFF, sizeof(DisplayBuffer));
	DMA_CH_t* pDMA = DMA_GetChannelPtr(DMA_CHANNEL);

	sDMAChannelConfig dmaConfig = {
		.BlockTransferCount = DISPLAY_FRAME_SIZE,
		.RepeatCount		= 1,
		.IntPriority		= DMA_INT_PRIO_MED,
		.TriggerSource		= DMA_CH_TRIGSRC_USARTD0_DRE_gc,
		.SrcAddr			= (u16)(uintptr_t)DisplayBuffer,
		.SrcAddrMode		= DMA_CH_SRCDIR_INC_gc,
		.SrcReloadMode		= DMA_CH_SRCRELOAD_NONE_gc,
		.DstAddr			= (u16)(uintptr_t)&USARTD0.DATA,
		.DstAddrMode		= DMA_CH_DESTDIR_FIXED_gc,
		.DstReloadMode		= DMA_CH_DESTRELOAD_NONE_gc,
	};

	DMA_SetChannelConfig(pDMA, &dmaConfig);
}