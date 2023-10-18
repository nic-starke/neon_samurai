/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>
#include <util/atomic.h>
#include <math.h>

#include "core/core_types.h"
#include "core/core_event.h"
#include "core/core_encoder.h"
#include "core/core_switch.h"
#include "core/core_rgb.h"

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
  indicator_style_e led_mode;
  encoder_mode_e    encoder_mode;
  rgb_15_t          rgb_state;
  u8                detent;

  union { // Union of the various mode "configurations"
    mf_midi_cfg_t midi;
  } cfg;

} mf_encoder_ctx_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Event handler for encoder change events
static void encoder_evt_handler(event_s* event);

static void update_display(u8 index);

static u8 max_brightness = MF_MAX_BRIGHTNESS;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static quadrature_ctx_t   hw_ctx[MF_NUM_ENCODERS];
static encoder_ctx_s      sw_ctx[MF_NUM_ENCODERS];
static mf_encoder_ctx_t   mf_ctx[MF_NUM_ENCODERS];
static switch_x16_ctx_s   switch_ctx;

static const u16 led_interval = ENC_MAX / 11;

// Event handlers
static event_handler_s enc_evt_handler = {
    .priority = 0,
    .handler  = encoder_evt_handler,
    .next     = NULL,
};

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
    mf_ctx[i].led_mode     = (i % INDICATOR_MODE_NB);

    //  Randomly set the colour of each encoder RGB.
    mf_ctx[i].rgb_state.red   = rand() % MF_RGB_MAX_VAL;
    mf_ctx[i].rgb_state.green = rand() % MF_RGB_MAX_VAL;
    mf_ctx[i].rgb_state.blue  = rand() % MF_RGB_MAX_VAL;

    mf_ctx[i].detent   = true;
    sw_ctx[i].index    = i;
    sw_ctx[i].curr_val = ENC_MID;
    sw_ctx[i].accel_mode = (i % 2 == 0);
  }

  // Subscribe to encoder change events
  event_subscribe(&enc_evt_handler, EVT_ENCODER_ROTATION);
  event_subscribe(&enc_evt_handler, EVT_ENCODER_SWITCH_STATE);
  event_subscribe(&enc_evt_handler, EVT_MAX_BRIGHTNESS);

  // Post the initial event to update display
  for (int i = 0; i < MF_NUM_ENCODERS; ++i) {
    event_s evt = {
        .id = EVT_ENCODER_ROTATION,
        .data.encoder =
            {
                .current_value = sw_ctx[i].curr_val,
                .encoder_index = i,
            },
    };
    event_post(&evt);
  }
}

// Scan the hardware state of the midifighter and update local contexts
void mf_encoder_update(void) {

  // Latch the IO levels into the shift registers
  gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 1);

  // Clock the 16 data bits for the encoder switches
  u16 swstates = 0;
  for (size_t i = 0; i < MF_NUM_ENCODER_SWITCHES; i++) {
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);
    u8 state = !(bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
    swstates |= (state << i);
  }

  // Clock the 32 bits for the 2x16 quadrature encoder signals, and update
  // encoder state.
  for (int i = 0; i < MF_NUM_ENCODERS; ++i) {
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
    u8 ch_a = (bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
    u8 ch_b = (bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
    gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);

    core_quadrature_decode(&hw_ctx[i], ch_a, ch_b);

    i16 direction = 0;
    if (hw_ctx[i].dir != DIR_ST) {
      direction = hw_ctx[i].dir == DIR_CW ? 1 : -1;
    }

    // Process encoder changes
    if (mf_ctx[i].encoder_mode != ENCODER_MODE_DISABLED) {
      core_encoder_update(&sw_ctx[i], direction);
    }
  }

  // Close the door!
  gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 0);

  // Execute the debounce and update routine for the switches
  switch_x16_update(&switch_ctx, swstates);
  switch_x16_debounce(&switch_ctx);

  // Process switch changes
  for (int i = 0; i < MF_NUM_ENCODERS; ++i) {
    u16 mask      = (1u << i);
    u8  prevstate = (switch_ctx.previous & mask) ? 1 : 0;
    u8  state     = (switch_ctx.current & mask) ? 1 : 0;
    if (state != prevstate) {
      event_s evt = {
          .id = EVT_ENCODER_SWITCH_STATE,
          .data =
              {
                  .sw =
                      {
                          .switch_index = i,
                          .state        = state,
                      },
              },
      };
      event_post(&evt);
    }
  }
}

