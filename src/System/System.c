/*
 * File: System.c ( 20th November 2021 )
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

#include <avr/wdt.h>
#include <Platform/XMEGA/ClockManagement.h>
#include <Common/Common.h>
#include <Drivers/USB/USB.h>

#include "System.h"
#include "CPU.h"
#include "Interrupt.h"
#include "Data.h"
#include "Display.h"

void (*bootloader)(void) = (void (*)(void))(BOOT_SECTION_START / 2 + 0x1FC / 2);
uint32_t mBootKey __attribute__((section(".noinit")));

void System_Init(void)
{
    wdt_disable();

    // Initialise system clocks
    XMEGACLK_StartPLL(CLOCK_SRC_INT_RC2MHZ, 2000000, F_CPU);
    XMEGACLK_SetCPUClockSource(CLOCK_SRC_PLL);
    XMEGACLK_StartInternalOscillator(CLOCK_SRC_INT_RC32MHZ);
    XMEGACLK_StartDFLL(CLOCK_SRC_INT_RC32MHZ, DFLL_REF_INT_USBSOF, F_USB);

    // Configure interrupt controller
    PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
}

void System_BootloaderCheck(void)
{
    if (((RST.STATUS & RST_WDRF_bm)) && (mBootKey == BOOTKEY))
    {
        mBootKey = 0;
        EIND     = BOOT_SECTION_START >> 17;
        bootloader();
    }
}

void System_StartBootloader(void)
{
    Display_Flash(100, 2);
    USB_Disable();
    IRQ_DisableInterrupts();

    mBootKey = BOOTKEY;
    wdt_enable(WDTO_30MS);
    while (1) {}
}