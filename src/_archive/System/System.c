/*
 * File: System.c ( 20th November 2021 )
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

#include <avr/wdt.h>

#include "LUFA/Common/Common.h"
#include "LUFA/Drivers/USB/USB.h"
#include "LUFA/Platform/XMEGA/ClockManagement.h"

#include "core/CPU.h"
#include "core/Data.h"
#include "Display/Display.h"
#include "core/Interrupt.h"
#include "core/core_types.h"

// For more info on the bootloader jump code see
// http://www.fourwalledcubicle.com/files/LUFA/Doc/120730/html/_page__software_bootloader_start.html

// This is a pointer to the reset interrupt vector of the bootloader, which is
// located at this specific location (as per the Atmel application note.)
void (*bootloader)(void) = (void (*)(void))(BOOT_SECTION_START / 2 + 0x1FC / 2);

// The boot key is placed in the no init section - it will not be initialised by
// crt0, meaning that its value will be retained AFTER a soft-reset. The boot
// key is checked at system startup and if its value matches BOOTKEY then the
// bootloader execution will jump to the bootloader.
uint32_t mBootKey __attribute__((section(".noinit")));

void System_Init(void) {
  wdt_disable();

  // Initialise system clocks
  XMEGACLK_StartPLL(CLOCK_SRC_INT_RC2MHZ, 2000000, F_CPU);
  XMEGACLK_SetCPUClockSource(CLOCK_SRC_PLL);
  XMEGACLK_StartInternalOscillator(CLOCK_SRC_INT_RC32MHZ);
  XMEGACLK_StartDFLL(CLOCK_SRC_INT_RC32MHZ, DFLL_REF_INT_USBSOF, F_USB);

  // Configure interrupt controller
  PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
}

void System_BootloaderCheck(void) {
  // Check if the reset was caused by the watchdog timer, and that the bootkey
  // is valid.
  if (((RST.STATUS & RST_WDRF_bm)) && (mBootKey == BOOTKEY)) {
    mBootKey = 0; // Reset the bootkey to stop a bootloader loop.

    /**
     * Copied from the GCC AVR options documentation -
     * https://gcc.gnu.org/onlinedocs/gcc-6.3.0/gcc/AVR-Options.html In order to
     * facilitate indirect jump on devices with more than 128 Ki bytes of
     * program memory space, there is a special function register called EIND
     * that serves as most significant part of the target address when EICALL or
     * EIJMP instructions are used.
     * */
    EIND = BOOT_SECTION_START >> 17;
    bootloader();
  }
}

/**
 * @brief Start the bootloader.
 * This disables USB and IRQs, then sets the bootkey and enables the watchdog
 * timer. When the watchdog executes it will reset the system, causing it to
 * jump to the bootloader.
 */
void System_StartBootloader(void) {
  Display_Flash(100, 2);
  USB_Disable();
  IRQ_DisableInterrupts();

  mBootKey = BOOTKEY;
  wdt_enable(WDTO_30MS);
  while (1) {}
}
