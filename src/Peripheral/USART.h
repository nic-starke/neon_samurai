/*
 * File: USART.h ( 7th November 2021 )
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

#include "Utils.h"
#include "Types.h"
#include "Peripheral.h"

typedef struct USART
{
    u32 BaudRate;
    u8 SPIMode;
    eDataOrder DataOrder;
} sUSART_SPIConfig;


bool USART_Init(ePeripheral USART);
void USART_SetSPIConfig(USART_t *pUSART, sUSART_SPIConfig* pConfig);