/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>

#include "hal/avr/xmega/128a4u/gpio.h"
#include "board/djtt/midifighter.h"
#include "drivers/encoder.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PORT_SWITCHES (PORTA)
#define PORT_SREG     (PORTC)

#define PIN_SREG_LATCH   (0)
#define PIN_SREG_CLOCK   (1)
#define PIN_SREG_DATA_IN (2)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static encoder_hwctx_t encoder_ctx[MF_NUM_ENCODERS];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int mf_hardware_init(void) {
  // Configure gpios for side switches
  for (int i = 0; i < MF_NUM_SIDE_SWITCHES; ++i) {
    gpio_dir(&PORT_SWITCHES, i, GPIO_INPUT);
    gpio_mode(&PORT_SWITCHES, i, PORT_OPC_PULLUP_gc);
  }

  // Configure shift registers for encoders
  gpio_dir(&PORT_SREG, PIN_SREG_LATCH, GPIO_OUTPUT);
  gpio_dir(&PORT_SREG, PIN_SREG_CLOCK, GPIO_OUTPUT);
  gpio_dir(&PORT_SREG, PIN_SREG_DATA_IN, GPIO_INPUT);

  // Latch sreg
  gpio_set(&PORT_SREG, PIN_SREG_LATCH, 1);
  gpio_set(&PORT_SREG, PIN_SREG_LATCH, 0);

  return 0;
}

void mf_update_encoders(void) {
  for (int i = 0; i < MF_NUM_ENCODERS; ++i) {
    encoder_update(&encoder_ctx[i]);
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
