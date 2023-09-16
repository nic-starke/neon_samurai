/*
 * File: Peripheral.C ( 10th November 2021 )
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

#include "Types.h"
#include "Peripheral.h"
#include "Interrupt.h"

inline ePort GetPort(ePeripheral Peripheral)
{
    switch ( Peripheral )
    {
        case PERIPH_DMA:
        case PERIPH_EVSYS:
        case PERIPH_RTC:
        case PERIPH_AES:
        case PERPIH_USB:
            return PERIPH_PORT_UNASSIGED;

        case PERIPH_A_ANALOGCOMPARATOR:
        case PERIPH_A_ADC:
            return PERIPH_PORT_A;

        case PERIPH_B_DAC:
            return PERIPH_PORT_B;

        case PERIPH_C_TC0:
        case PERIPH_C_TC1:
        case PERIPH_C_TC2:
        case PERIPH_C_HIRES:
        case PERIPH_C_SPI:
        case PERIPH_C_USART0:
        case PERIPH_C_USART1:
        case PERIPH_C_TWI:
            return PERIPH_PORT_C;

        case PERIPH_D_TC0:
        case PERIPH_D_TC1:
        case PERIPH_D_TC2:
        case PERIPH_D_HIRES:
        case PERIPH_D_SPI:
        case PERIPH_D_USART0:
        case PERIPH_D_USART1:
            return PERIPH_PORT_D;

        case PERIPH_E_TWI:
        case PERIPH_E_TC0:
        case PERIPH_E_HIRES:
        case PERIPH_E_USART0:
            return PERIPH_PORT_E;


        default:
            return INVALID_PORT;
    }

    return INVALID_PORT;
}

inline u8 GetPRBitMask(ePeripheral Peripheral)
{
    switch ( Peripheral )
    {
        case PERIPH_DMA:
            return PR_DMA_bm;

        case PERIPH_EVSYS:
            return PR_EVSYS_bm;

        case PERIPH_RTC:
            return PR_RTC_bm;

        case PERIPH_AES:
            return PR_AES_bm;

        case PERPIH_USB:
            return PR_USB_bm;

        case PERIPH_A_ANALOGCOMPARATOR:
            return PR_AC_bm;

        case PERIPH_A_ADC:
            return PR_ADC_bm;

        case PERIPH_B_DAC:
            return PR_DAC_bm;

        case PERIPH_C_TC0:
        case PERIPH_D_TC0:
        case PERIPH_E_TC0:
            return PR_TC0_bm;

        case PERIPH_C_TC1:
        case PERIPH_D_TC1:
            return PR_TC1_bm;

        case PERIPH_C_TC2:      // TC2 is actually TC1 in a specific mode.
        case PERIPH_D_TC2:      // RTFM
            return INVALID_PERIPH;

        case PERIPH_C_HIRES:
        case PERIPH_D_HIRES:
        case PERIPH_E_HIRES:
            return PR_HIRES_bm;

        case PERIPH_C_SPI:
        case PERIPH_D_SPI:
            return PR_SPI_bm;

        case PERIPH_C_USART0:
        case PERIPH_D_USART0:
        case PERIPH_D_USART1:
            return PR_USART0_bm;

        case PERIPH_C_USART1:
        case PERIPH_E_USART0:
            return PR_USART1_bm;

        case PERIPH_C_TWI:
        case PERIPH_E_TWI:
            return PR_TWI_bm;

        default:
            return INVALID_PERIPH;
            break;
    }
}

bool PeripheralClock_Enable(ePeripheral Peripheral)
{
    ePort port = GetPort(Peripheral);

    if (port == INVALID_PORT)
    {
        return false;
    }

    u8 prbm = GetPRBitMask(Peripheral);

    if (prbm == INVALID_PERIPH)
    {
        return false;
    }

    u8 oldSREG = IRQ_DisableGlobalInterrupts();

    *((u8 *)&PR.PRGEN + port) &= ~prbm;

    IRQ_RestoreSREG(oldSREG);

    return true;
}