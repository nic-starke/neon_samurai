/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>

#include "hal/avr/xmega/128a4u/dma.h"
#include "hal/avr/xmega/128a4u/usart.h"

#include "board/djtt/midifighter.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PORT_SR_LED (PORTD) // IO port for led shift registers

#define PIN_SR_LED_ENABLE   (0)
#define PIN_SR_LED_CLOCK    (1)
#define PIN_SR_LED_DATA_OUT (3)
#define PIN_SR_LED_LATCH    (4)
#define PIN_SR_LED_RESET_N  (5)

#define USART_BAUD (4000000)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void mf_led_init(void) {
  // Configure GPIO for LED shift registers
  gpio_dir(&PORT_SR_LED, PIN_SR_LED_ENABLE, GPIO_OUTPUT);
  gpio_dir(&PORT_SR_LED, PIN_SR_LED_CLOCK, GPIO_OUTPUT);
  gpio_dir(&PORT_SR_LED, PIN_SR_LED_DATA_OUT, GPIO_OUTPUT);
  gpio_dir(&PORT_SR_LED, PIN_SR_LED_LATCH, GPIO_OUTPUT);
  gpio_dir(&PORT_SR_LED, PIN_SR_LED_RESET_N, GPIO_OUTPUT);

  // Configure USART (SPI) for LED shift registers
  const usart_config_t usart_cfg = {
      .baudrate = USART_BAUD,
      .endian   = ENDIAN_LSB,
      .mode     = SPI_MODE_CLK_LO_PHA_LO,
  };

  usart_module_init(&USARTD0, &usart_cfg);

  // Reset the LEDs (active low)
  gpio_set(&PORT_SR_LED, PIN_SR_LED_RESET_N, 0);
  gpio_set(&PORT_SR_LED, PIN_SR_LED_RESET_N, 1);

  /*
    Setup the dma for the following:
    1 byte burst
    block size is 32 (32 shift registers, 2 per encoder)
    repeat count is number of display buffer frames (per encoder)
    trigger - usart empty
    reload address = end of transaction
    interrupt - end of block

    This "should" result in:
      1 byte of frame data being transferred to the 1 byte usart input buffer.
      x32 times, until a single frame is transffered for all encoders.
      Then reloading the starting address when the transaction is complete.

      But also firing an interrupt when each "frame" is transferred
      The interrupt can then latch the SRs.
  */
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
