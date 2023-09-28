/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>

#include "hal/avr/xmega/128a4u/gpio.h"
#include "hal/avr/xmega/128a4u/dma.h"
#include "hal/avr/xmega/128a4u/usart.h"

#include "drivers/encoder.h"

#include "board/djtt/midifighter.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PORT_SW     (PORTA) // IO port for side-switches
#define PORT_SR_ENC (PORTC) // IO port for encoder IO shift registers
#define PORT_SR_LED (PORTD) // IO port for led shift registers

#define PIN_SR_ENC_LATCH   (0)
#define PIN_SR_ENC_CLOCK   (1)
#define PIN_SR_ENC_DATA_IN (2)

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

static encoder_hwctx_t encoder_ctx[MF_NUM_ENCODERS];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int mf_hardware_init(void) {
  // Configure gpios for side switches
  for (int i = 0; i < MF_NUM_SIDE_SWITCHES; ++i) {
    gpio_dir(&PORT_SW, i, GPIO_INPUT);
    gpio_mode(&PORT_SW, i, PORT_OPC_PULLUP_gc);
  }

  // Configure GPIO for encoder IO shift regsiters
  gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_LATCH, GPIO_OUTPUT);
  gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, GPIO_OUTPUT);
  gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN, GPIO_INPUT);

  // Latch initial encoder data
  gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 1);
  gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 0);

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

  return 0;
}

void mf_update_encoders(void) {
  for (int i = 0; i < MF_NUM_ENCODERS; ++i) {
    encoder_update(&encoder_ctx[i]);
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
