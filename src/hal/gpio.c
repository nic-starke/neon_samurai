/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"
#include "hal/gpio.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PIN_MASK(x) (1u << ((x) & 0x07))

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void gpio_mode(PORT_t* port, u8 pin, PORT_OPC_t mode) {
	assert(port);
	assert(pin < 8);

	volatile u8* ctrl = &port->PIN0CTRL + pin;

	*ctrl &= (u8)~PORT_OPC_gm; // Clear all OPC bits
	*ctrl |= (u8)mode;         // Set the new mode
}

void gpio_dir(PORT_t* port, u8 pin, enum gpio_dir dir) {
	assert(port);

	if (dir == GPIO_INPUT) {
		port->DIRCLR = PIN_MASK(pin);
	} else if (dir == GPIO_OUTPUT) {
		port->DIRSET = PIN_MASK(pin);
	}
}

void gpio_set(PORT_t* port, u8 pin, u8 state) {
	assert(port);

	if (state) {
		port->OUTSET = PIN_MASK(pin);
	} else {
		port->OUTCLR = PIN_MASK(pin);
	}
}

u8 gpio_get(PORT_t* port, u8 pin) {
	assert(port);
	return port->IN & PIN_MASK(pin);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
