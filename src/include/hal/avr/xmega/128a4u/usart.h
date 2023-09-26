/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>
#include "system/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// typedef enum {
//   UART_MODULE_C0,
//   UART_MODULE_C1,
//   UART_MODULE_D0,
//   UART_MODULE_D1,
//   UART_MODULE_E0,

//   UART_MODULE_NB,
// } uart_module_e;

typedef enum {
  SPI_MODE_CLK_LO_PHA_LO,
  SPI_MODE_CLK_LO_PHA_HI,
  SPI_MODE_CLK_HI_PHA_LO,
  SPI_MODE_CLK_HI_PHA_HI,

  SPI_MODE_NB,
} spi_mode_e;

typedef struct {
  spi_mode_e mode;
  uint32_t   baudrate;
  endian_e   endian;
} usart_config_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void usart_peripheral_init(void);

/**
 * @brief Initialise a USART peripheral (such as USARTC0) in Master SPI Mode
 * IMPORTANT!! Only call this after the CPU clock is configured to run at F_CPU.
 * Does not start UART tx/rx.
 * GPIO will be configured based on the port and PORT.REMAP
 *
 * Default pinout is:
 * Pin 1 - SCK
 * Pin 2 - RX
 * Pin 3 - TX
 *
 * Remapped pinout is:
 * Pin 5 - SCK
 * Pin 6 - RX
 * Pin 7 - TX
 *
 * @param config A pointer to a USART config.
 */
void usart_module_init(USART_t* usart, const usart_config_t* config);

// Enable or disable USART module transmission.
void usart_set_tx(USART_t* usart, bool enable);

// Enable or disable UART module receive.
void usart_set_rx(USART_t* usart, bool enable);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
