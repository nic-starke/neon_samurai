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

void USART_Init(USART_t* pUSART, sUSARTConfig* pConfig)
{
    // if (opt->spimode == 1 || opt->spimode == 3) {
	// 	usart->CTRLC |= USART_UCPHA_bm;
	// } else {
	// 	usart->CTRLC &= ~USART_UCPHA_bm;
	// }
	// if (opt->data_order) {
	// 	(usart)->CTRLC |= USART_DORD_bm;
	// } else {
	// 	(usart)->CTRLC &= ~USART_DORD_bm;
	// }
}