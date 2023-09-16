/*
 * File: USART.c ( 7th November 2021 )
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

#include "USART.h"
#include "Types.h"
#include "SystemClock.h"

#define USART_DORD_bm   (0x04) // missing define from xmega IO.h

inline static eSysClkPort GetSysClkPort(USART_t* pUSART)
{
    if ( (pUSART == &USARTC0) || (pUSART == &USARTC1) )
    {
        return SYSCLK_PORT_C;
    }
    else if ( (pUSART == &USARTD0) || (pUSART == &USARTD1) )
    {
        return SYSCLK_PORT_D;
    }
    else if (pUSART == &USARTE0)
    {
        return SYSCLK_PORT_E;
    }

    return SYSCLK_PORT_INVALID;
}

inline static u8 GetPRGenBitMask(USART_t* pUSART)
{
    if ( (pUSART == &USARTC0) || (pUSART == &USARTD0) || (pUSART == &USARTE0))
    {
        return PR_USART0_bm;
    }
    else if ( (pUSART == &USARTC1) || (pUSART == &USARTD1) )
    {
        return PR_USART1_bm;
    }

    return 
}

bool USART_Init(USART_t* pUSART)
{
    eSysClkPort = GetSysClkPort(pUSART);
    ePort port = GetPort(Peripheral);

    if (port == SYSCLK_PORT_INVALID)
    {
        return false;
    }

    u8 prbm = GetPRGenBitMask(pUSART);

    u8 oldSREG = IRQ_DisableGlobalInterrupts();

    *((u8 *)&PR.PRGEN + port) &= ~prbm;

    IRQ_RestoreSREG(oldSREG);

    return true;

}

inline static void SetBaud (USART_t* pUSART, u32 BaudRate, u32 CPUFreq)
{
    u16 baudRegValue;
    if (BaudRate < (CPUFreq / 2))
    {
        baudRegValue = (CPUFreq / (BaudRate * 2) - 1);
    }
    else
    {
        baudRegValue = 0;
    }

    (pUSART)->BAUDCTRLB = (u8)((~USART_BSCALE_gm) & (baudRegValue >> 8));
	(pUSART)->BAUDCTRLA = (u8)(baudRegValue);
}

// Initialises the USART in SPI master mode
void USART_SetSPIConfig(USART_t *pUSART, sUSART_SPIConfig* pConfig)
{
    
    
    SetBaud(pUSART, pConfig->BaudRate, F_CPU);
    pUSART->CTRLC = USART_CMODE_MSPI_gc;

    if(pConfig->DataOrder == LSB_FIRST)
    {
        SET_BIT(pUSART->CTRLC, USART_DORD_bm);
    }
    else
    {
        CLR_BIT(pUSART->CTRLC, USART_DORD_bm);
    }

    pUSART->CTRLB = USART_RXEN_bm | USART_TXEN_bm;
}