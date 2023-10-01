/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/wdt.h>
#include <avr/io.h>

#include "LUFA/Common/Common.h"
#include "LUFA/Drivers/USB/USB.h"
#include "LUFA/Platform/XMEGA/ClockManagement.h"

#include "system/system.h"
#include "board/board.h"

#include "hal/avr/xmega/128a4u/dma.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define BOOTKEY 0x99C0FFEE

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// The boot key is placed in the no init section - it will not be initialised by
// crt0, meaning that its value will be retained AFTER a soft-reset. The boot
// key is checked at system startup and if its value matches BOOTKEY then the
// bootloader execution will jump to the bootloader.
__attribute__((section(".noinit"))) static uint32_t boot_key;

// This is a pointer to the reset interrupt vector of the bootloader, which is
// located at this specific location (as per the Atmel application note.)
void (*bootloader)(void) = (void (*)(void))(BOOT_SECTION_START / 2 + 0x1FC / 2);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int system_init(void) {
  wdt_disable();

  // Initialise system clocks
  XMEGACLK_StartPLL(CLOCK_SRC_INT_RC2MHZ, 2000000, F_CPU);
  XMEGACLK_SetCPUClockSource(CLOCK_SRC_PLL);
  XMEGACLK_StartInternalOscillator(CLOCK_SRC_INT_RC32MHZ);
  XMEGACLK_StartDFLL(CLOCK_SRC_INT_RC32MHZ, DFLL_REF_INT_USBSOF, F_USB);

  // Configure interrupt controller
  PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;

  dma_peripheral_init();
  // input_init();
  // usb_init();
  // display_init();
  // serial_init();
  // softtimer_init();
  // encoder_init();

  // Call the board_init function defined by the user
  board_init();

  return 0;
}

void system_thread(uint32_t data) {
  while (1) {
    board_update();
    // Input_Update();

    // switch (gData.OperatingMode) {
    // case DEFAULT_MODE: {
    //   Display_Update();
    //   Encoder_Update();
    //   // SideSwitch_Update(); // FIXME Not yet implemented
    //   MIDI_Update();
    //   Network_Update();
    //   Comms_Update();
    //   break;
    // }

    // case TEST_MODE: RunTest(); break;

    // default: break;
    // }

    // Serial_Update();
    // USB_USBTask();
  }
}

void system_beforemain(void) {
  // Check if the reset was caused by the watchdog timer, and that the bootkey
  // is valid.
  if (((RST.STATUS & RST_WDRF_bm)) && (boot_key == BOOTKEY)) {
    boot_key = 0; // Reset the bootkey to stop a bootloader loop.

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

void system_startbootloader(void) {
  // USB_Disable();
  // IRQ_DisableInterrupts();

  boot_key = BOOTKEY;
  wdt_enable(WDTO_30MS);
  while (1) {}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