void mf_debug_encoder_set_indicator(u8 indicator, u8 state) {
  const u8 debug_encoder = 0;
  assert(indicator < MF_NUM_INDICATOR_LEDS);
  encoder_led_t leds;
  leds.state = 0; // turn off all leds

  switch (indicator) {
    case 0: leds.indicator_1 = state; break;
    case 1: leds.indicator_2 = state; break;
    case 2: leds.indicator_3 = state; break;
    case 3: leds.indicator_4 = state; break;
    case 4: leds.indicator_5 = state; break;
    case 5: leds.indicator_6 = state; break;
    case 6: leds.indicator_7 = state; break;
    case 7: leds.indicator_8 = state; break;
    case 8: leds.indicator_9 = state; break;
    case 9: leds.indicator_10 = state; break;
    case 10: leds.indicator_11 = state; break;
  }

  for (int i = 0; i < MF_NUM_PWM_FRAMES; ++i) {
    mf_frame_buf[i][debug_encoder] = ~leds.state;
  }
}

void mf_debug_encoder_set_rgb(bool red, bool green, bool blue) {
  const u8      debug_encoder = 0;
  encoder_led_t leds;
  leds.state     = 0;
  leds.rgb_blue  = blue;
  leds.rgb_red   = red;
  leds.rgb_green = green;

  for (int i = 0; i < MF_NUM_PWM_FRAMES; ++i) {
    mf_frame_buf[i][debug_encoder] |= ~leds.state;
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void update_display(u8 index) {
  f32           ind_pwm;    // Partial encoder positions (e.g position = 5.7)
  unsigned int  ind_norm;   // Integer encoder positions (e.g position = 5)
  unsigned int  pwm_frames; // Number of PWM frames for partial brightness
  encoder_led_t leds;       // LED states

  // Clear all LEDs
  leds.state = 0;

  // Determine the position of the indicator led and which led requires
  // partial brightness
  if (sw_ctx[index].curr_val <= led_interval) {
    ind_pwm  = 0;
    ind_norm = 0;
  } else if (sw_ctx[index].curr_val == ENC_MAX) {
    ind_pwm  = MF_NUM_INDICATOR_LEDS;
    ind_norm = MF_NUM_INDICATOR_LEDS;
  } else {
    ind_pwm  = ((f32)sw_ctx[index].curr_val / led_interval);
    ind_norm = (unsigned int)roundf(ind_pwm);
  }

  // Generate the LED states based on the display mode
  switch (mf_ctx[index].led_mode) {
    case INDICATOR_MODE_SINGLE: {
      // Set the corresponding bit for the indicator led
      leds.state = MASK_INDICATORS & (0x8000 >> (ind_norm - 1));
      break;
    }

    case INDICATOR_MODE_MULTI: {
      if (mf_ctx[index].detent) {
        if (ind_norm < 6) {
          leds.state |=
              (LEFT_INDICATORS_MASK >> (ind_norm - 1)) & LEFT_INDICATORS_MASK;
        } else if (ind_norm > 6) {
          leds.state |=
              (LEFT_INDICATORS_MASK >> (ind_norm - 5)) & RIGHT_INDICATORS_MASK;
        }
      } else {
        // Default mode - set the indicator leds upto "led_index"
        leds.state = MASK_INDICATORS & ~(0xFFFF >> ind_norm);
      }
      break;
    }

    case INDICATOR_MODE_MULTI_PWM: {
      f32 diff   = ind_pwm - (floorf(ind_pwm));
      pwm_frames = (unsigned int)((diff)*MF_NUM_PWM_FRAMES);
      if (mf_ctx[index].detent) {
        if (ind_norm < 6) {
          leds.state |=
              (LEFT_INDICATORS_MASK >> (ind_norm - 1)) & LEFT_INDICATORS_MASK;
        } else if (ind_norm > 6) {
          leds.state |=
              (LEFT_INDICATORS_MASK >> (ind_norm - 5)) & RIGHT_INDICATORS_MASK;
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
  if (mf_ctx[index].encoder_mode != ENCODER_MODE_DISABLED) {
    mf_ctx[index].rgb_state.value = 0;
    // uint8_t multi = abs(((float)sw_ctx[index].velocity / ENC_MAX_VELOCITY) *
    //                     MF_RGB_MAX_VAL);
    // if (sw_ctx[index].velocity > 0) {
    //   mf_ctx[index].rgb_state.red = multi;
    // } else if (sw_ctx[index].velocity < 0) {
    //   mf_ctx[index].rgb_state.blue = multi;
    // } else {
    //   mf_ctx[index].rgb_state.green = MF_RGB_MAX_VAL;
    // }

    // RGB colour based on current value
    u8 brightness =
        (u8)((f32)sw_ctx[index].curr_val / ENC_MAX * MF_RGB_MAX_VAL);
    // mf_ctx[index].rgb_state.blue  = brightness;
    // mf_ctx[index].rgb_state.green = brightness;
    mf_ctx[index].rgb_state.red = brightness;
  }

  // When detent mode is enabled the detent LEDs must be displayed, and
  // indicator LED #6 at the 12 o'clock position must be turned off.
  if (mf_ctx[index].detent && (ind_norm == 6)) {
    leds.indicator_6 = 0;
    leds.detent_blue = 1;
  }

  // Handle PWM for RGB colours, MULTI_PWM mode, and global max brightness
  for (unsigned int p = 0; p < MF_NUM_PWM_FRAMES; ++p) {
    // When the brightness is below 100% then begin to dim the LEDs.
    if (max_brightness < p) {
      leds.state             = 0;
      mf_frame_buf[p][index] = ~leds.state;
      continue;
    }

    if (mf_ctx[index].led_mode == INDICATOR_MODE_MULTI_PWM) {
      if (mf_ctx[index].detent) {
        if (ind_norm < 6) {
          if (p < pwm_frames) {
            leds.state &=
                ~((0x8000 >> (int)(ind_pwm - 1)) & MASK_PWM_INDICATORS);
          } else {
            leds.state |=
                ((0x8000 >> (int)(ind_pwm - 1)) & MASK_PWM_INDICATORS);
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
    if ((int)(mf_ctx[index].rgb_state.red - p) > 0) {
      leds.rgb_red = 1;
    }
    if ((int)(mf_ctx[index].rgb_state.green - p) > 0) {
      leds.rgb_green = 1;
    }
    if ((int)(mf_ctx[index].rgb_state.blue - p) > 0) {
      leds.rgb_blue = 1;
    }
    mf_frame_buf[p][index] = ~leds.state;
  }
}

static void encoder_evt_handler(event_s* event) {
  assert(event);
  switch (event->id) {
    case EVT_ENCODER_ROTATION: {
      update_display(event->data.encoder.encoder_index);
      break;
    }

    case EVT_ENCODER_SWITCH_STATE: {
      u8 index               = event->data.sw.switch_index;
      mf_ctx[index].led_mode = (mf_ctx[index].led_mode + 1) % INDICATOR_MODE_NB;
      update_display(index);
      break;
    }

    case EVT_MAX_BRIGHTNESS: {
      max_brightness = event->data.max_brightness;
      break;
    }
  }
}
