/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>

#include "drivers/hw_encoder.h"
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

typedef union {
  struct {
    uint16_t rgb_green    : 1;
    uint16_t indicator_10 : 1;
    uint16_t indicator_9  : 1;
    uint16_t indicator_8  : 1;
    uint16_t indicator_7  : 1;
    uint16_t indicator_6  : 1;
    uint16_t indicator_5  : 1;
    uint16_t indicator_4  : 1;
    uint16_t indicator_3  : 1;
    uint16_t indicator_2  : 1;
    uint16_t indicator_1  : 1;
    uint16_t indicator_0  : 1;
    uint16_t detent_blue  : 1;
    uint16_t detent_red   : 1;
    uint16_t rgb_blue     : 1;
    uint16_t rgb_red      : 1;
  };
  uint16_t state;
} encoder_led_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
static const uint16_t indicator_interval =
    ((ENC_MAX - ENC_MIN) / MF_NUM_INDICATOR_LEDS);
static hw_encoder_ctx_t hw_ctx[MF_NUM_ENCODERS];
static encoder_ctx_t    sw_ctx[MF_NUM_ENCODERS];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void mf_encoder_init(void) {
  // Configure GPIO for encoder IO shift regsiters
  gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_LATCH, GPIO_OUTPUT);
  gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, GPIO_OUTPUT);
  gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN, GPIO_INPUT);

  // Latch initial encoder data
  gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 0);
  gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 1);

  // Set the encoder configurations
  for (int i = 0; i < MF_NUM_ENCODERS; ++i) {
    sw_ctx[i].enabled = true;
    sw_ctx[i].changed = true;
  }
}

void mf_encoder_update(void) {
  gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 1);

  // remember - clock ALL shift registers,
  for (size_t i = 0; i < MF_NUM_ENCODER_SWITCHES; i++) {
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);
  }

  for (int i = 0; i < MF_NUM_ENCODERS; ++i) {
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
    uint8_t ch_a = (bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
    uint8_t ch_b = (bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);

    hw_encoder_update(&hw_ctx[i], ch_a, ch_b);

    int16_t direction = 0;
    if (hw_ctx[i].dir != DIR_ST) {
      direction = hw_ctx[i].dir == DIR_CW ? 1 : -1;
    }

    encoder_update(&sw_ctx[i], direction);
  }

  gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 0);
}

void mf_encoder_led_update(void) {
  bool leds_changed = true;

  for (int i = 0; i < MF_NUM_ENCODERS; ++i) {
    encoder_ctx_t* ctx = &sw_ctx[i];

    if (!ctx->enabled || !ctx->changed) {
      continue; // Skip to next encoder
    }

    // Calculate the new state of the LEDs based on current value and mode
    volatile encoder_led_t leds = {0};

    // Set indicator leds (11 small white leds)
    unsigned int num_indicators = (ctx->curr_val / indicator_interval);

    // switch (num_indicators) {
    //   case 11: leds.indicator_10 = 1;
    //   case 10: leds.indicator_9 = 1;
    //   case 9: leds.indicator_8 = 1;
    //   case 8: leds.indicator_7 = 1;
    //   case 7: leds.indicator_6 = 1;
    //   case 6: leds.indicator_5 = 1;
    //   case 5: leds.indicator_4 = 1;
    //   case 4: leds.indicator_3 = 1;
    //   case 3: leds.indicator_2 = 1;
    //   case 2: leds.indicator_1 = 1;
    //   case 1: leds.indicator_0 = 1;
    //   default: break;
    // }

    leds.state = 0xFFE0 & ~(0xFFFF >> num_indicators);

    // Update the frame buffer with the new state of the LEDs
    unsigned int index;
    if (i == 0) {
      index = MF_NUM_ENCODERS - 1 - i;
    } else {
      index = i - 1;
    }

    // Disable usart
    mf_frame_buf[i] = ~leds.state;
    ctx->changed    = false; // Clear the flag
    leds_changed    = true;
  }

  // Set the DMA to transmit only if something changed
  if (leds_changed) {
    mf_led_transmit();
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
