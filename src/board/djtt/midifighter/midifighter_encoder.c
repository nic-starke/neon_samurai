/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>
#include <util/atomic.h>
#include <math.h>

#include "drivers/gpio_switch.h"
#include "drivers/quad_encoder.h"

#include "application/io/encoder.h"
#include "application/display/rgb.h"

#include "hal/avr/xmega/128a4u/gpio.h"

#include "board/djtt/midifighter.h"
#include "board/djtt/midifighter_encoder.h"
#include "board/djtt/midifighter_colours.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PORT_SR_ENC (PORTC) // IO port for encoder IO shift registers

#define PIN_SR_ENC_LATCH   (0) // 74HC595N
#define PIN_SR_ENC_CLOCK   (1)
#define PIN_SR_ENC_DATA_IN (2)

#define MASK_INDICATORS        (0xFFE0)
#define LEFT_INDICATORS_MASK   (0xF800)
#define RIGHT_INDICATORS_MASK  (0x03E0)
#define CLEAR_LEFT_INDICATORS  (0x03FF) // Mask to clear mid and left-side leds
#define CLEAR_RIGHT_INDICATORS (0xF80F) // Mask to clear mid and right-side leds
#define MASK_PWM_INDICATORS    (0xFBE0)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef union {
  struct {
    u16 detent_blue  : 1;
    u16 detent_red   : 1;
    u16 rgb_blue     : 1;
    u16 rgb_red      : 1;
    u16 rgb_green    : 1;
    u16 indicator_11 : 1;
    u16 indicator_10 : 1;
    u16 indicator_9  : 1;
    u16 indicator_8  : 1;
    u16 indicator_7  : 1;
    u16 indicator_6  : 1;
    u16 indicator_5  : 1;
    u16 indicator_4  : 1;
    u16 indicator_3  : 1;
    u16 indicator_2  : 1;
    u16 indicator_1  : 1;
  };
  u16 state;
} encoder_led_t;

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

typedef struct {
  u8 channel;
  u8 value; // cc, or note value
} mf_midi_cfg_t;

typedef struct {
  u8                update;
  indicator_style_e led_mode;
  encoder_mode_e    encoder_mode;
  rgb_15_t          rgb_state;
  u8                detent;

  union { // Union of the various mode "configurations"
    mf_midi_cfg_t midi;
  } cfg;

} mf_encoder_ctx_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static quad_encoder_ctx_t hw_ctx[MF_NUM_ENCODERS];
static encoder_ctx_t      sw_ctx[MF_NUM_ENCODERS];
static mf_encoder_ctx_t   mf_ctx[MF_NUM_ENCODERS];
static switch_x16_ctx_t   switch_ctx;

static const u16 led_interval = ENC_MAX / 11;

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
    mf_ctx[i].led_mode           = (i % INDICATOR_MODE_NB);
    mf_ctx[i].rgb_state.value    = MF_RGB_WHITE;
    mf_ctx[i].detent             = true;
    sw_ctx[i].curr_val           = ENC_MID;
  }
}

// Scan the hardware state of the midifighter and update local contexts
void mf_encoder_update(void) {

  // 1. Latch the IO levels into the shift registers
  gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 1);

  // 2. Clock the 16 data bits for the encoder switches
  u16 swstates = 0;
  for (size_t i = 0; i < MF_NUM_ENCODER_SWITCHES; i++) {
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);
    u8 state = !(bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
    swstates |= (state << i);
  }

  // 3. Clock the 32 bits for the 2x16 quadrature encoder signals
  for (int i = 0; i < MF_NUM_ENCODERS; ++i) {
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
    u8 ch_a = (bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
    u8 ch_b = (bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);

    quad_encoder_update(&hw_ctx[i], ch_a, ch_b);

    i16 direction = 0;
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
    const u16 mask = (1u << i);
    if ((switch_ctx.current & mask) != (switch_ctx.previous & mask)) {
      mf_ctx[i].update = true;
    }
  }
}

