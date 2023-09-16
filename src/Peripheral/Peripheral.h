/*
 * File: Peripheral.h ( 10th November 2021 )
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

#include "Types.h"

typedef enum
{
    PERIPH_DMA,
    PERIPH_EVSYS,
    PERIPH_RTC,
    PERIPH_AES,
    PERPIH_USB,

    PERIPH_A_ANALOGCOMPARATOR,
    PERIPH_A_ADC,

    PERIPH_B_DAC,

    PERIPH_C_TC0,
    PERIPH_C_TC1,
    PERIPH_C_TC2,
    PERIPH_C_HIRES,
    PERIPH_C_SPI,
    PERIPH_C_USART0,
    PERIPH_C_USART1,
    PERIPH_C_TWI,

    PERIPH_D_TC0,
    PERIPH_D_TC1,
    PERIPH_D_TC2,
    PERIPH_D_HIRES,
    PERIPH_D_SPI,
    PERIPH_D_USART0,
    PERIPH_D_USART1,

    PERIPH_E_TWI,
    PERIPH_E_TC0,
    PERIPH_E_HIRES,
    PERIPH_E_USART0,

    INVALID_PERIPH,
} ePeripheral;

typedef enum
{
	PERIPH_PORT_UNASSIGED,
	PERIPH_PORT_A,
	PERIPH_PORT_B,
	PERIPH_PORT_C,
	PERIPH_PORT_D,
	PERIPH_PORT_E,
	PERIPH_PORT_F,

    INVALID_PORT,

    NUM_PERIPH_PORTS,
} ePort;

bool PeripheralClock_Enable(ePeripheral Peripheral);