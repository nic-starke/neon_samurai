/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define MF_NUM_ENCODERS              (16)
#define MF_NUM_ENCODER_SWITCHES      (MF_NUM_ENCODERS)
#define MF_NUM_SIDE_SWITCHES         (6)
#define MF_NUM_LEDS                  (256)
#define MF_NUM_LEDS_PER_ENCODER      (16)
#define MF_NUM_INDICATOR_LEDS        (11)
#define MF_NUM_LED_SHIFT_REGISTERS   (32)
#define MF_NUM_INPUT_SHIFT_REGISTERS (6)
#define MF_NUM_PWM_FRAMES            (32)

#define MF_MAX_BRIGHTNESS (MF_NUM_PWM_FRAMES)
#define MF_MIN_BRIGHTNESS (1)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

extern volatile u16 mf_frame_buf[MF_NUM_PWM_FRAMES][MF_NUM_ENCODERS];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void mf_led_init(void);
void mf_led_transmit(void);
void mf_led_set_max_brightness(u16 brightness);

void mf_encoder_init(void);
void mf_encoder_update(void);

void mf_switch_init(void);
void mf_switch_update(void);

void mf_usb_init(void);
void mf_usb_start(void);

void mf_debug_encoder_set_indicator(u8 indicator, u8 state);
void mf_debug_encoder_set_rgb(bool red, bool green, bool blue);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
