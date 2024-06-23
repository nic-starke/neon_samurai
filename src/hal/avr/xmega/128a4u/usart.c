/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>
#include <util/atomic.h>

#include "sys/types.h"
#include "hal/avr/xmega/128a4u/usart.h"
#include "hal/avr/xmega/128a4u/gpio.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Missing defines for xmega usart
#define USART_UCPHA_bm (0x02) // clock phase bitmask
#define USART_DORD_bm  (0x04) // data order bitmask

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static u8	get_mask(USART_t* usart);
static void configure_io(USART_t* usart, spi_mode_e mode);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

// SPI Master only
void usart_module_init(USART_t* usart, const usart_config_s* config) {
	assert(usart);
	assert(config);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {

		// Enable power
		PR.PRPC &= ~(get_mask(usart));

		// Disable rx and tx
		usart->CTRLB &= ~USART_RXEN_bm;
		usart->CTRLB &= ~USART_TXEN_bm;

		// Configure IO pins
		configure_io(usart, config->mode);

		// Set to SPI Master
		usart->CTRLC |= USART_CMODE_MSPI_gc;

		// Set clock phase
		if ((config->mode == SPI_MODE_CLK_LO_PHA_HI) ||
			(config->mode == SPI_MODE_CLK_HI_PHA_HI)) {
			usart->CTRLC |= USART_UCPHA_bm;
		} else {
			usart->CTRLC &= ~USART_UCPHA_bm;
		}

		// Set endianness
		if (config->endian == ENDIAN_LSB) {
			usart->CTRLC |= USART_DORD_bm;
		} else {
			usart->CTRLC &= ~USART_DORD_bm;
		}

		u16 baud = 0;

		if (config->baudrate < (F_CPU / 2)) {
			baud = (F_CPU / (config->baudrate * 2)) - 1;
		}

		usart->BAUDCTRLB = (u8)((~USART_BSCALE_gm) & (baud & 0xF00) >> 8);
		usart->BAUDCTRLA = (u8)(baud);

		// Enable
		usart->CTRLB |= USART_TXEN_bm | USART_RXEN_bm;

	} // ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
}

void usart_set_tx(USART_t* usart, bool enable) {
	assert(usart);

	if (enable) {
		usart->CTRLB |= USART_TXEN_bm;
	} else {
		usart->CTRLB &= ~USART_TXEN_bm;
	}
}

void usart_set_rx(USART_t* usart, bool enable) {
	assert(usart);

	if (enable) {
		usart->CTRLB |= USART_RXEN_bm;
	} else {
		usart->CTRLB &= ~USART_RXEN_bm;
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

static void configure_io(USART_t* usart, spi_mode_e mode) {
	// Default pins are  SCK = 1, RX = 2, TX = 3
	// Remapped pins are SCK = 5, RX = 6, TX = 7

	bool	remap = false;
	PORT_t* port  = NULL;

	if (usart == &USARTC0) {
		port  = &PORTC;
		remap = (PORTC.REMAP & PORT_USART0_bm);
	} else if (usart == &USARTC1) {
		port  = &PORTC;
		remap = true;
	} else if (usart == &USARTD0) {
		port  = &PORTD;
		remap = (PORTD.REMAP & PORT_USART0_bm);
	} else if (usart == &USARTD1) {
		port  = &PORTD;
		remap = true;
	} else if (usart == &USARTE0) {
		port  = &PORTE;
		remap = (PORTE.REMAP & PORT_USART0_bm);
	}

	const u8   sck = (remap ? 5 : 1);
	const u8   rx  = (remap ? 6 : 2);
	const u8   tx  = (remap ? 7 : 3);
	const bool invert_sck =
		(mode == SPI_MODE_CLK_HI_PHA_LO || mode == SPI_MODE_CLK_HI_PHA_HI);

	gpio_mode(port, sck, PORT_OPC_TOTEM_gc | (invert_sck ? (0x01 << 6) : 0));

	gpio_dir(port, sck, GPIO_OUTPUT);
	gpio_dir(port, rx, GPIO_INPUT);
	gpio_dir(port, tx, GPIO_OUTPUT);
	gpio_set(port, sck, 1);
}
