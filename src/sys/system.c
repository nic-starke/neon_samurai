/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/system.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void system_thread(uint32_t data) {
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

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
