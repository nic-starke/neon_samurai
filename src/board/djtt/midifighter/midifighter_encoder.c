/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>

#include "drivers/encoder.h"
#include "input/encoder.h"

#include "hal/avr/xmega/128a4u/gpio.h"

#include "board/djtt/midifighter.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PORT_SR_ENC (PORTC) // IO port for encoder IO shift registers

#define PIN_SR_ENC_LATCH   (0)
#define PIN_SR_ENC_CLOCK   (1)
#define PIN_SR_ENC_DATA_IN (2)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static hw_encoder_ctx_t hw_ctx[MF_NUM_ENCODERS];
static encoder_ctx_t    sw_ctx[MF_NUM_ENCODERS];
static encoder_desc_t   desc = {
      .sw_ctx = sw_ctx,
      .hw_ctx = hw_ctx,
      .count  = MF_NUM_ENCODERS,
};
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void mf_encoder_init(void) {
  // Configure GPIO for encoder IO shift regsiters
  gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_LATCH, GPIO_OUTPUT);
  gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, GPIO_OUTPUT);
  gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN, GPIO_INPUT);

  // Latch initial encoder data
  gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 1);
  gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 0);

  // Initialise encoder
  int status = encoder_init(&desc);
  RETURN_ON_ERR(status);
}

void mf_hw_update(void) {
  // Poll the shift registers, then  update hw ctx
  for (int i = 0; i < MF_NUM_ENCODERS; i++) {
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
    uint8_t ch_a = gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
    uint8_t ch_b = gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);

    hw_encoder_update(&hw_ctx[i], ch_a, ch_b);
  }
}

void mf_encoder_update(void) {
  for (int i = 0; i < MF_NUM_ENCODERS; i++) {
    encoder_update();
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
