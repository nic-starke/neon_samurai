/*
 * File: System.h ( 7th November 2021 )
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

#include <avr/io.h>
#include <Platform/XMEGA/ClockManagement.h>

#include "Types.h"

bool SYS_Init(void);

// Wait for the WDT to be synced to the WDT time clock domain
inline static void WDT_WaitForSync(void)
{
    while ((WDT.STATUS & WDT_SYNCBUSY_bm) == WDT_SYNCBUSY_bm) {}
}

inline static void SYS_EnableWDT(void)
{
    u8 val = (WDT.CTRL & WDT_PER_gm) | (1 << WDT_ENABLE_bp) | (1 << WDT_CEN_bp);
	XMEGACLK_CCP_Write((void *)&WDT.CTRL, val);
	WDT_WaitForSync();
}

inline static void SYS_DisableWDT(void)
{
    u8 val = (WDT.CTRL & ~WDT_ENABLE_bm) | (1 << WDT_CEN_bp);
    XMEGACLK_CCP_Write((void *)&WDT.CTRL, val);
}