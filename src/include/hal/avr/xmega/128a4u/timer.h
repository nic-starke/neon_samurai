/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>

#include "system/types.h"

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
  // TIMER_TCE0, // Reserved for RTOS

  TIMER_NB,
} timer_peripheral_e;

typedef enum {
  TIMER_CHANNEL_A,
  TIMER_CHANNEL_B,
  TIMER_CHANNEL_C,
  TIMER_CHANNEL_D,

  TIMER_CHANNEL_NB,
} timer_channel_e;

typedef struct {
  timer_peripheral_e periph;
  TC_WGMODE_t        wgm_mode;
  TC_CLKSEL_t        clk_sel;
} timer_config_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void timer_init(TC0_t* timer, const timer_config_t* config);
void timer_ch_isr_enable(TC0_t* timer, timer_channel_e channel,
                         isr_priority_e priority);
void timer_ch_isr_disable(TC0_t* timer, timer_channel_e channel);
void timer_ovr_isr_enable(TC0_t* timer, isr_priority_e priority);
void timer_ovr_isr_disable(TC0_t* timer);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
