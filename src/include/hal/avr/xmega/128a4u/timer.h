/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>

#include "core/core_types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
  TIMER_TCC0,
  TIMER_TCC1,
  TIMER_TCC2,
  TIMER_TCD0,
  TIMER_TCD1,
  TIMER_TCD2,
  TIMER_TCE0,

  TIMER_NB,
} timer_peripheral_e;

typedef enum {
  TIMER_CHANNEL_A, // PWM on pin 0
  TIMER_CHANNEL_B, // PWM on pin 1
  TIMER_CHANNEL_C, // PWM on pin 2
  TIMER_CHANNEL_D, // PWM on pin 3

  TIMER_CHANNEL_NB,
} timer_channel_e;

typedef enum {
  TIMER_MODE_OVF,
  TIMER_MODE_PWM,

  TIMER_MODE_NB,
} timer_mode_e;

typedef struct {
  u16 freq; // Desired PWM frequency
  u8  duty; // Desired duty cycle percentage (0 to 100)
} pwm_config_t;

typedef struct {
  volatile TC0_t*    timer;
  timer_peripheral_e periph;
  timer_channel_e             channel;
  u32                    freq; // Timer frequency (Hz)

  timer_mode_e mode;
  union {
    pwm_config_t pwm;
  };

} timer_config_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int timer_init(timer_config_t* cfg);

void timer_ch_isr_enable(timer_config_t* cfg, isr_priority_e priority);
void timer_ch_isr_disable(timer_config_t* cfg);
void timer_ovr_isr_enable(timer_config_t* cfg, isr_priority_e priority);
void timer_ovr_isr_disable(timer_config_t* cfg);

void timer_pwm_start(timer_config_t* cfg);
void timer_pwm_stop(timer_config_t* cfg);
void timer_pwm_set_duty(timer_config_t* cfg, u8 duty);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
