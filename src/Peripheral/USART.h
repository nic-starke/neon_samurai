/*
 * File: USART.h ( 20th November 2021 )
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

#pragma once

#include <avr/io.h>

#include "data_types.h"
#include "Utility.h"

// This implementation is fixed to USART in Master SPI Mode (additional modes
// exist but are not implemented).

typedef enum
{
    UART_Module_C0,
    UART_Module_C1,
    UART_Module_D0,
    UART_Module_D1,
    UART_Module_E0,

    NUM_UART_MODULES,
} eUART_Module;

typedef enum
{
    SPI_MODE_0, // clock low, phase low
    SPI_MODE_1, // clock low, phase high
    SPI_MODE_2, // clock high, phase low
    SPI_MODE_3, // clock high, phase high
} eSPI_Mode;

typedef struct
{
    USART_t*   pUSART;
    u32        BaudRate;
    eSPI_Mode  SPIMode;
    eDataOrder DataOrder;
} sUSART_ModuleConfig;

static inline void USART_DisableTX(USART_t* pUSART)
{
    CLR_REG(pUSART->CTRLB, USART_TXEN_bm);
}

static inline void USART_EnableTX(USART_t* pUSART)
{
    SET_REG(pUSART->CTRLB, USART_TXEN_bm);
}

static inline void USART_DisableRX(USART_t* pUSART)
{
    CLR_REG(pUSART->CTRLB, USART_RXEN_bm);
}

static inline void USART_EnableRX(USART_t* pUSART)
{
    SET_REG(pUSART->CTRLB, USART_RXEN_bm);
}

void USART_Init(void);
void USART_InitModule(const sUSART_ModuleConfig* pConfig);