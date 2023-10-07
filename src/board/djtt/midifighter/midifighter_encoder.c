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
#include "display/rgb.h"

#include "hal/avr/xmega/128a4u/gpio.h"

#include "board/djtt/midifighter.h"
#include "board/djtt/midifighter_colours.h"

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
 * INDICATOR_MODE_SINGLE  - A single LED will be lit to display the value
 * INDICATOR_MODE_TRI     - Three LEDs will be lit to display the value (the
 * middle led is brightest) INDICATOR_MODE_MULTI   - All leds are light in
 * sequence. INDICATOR_MODE_MULTI_PWM - As above, but the brightness of the
 * final LED will be adjusted to display intermediate values.
 *
 * The modes also apply when the encoder detent mode is enabled.
 *
 */
enum {
  INDICATOR_MODE_SINGLE,
  INDICATOR_MODE_MULTI,
  INDICATOR_MODE_MULTI_PWM,

  INDICATOR_MODE_NB, // max is currently 4 (see led_mode_t bitfield)
};

typedef struct {
  uint8_t indicator : 2;
} led_mode_t;

typedef struct {
  uint8_t channel;
  uint8_t value; // cc, or note value
} mf_midi_cfg_t;

typedef struct {
  uint8_t        update;
  led_mode_t     led_mode;
  encoder_mode_e encoder_mode;
  rgb_15_bit_t   rgb_state;
  uint8_t        detent;

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

static uint8_t global_brightness = MF_MAX_BRIGHTNESS;

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
    mf_ctx[i].encoder_mode       = ENCODER_MODE_MIDI_CC;
    mf_ctx[i].update             = true;
    mf_ctx[i].led_mode.indicator = (i % INDICATOR_MODE_NB);
    mf_ctx[i].rgb_state.value    = MF_RGB_WHITE;
    mf_ctx[i].detent             = true;
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

    // Check the switch state for the current encoder (the state of each switch
    // is stored in a bitfield)
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

    encoder_led_t leds;
    leds.state = 0;

    // Calculate which indicators need to be on
    float num_indicators = (sw_ctx[i].curr_val / indicator_interval);
    int   int_indicators = (int)num_indicators;

    // Set indicators
    switch (mf_ctx[i].led_mode.indicator) {
      case INDICATOR_MODE_SINGLE: {
        leds.state = MASK_INDICATORS & (0x8000 >> int_indicators);
        break;
      }

      case INDICATOR_MODE_MULTI:
      case INDICATOR_MODE_MULTI_PWM: {
        int_indicators += 1;
        leds.state = MASK_INDICATORS & ~(0xFFFF >> int_indicators);
        if (mf_ctx[i].detent) {
          leds.detent_blue = 1;
          if (int_indicators > 5) {
            leds.state &= CLEAR_LEFT_INDICATORS;
          } else if (int_indicators < 7) {
            leds.state ^= 0xFC00;
            leds.state &= CLEAR_RIGHT_INDICATORS;
          }
          break;
        }

        default: return;
      }
    }

    // Handle PWM for RGB colours
    for (size_t p = 0; p < MF_NUM_PWM_FRAMES; ++p) {
      leds.rgb_blue = leds.rgb_red = leds.rgb_green = 0;
      if ((int)(mf_ctx[i].rgb_state.red - p) > 0) {
        leds.rgb_red = 1;
      }
      if ((int)(mf_ctx[i].rgb_state.green - p) > 0) {
        leds.rgb_green = 1;
      }
      if ((int)(mf_ctx[i].rgb_state.blue - p) > 0) {
        leds.rgb_blue = 1;
      }
      mf_frame_buf[p][i] = ~leds.state;
    }

    mf_ctx[i].update = false;
    update_leds      = true;
  }

  // Set the DMA to transmit only if something changed
  if (update_leds) {
    update_leds = false;
    mf_led_transmit();
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
