/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>
#include <util/atomic.h>

#include "system/types.h"
#include "system/error.h"
#include "hal/usart.h"
#include "hal/gpio.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Missing defines for xmega usart
#define USART_UCPHA_bm (0x02) // clock phase bitmask
#define USART_DORD_bm	 (0x04) // data order bitmask

#define BSEL_MAX			 (4095u)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static u8						get_mask(USART_t* usart);
static register8_t* get_power_reg(USART_t* usart);
static PORT_t*			get_port(USART_t* usart);
static void					configure_io(PORT_t* port, USART_t* usart,
																 const struct usart_config* config);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

// SPI Master only
int usart_module_init(USART_t* usart, const struct usart_config* config) {
	assert(usart);
	assert(config);

	register8_t* power = get_power_reg(usart);
	PORT_t*			 port	 = get_port(usart);

	if ((power == NULL) || (port == NULL)) {
		return ERR_BAD_PARAM;
	}

	if ((config->baudrate == 0) || (config->baudrate > (F_CPU / 2))) {
		return ERR_BAD_PARAM;
	}

	// AU manual table 23-1, synchronous and master SPI mode:
	// f_BAUD = f_PER / (2 * (BSEL + 1)). Rounding the divisor up keeps the
	// generated clock at or below what was asked for, never above it.
	const u32 divisor =
			((u32)F_CPU + (2u * config->baudrate) - 1u) / (2u * config->baudrate);
	const u32 bsel = divisor - 1u;

	if (bsel > BSEL_MAX) {
		return ERR_BAD_PARAM;
	}

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {

		*power &= (u8)~get_mask(usart);

		// Disable rx and tx
		usart->CTRLB &= (u8)~USART_RXEN_bm;
		usart->CTRLB &= (u8)~USART_TXEN_bm;

		// Configure IO pins
		configure_io(port, usart, config);

		// CTRLC is written whole - its reset value carries async-mode bits that
		// mean something different once CMODE selects master SPI.
		u8 ctrlc = USART_CMODE_MSPI_gc;

		if ((config->mode == SPI_MODE_CLK_LO_PHA_HI) ||
				(config->mode == SPI_MODE_CLK_HI_PHA_HI)) {
			ctrlc |= USART_UCPHA_bm;
		}

		if (config->endian == ENDIAN_LSB) {
			ctrlc |= USART_DORD_bm;
		}

		usart->CTRLC = ctrlc;

		// BSCALE has no term in the master SPI equation and is left at zero.
		usart->BAUDCTRLB = (u8)((bsel >> 8u) & (u8)~USART_BSCALE_gm);
		usart->BAUDCTRLA = (u8)(bsel);

		usart->CTRLB |= USART_TXEN_bm;

		if (config->rx_enable) {
			usart->CTRLB |= USART_RXEN_bm;
		}

	} // ATOMIC_BLOCK(ATOMIC_RESTORESTATE)

	return SUCCESS;
}

void usart_set_tx(USART_t* usart, bool enable) {
	assert(usart);

	if (enable) {
		usart->CTRLB |= USART_TXEN_bm;
	} else {
		usart->CTRLB &= (u8)~USART_TXEN_bm;
	}
}

void usart_set_rx(USART_t* usart, bool enable) {
	assert(usart);

	if (enable) {
		usart->CTRLB |= USART_RXEN_bm;
	} else {
		usart->CTRLB &= (u8)~USART_RXEN_bm;
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static u8 get_mask(USART_t* usart) {
	if (usart == &USARTC0 || usart == &USARTD0 || usart == &USARTE0) {
		return PR_USART0_bm;
	} else if (usart == &USARTC1 || usart == &USARTD1) {
		return PR_USART1_bm;
	}

	return 0;
}

static void configure_io(PORT_t* port, USART_t* usart,
												 const struct usart_config* config) {
	// Default pins are  SCK = 1, RX = 2, TX = 3
	// Remapped pins are SCK = 5, RX = 6, TX = 7

	bool remap = false;

	if (usart == &USARTC1 || usart == &USARTD1) {
		remap = true;
	} else {
		remap = (port->REMAP & PORT_USART0_bm);
	}

	const u8	 sck				= (remap ? 5 : 1);
	const u8	 rx					= (remap ? 6 : 2);
	const u8	 tx					= (remap ? 7 : 3);
	const bool invert_sck = (config->mode == SPI_MODE_CLK_HI_PHA_LO ||
													 config->mode == SPI_MODE_CLK_HI_PHA_HI);

	gpio_mode(port, sck, PORT_OPC_TOTEM_gc);
	gpio_invert(port, sck, invert_sck);

	gpio_dir(port, sck, GPIO_OUTPUT);
	gpio_dir(port, tx, GPIO_OUTPUT);
	gpio_set(port, sck, 1);

	if (config->rx_enable) {
		gpio_dir(port, rx, GPIO_INPUT);
	}
}

static register8_t* get_power_reg(USART_t* usart) {
	if (usart == &USARTC0 || usart == &USARTC1) {
		return &PR.PRPC;
	}

	if (usart == &USARTD0 || usart == &USARTD1) {
		return &PR.PRPD;
	}

	if (usart == &USARTE0) {
		return &PR.PRPE;
	}

	return NULL;
}

static PORT_t* get_port(USART_t* usart) {
	if (usart == &USARTC0 || usart == &USARTC1) {
		return &PORTC;
	}

	if (usart == &USARTD0 || usart == &USARTD1) {
		return &PORTD;
	}

	if (usart == &USARTE0) {
		return &PORTE;
	}

	return NULL;
}
