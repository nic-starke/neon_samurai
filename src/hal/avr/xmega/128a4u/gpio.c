/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/system.h"
#include "hal/avr/xmega/128a4u/gpio.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PIN_MASK(x) (1u << ((x) & 0x07))

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void gpio_mode(PORT_t* port, uint8_t pin, PORT_OPC_t mode) {
  assert(port);

  // Get pointer to pin register
  volatile uint8_t* ctrl = (&port->PIN0CTRL + PIN_MASK(pin));

  *ctrl &= ~PORT_ISC_gm; // Clear the mode
  *ctrl |= mode;         // Set the mode
}

void gpio_dir(PORT_t* port, uint8_t pin, gpio_dir_e dir) {
  assert(port);

  if (dir == GPIO_INPUT) {
    port->DIRCLR = PIN_MASK(pin);
  } else if (dir == GPIO_OUTPUT) {
    port->DIRSET = PIN_MASK(pin);
  }
}

void gpio_set(PORT_t* port, uint8_t pin, uint8_t state) {
  assert(port);

  if (state) {
    port->OUTSET = PIN_MASK(pin);
  } else {
    port->OUTCLR = PIN_MASK(pin);
  }
}

uint8_t gpio_get(PORT_t* port, uint8_t pin) {
  assert(port);
  return port->IN & PIN_MASK(pin);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