// Update the state of the encoder LEDS
void mf_encoder_led_update(void) {
  f32           ind_pwm;    // Partial encoder positions (e.g position = 5.7)
  unsigned int  ind_norm;   // Integer encoder positions (e.g position = 5)
  unsigned int  pwm_frames; // Number of PWM frames for partial brightness
  u16           pwm_mask;   // Mask of the led that requires PWM
  encoder_led_t leds;       // LED states
  bool update_leds = false; // Update flag, only true if encoder state changed

  for (int i = 0; i < MF_NUM_ENCODERS; ++i) {

    // It is not necessary to update the encoder LEDs if its state has not
    // changed since the previous update.
    if (!mf_ctx[i].update) {
      continue;
    }

    // Clear all LEDs
    leds.state = 0;

    // Determine the position of the indicator led and which led requires
    // partial brightness
    if (sw_ctx[i].curr_val <= led_interval) {
      ind_pwm  = 0;
      ind_norm = 0;
    } else if (sw_ctx[i].curr_val == ENC_MAX) {
      ind_pwm  = MF_NUM_INDICATOR_LEDS;
      ind_norm = MF_NUM_INDICATOR_LEDS;
    } else {
      ind_pwm  = ((f32)sw_ctx[i].curr_val / led_interval);
      ind_norm = (unsigned int)roundf(ind_pwm);
    }

    // Generate the LED states based on the display mode
    switch (mf_ctx[i].led_mode) {
      case INDICATOR_MODE_SINGLE: {
        // Set the corresponding bit for the indicator led
        leds.state = MASK_INDICATORS & (0x8000 >> ind_norm - 1);
        break;
      }

      case INDICATOR_MODE_MULTI: {
        if (mf_ctx[i].detent) {
          if (ind_norm < 6) {
            leds.state |=
                (LEFT_INDICATORS_MASK >> (ind_norm - 1)) & LEFT_INDICATORS_MASK;
          } else if (ind_norm > 6) {
            leds.state |= (LEFT_INDICATORS_MASK >> (ind_norm - 5)) &
                          RIGHT_INDICATORS_MASK;
          }
        } else {
          // Default mode - set the indicator leds upto "led_index"
          leds.state = MASK_INDICATORS & ~(0xFFFF >> ind_norm);
        }
        break;
      }

      case INDICATOR_MODE_MULTI_PWM: {
        f32 diff                  = ind_pwm - (floorf(ind_pwm));
        pwm_frames                = (unsigned int)((diff)*MF_NUM_PWM_FRAMES);
        if (mf_ctx[i].detent) {
          if (ind_norm < 6) {
            leds.state |=
                (LEFT_INDICATORS_MASK >> (ind_norm - 1)) & LEFT_INDICATORS_MASK;
          } else if (ind_norm > 6) {
            leds.state |= (LEFT_INDICATORS_MASK >> (ind_norm - 5)) &
                          RIGHT_INDICATORS_MASK;
          }
        } else {
          // Default mode - set the indicator leds upto "led_index"
          leds.state = MASK_INDICATORS & ~(0xFFFF >> ind_norm);
        }
        break;
      }

      default: return;
    }

    // Set the RGB colour based on the velocity of the encoder
    // if (mf_ctx[i].encoder_mode != ENCODER_MODE_DISABLED) {
    //   mf_ctx[i].rgb_state.value = 0;
    //   // uint8_t multi =
    //   //     abs(((float)sw_ctx[i].velocity / ENC_MAX_VELOCITY) *
    //   //     MF_RGB_MAX_VAL);
    //   // if (sw_ctx[i].velocity > 0) {
    //   //   mf_ctx[i].rgb_state.red = multi;
    //   // } else if (sw_ctx[i].velocity < 0) {
    //   //   mf_ctx[i].rgb_state.blue = multi;
    //   // } else {
    //   //   mf_ctx[i].rgb_state.green = MF_RGB_MAX_VAL;
    //   // }

    //   // RGB colour based on current value
    //   // u8 brightness = (u8)((f32)sw_ctx[i].curr_val / ENC_MAX *
    //   MF_RGB_MAX_VAL);
    //   // mf_ctx[i].rgb_state.blue  = brightness;
    //   // mf_ctx[i].rgb_state.green = brightness;
    //   // mf_ctx[i].rgb_state.red   = brightness;
    // }

    // When detent mode is enabled the detent LEDs must be displayed, and
    // indicator LED #6 at the 12 o'clock position must be turned off.
    if (mf_ctx[i].detent && (ind_norm == 6)) {
      leds.indicator_6 = 0;
      leds.detent_blue = 1;
    }

    // Handle PWM for RGB colours and MULTI_PWM mode
    for (unsigned int p = 0; p < MF_NUM_PWM_FRAMES; ++p) {
      if (mf_ctx[i].led_mode == INDICATOR_MODE_MULTI_PWM) {
        if (mf_ctx[i].detent) {
          if (ind_norm < 6) {
            if (p < pwm_frames) {
              leds.state &=
                  ~((0x8000 >> (int)ind_pwm - 1) & MASK_PWM_INDICATORS);
            } else {
              leds.state |=
                  ((0x8000 >> (int)ind_pwm - 1) & MASK_PWM_INDICATORS);
            }

          } else if (ind_norm > 6) {
            if (p < pwm_frames) {
              leds.state |= ((0x8000 >> (int)ind_pwm) & MASK_PWM_INDICATORS);
            } else {
              leds.state &= ~((0x8000 >> (int)ind_pwm) & MASK_PWM_INDICATORS);
            }
          }
        } else {
          if (p < pwm_frames) {
            leds.state |= ((0x8000 >> (int)ind_pwm) & MASK_PWM_INDICATORS);
          } else {
            leds.state &= ~((0x8000 >> (int)ind_pwm) & MASK_PWM_INDICATORS);
          }
        }
      }

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
