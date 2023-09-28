/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>
#include <util/atomic.h>

#include "hal/avr/xmega/128a4u/timer.h"
#include "system/system.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static uint8_t     get_bitmask(timer_peripheral_e periph);
static register8_t get_power_reg(timer_peripheral_e periph);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void timer_init(TC0_t* timer, const timer_config_t* config) {
  assert(timer);
  assert(config);

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    // Enable power
    register8_t   pp = get_power_reg(config->periph);
    const uint8_t bm = get_bitmask(config->periph);
    pp |= bm;

    //  Set WGM
    timer->CTRLB = (timer->CTRLB & ~TC0_WGMODE_gm) | config->wgm_mode;

    // Set clocksource
    timer->CTRLA = (timer->CTRLA & ~TC0_CLKSEL_gm) | config->clk_sel;
  }
}

void timer_ch_isr_enable(TC0_t* timer, timer_channel_e channel,
                         isr_priority_e priority) {
  assert(timer);

  const uint8_t shift = channel << 1;
  const uint8_t mask  = (TC0_CCAINTLVL_gm) << shift;
  timer->INTCTRLB     = (timer->INTCTRLB & ~mask) | (priority << shift);
}

void timer_ch_isr_disable(TC0_t* timer, timer_channel_e channel) {
  assert(timer);
  const uint8_t shift = channel << 2;
  const uint8_t mask  = (TC0_CCAINTLVL_gm) << shift;
  timer->INTCTRLB &= ~mask;
}

void timer_ovr_isr_enable(TC0_t* timer, isr_priority_e priority) {
  assert(timer);
  timer->INTCTRLA = (timer->INTCTRLA & ~TC0_OVFINTLVL_gm) | priority;
}

void timer_ovr_isr_disable(TC0_t* timer) {
  assert(timer);
  timer->INTCTRLA &= ~TC0_OVFINTLVL_gm;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static uint8_t get_bitmask(timer_peripheral_e periph) {
  switch (periph) {
  // case TIMER_TCE0:
  case TIMER_TCC0:
  case TIMER_TCC2:
  case TIMER_TCD0:
  case TIMER_TCD2: {
    return PR_TC0_bm;
  }
  case TIMER_TCC1:
  case TIMER_TCD1: {
    return PR_TC1_bm;
  }

  default: return 0;
  }

  return 0;
}

static register8_t get_power_reg(timer_peripheral_e periph) {
  switch (periph) {
  case TIMER_TCC0:
  case TIMER_TCC1:
  case TIMER_TCC2: {
    return PR.PRPC;
  }

  case TIMER_TCD0:
  case TIMER_TCD1:
  case TIMER_TCD2: {
    return PR.PRPD;
  }

    // case TIMER_TCE0: {
    //   return PR.PRPE;
    // }
  }
}
