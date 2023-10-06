/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>
#include <util/atomic.h>

#include "drivers/switch.h"
#include "drivers/hw_encoder.h"

#include "input/encoder.h"

#include "hal/avr/xmega/128a4u/gpio.h"

#include "board/djtt/midifighter.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PORT_SR_ENC (PORTC) // IO port for encoder IO shift registers

#define PIN_SR_ENC_LATCH   (0) // 74HC595N
#define PIN_SR_ENC_CLOCK   (1)
#define PIN_SR_ENC_DATA_IN (2)

#define MASK_INDICATORS        (0xFFE0)
#define CLEAR_LEFT_INDICATORS  (0x03FF) // Mask to clear mid and left-side leds
#define CLEAR_RIGHT_INDICATORS (0xF80F) // Mask to clear mid and right-side leds

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef union {
  struct {
    uint16_t detent_blue  : 1;
    uint16_t detent_red   : 1;
    uint16_t rgb_blue     : 1;
    uint16_t rgb_red      : 1;
    uint16_t rgb_green    : 1;
    uint16_t indicator_11 : 1;
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
  };
  uint16_t state;
} encoder_led_t;

typedef enum {
  ENCODER_MODE_MIDI_CC,
  ENCODER_MODE_MIDI_REL_CC,
  ENCODER_MODE_MIDI_NOTE,

  ENCODER_MODE_DISABLED,

  ENCODER_MODE_NB,
} encoder_mode_e;

/**
 * @brief The LED mode determines how the current value of the encoder should be
 * displayed via the LEDs.
 *
 * LED_MODE_SINGLE  - A single LED will be lit to display the value
 * LED_MODE_TRI     - Three LEDs will be lit to display the value (the middle
 * led is brightest) LED_MODE_MULTI   - All leds are light in sequence.
 * LED_MODE_MULTI_PWM - As above, but the brightness of the final LED will be
 * adjusted to display intermediate values.
 *
 * The modes also apply when the encoder detent mode is enabled.
 *
 */
enum {
  LED_MODE_SINGLE,
  LED_MODE_MULTI,
  LED_MODE_MULTI_PWM,

  LED_MODE_NB,
};

typedef struct {
  uint8_t mode   : 7;
  uint8_t detent : 1;
} led_mode_t;

typedef struct {
  uint8_t channel;
  uint8_t value; // cc, or note value
} mf_midi_cfg_t;

typedef struct {
  uint8_t        update;
  led_mode_t     led_mode;
  encoder_mode_e encoder_mode;

  union { // Union of the various mode "configurations"
    mf_midi_cfg_t midi;
  } cfg;

} mf_encoder_ctx_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static const uint16_t indicator_interval =
    ((ENC_MAX - ENC_MIN) / (MF_NUM_INDICATOR_LEDS - 1));

static hw_encoder_ctx_t hw_ctx[MF_NUM_ENCODERS];
static encoder_ctx_t    sw_ctx[MF_NUM_ENCODERS];
static mf_encoder_ctx_t mf_ctx[MF_NUM_ENCODERS];
static switch_x16_ctx_t switch_ctx;

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
    mf_ctx[i].encoder_mode = ENCODER_MODE_MIDI_CC;
    mf_ctx[i].update       = true;
  }
}

// Scan the hardware state of the midifighter and update local contexts
void mf_encoder_update(void) {

  // 1. Latch the IO levels into the shift registers
  gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 1);

  // 2. Clock the 16 data bits for the encoder switches
  uint16_t swstates = 0;
  for (size_t i = 0; i < MF_NUM_ENCODER_SWITCHES; i++) {
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);
    uint8_t state = !(bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
    swstates |= (state << i);
  }

  // 3. Clock the 32 bits for the 2x16 quadrature encoder signals
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

    // Capture changes in rotation and process them
    if (mf_ctx[i].encoder_mode != ENCODER_MODE_DISABLED) {
      encoder_update(&sw_ctx[i], direction);
      sw_ctx[i].curr_val += i;
    }
  }

  // Close the door!
  gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 0);

  // Execute the debounce and update routine for the switches
  switch_x16_update(&switch_ctx, swstates);
  switch_x16_debounce(&switch_ctx);

  // Check if the hardware state of the encoder changed, set the update flag is
  // true
  for (int i = 0; i < MF_NUM_ENCODERS; ++i) {
    if (sw_ctx[i].changed) {
      mf_ctx[i].update = true;
    }

    const uint16_t mask = (1u << i);

    if ((switch_ctx.current & mask) != (switch_ctx.previous & mask)) {
      mf_ctx[i].update = true;
    }
  }
}

// Update the state of the encoder LEDS
void mf_encoder_led_update(void) {
  bool update_leds = false;
  for (int i = 0; i < MF_NUM_ENCODERS; ++i) {
    if (!mf_ctx[i].update) {
      continue;
    }

    encoder_led_t leds = {0};

    switch (mf_ctx[i].led_mode.mode) {
      case LED_MODE_SINGLE: {
        unsigned int indicator = (sw_ctx[i].curr_val / indicator_interval);
        leds.state             = MASK_INDICATORS & (0x8000 >> indicator);
        break;
      }

      case LED_MODE_MULTI: {
        unsigned int num_indicators =
            1 + (sw_ctx[i].curr_val / indicator_interval);
        leds.state = MASK_INDICATORS & ~(0xFFFF >> num_indicators);
        break;
      }

      case LED_MODE_MULTI_PWM: {
        unsigned int num_indicators =
            1 + (sw_ctx[i].curr_val / indicator_interval);
        leds.state = MASK_INDICATORS & ~(0xFFFF >> num_indicators);
        break;
        break;
      }
      default: return;
    }

    if (switch_ctx.current & (1u << i)) {
      leds.rgb_red = 1;
    }

    mf_frame_buf[0][i]  = ~leds.state;
    mf_ctx[i].update    = false;
    update_leds         = true;
  }

  // Set the DMA to transmit only if something changed
  if (update_leds) {
    update_leds = false;
    mf_led_transmit();
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
