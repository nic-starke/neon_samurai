/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/sys.h"
#include "system/os.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define MAIN_STACK_SIZE 128
#define MAIN_PRIORITY   16

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifndef __AVR_ATxmega128A4U__
#warning                                                                       \
    "This project has only been tested on the XMEGA 128A4U, continue at your own risk"
#endif

#include <avr/wdt.h>

#include "USB/USB.h"
#include "LUFA/Common/Common.h"
#include "Comms/Comms.h"
#include "Config.h"
#include "Peripheral/DMA.h"
#include "system/Data.h"
#include "Display/Display.h"
#include "Display/EncoderDisplay.h"
#include "Input/Input.h"
#include "MIDI/MIDI.h"
#include "Comms/Network.h"
#include "system/SoftTimer.h"
#include "system/System.h"
#include "Peripheral/USART.h"

#define ENABLE_SERIAL
#include "USB/VirtualSerial.h"

static sSoftTimer mTestTimer = {0};

static void SetupOperatingMode(void);
static void SetupTest(void);
static void RunTest(void);
static void BootAnimation(void);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void main_thread(uint32_t data);
static void old_init(void);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Main thread stack
static uint8_t main_stack[MAIN_STACK_SIZE];

// Main thread data
static os_thread_t main_thread_data = {
    .stack      = &main_stack[0],
    .stack_size = MAIN_STACK_SIZE,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void main(void) {
  // Init
  old_init();

  // Initialise the operating system
  int s = os_init();
  EXIT_ON_ERR(s, error);

  // Create the first thread
  s = (int)os_thread_new(&main_thread_data, MAIN_PRIORITY, 0, main_thread);
  EXIT_ON_ERR(s, error);

  // Start execution
  os_start();

error:
  while (1) {
    RunTest();
    // blink LED
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void main_thread(uint32_t data) {
  while (1) {
    Input_Update();

    switch (gData.OperatingMode) {
    case DEFAULT_MODE: {
      Display_Update();
      Encoder_Update();
      // SideSwitch_Update(); // FIXME Not yet implemented
      MIDI_Update();
      Network_Update();
      Comms_Update();
      break;
    }

    case TEST_MODE: RunTest(); break;

    default: break;
    }

    Serial_Update();
    USB_USBTask();
  }
}

static void old_init(void) {
  /** ** WARNING ** Do not adjust the order of these init functions! **/
  System_Init();

  // Peripherals and hardware and middleware
  DMA_Init();
  USART_Init();
  Input_Init();
  USB_Init();
  Display_Init();
  Serial_Init();
  SoftTimer_Init();
  Encoder_Init();

  GlobalInterruptEnable(); // Enable global interrupts so that the display gets
                           // updated, and so I/O can be read.
  SetupOperatingMode();

  Data_Init();
  Comms_Init(); // Modules that require comms should be initialised after this
                // point.
  Network_Init();
  MIDI_Init();

  EncoderDisplay_UpdateAllColours(); // FIXME - A better approach would be a
                                     // single function to completely reset the
                                     // display.

  Serial_Print("Booted\r\n");

#ifdef VSER_ENABLE
  SoftTimer_Stop(&mTestTimer);
  SoftTimer_Start(&mTestTimer);
  while (SoftTimer_Elapsed(&mTestTimer) <
         2500) // Poll the USB and serial for a bit so that host is immediately
               // ready for serial comms
  {
    Serial_Update();
    USB_USBTask();
  }
  SoftTimer_Stop(&mTestTimer);
#endif

  Network_StartDiscovery(false);
}

/**
 * @brief Sets the operating mode based on special input switch combinations.
 * @warning This must only be executed if global interrupts are enabled.
 * @warning Execute this before Data_Init() to ensure EEPROM wipes occur before
 * data recall.
 */
static void SetupOperatingMode(void) {
  // Run update a few times to flush buffers
  for (int i = 0; i < 100; i++) {
    Input_Update();
  }

  Input_CheckSpecialSwitchCombos();

  // Check if need to start any special modes
  switch (gData.OperatingMode) {
  case TEST_MODE: SetupTest(); break;

  case BOOTLOADER_MODE: System_StartBootloader(); break;

  case DEFAULT_MODE:
  default: BootAnimation(); break;
  }
}

/**
 * @brief Setup any test mode variables
 *
 */
static void SetupTest(void) {
  SoftTimer_Start(&mTestTimer);
}

/**
 * @brief The function that gets called when running test mode.
 *
 */
static void RunTest(void) {
  if (SoftTimer_Elapsed(&mTestTimer) >= 250) {
    Display_Test();
    SoftTimer_Start(&mTestTimer);
  }
}

/**
 * @brief A simple boot animation.
 *
 */
static void BootAnimation(void) {
  // TODO Implement a separate timer for use as an arbitrary task scheduler.
  // Use this to run non-blocking animations.

  // for (uint16_t br = 0; br < BRIGHTNESS_MAX; br += 3)
  // {

  //     // Display_SetMaxBrightness(br);
  //     // Display_Update();
  //     //Delay_MS(10);
  // }
}
