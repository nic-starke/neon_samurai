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

static void         get_parameters(unsigned int freq, TC_CLKSEL_t* clk_sel,
                                   uint16_t* period);
static uint8_t     get_bitmask(timer_peripheral_e periph);
static register8_t* get_power_reg(timer_peripheral_e periph);

static int pwm_get_params(uint16_t freq, TC_CLKSEL_t* clk, uint16_t* per);

static void cc_buffer_set(timer_config_t* cfg, uint16_t val);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int timer_init(timer_config_t* cfg) {
  assert(cfg);

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    register8_t* pp = get_power_reg(cfg->periph);
    *pp &= ~get_bitmask(cfg->periph); // Enable power to timer peripheral

    switch (cfg->mode) {

      case TIMER_MODE_OVF: {
        // Set waveform generation mode
        cfg->timer->CTRLB |= TC_WGMODE_NORMAL_gc;

        // Configure timer period and divisor
        uint16_t    period;
        TC_CLKSEL_t clk_sel;

        get_parameters(cfg->freq, &clk_sel, &period);
        cfg->timer->CTRLA |= clk_sel;
        cfg->timer->PER = period;
        break;
      }

      case TIMER_MODE_PWM: {
        // Configure timer period and divisor
        TC_CLKSEL_t clk_sel = 0;
        uint16_t    period  = 0;

        int status = pwm_get_params(cfg->pwm.freq, &clk_sel, &period);
        RETURN_ON_ERR(status);

        // Set the timer compare channel buffer for the desired duty %
        timer_pwm_set_duty(cfg, cfg->pwm.duty);

        // Set config parameters
        cfg->timer->CTRLB |= TC_WGMODE_SINGLESLOPE_gc;
        cfg->timer->CTRLB |= (TC0_CCAEN_bm << (cfg->channel));
        cfg->timer->PER = period;

        // // Enable output on port direction
        // PORTD.DIR |= (1u << cfg->channel);

        // Set clock source (starts the timer)
        cfg->timer->CTRLA |= clk_sel;
        break;
      }
      default: return ERR_BAD_PARAM;
    }
  } // ATOMIC_BLOCK(ATOMIC_RESTORESTATE)

  return 0;
}

void timer_ch_isr_enable(timer_config_t* cfg, isr_priority_e priority) {
  assert(cfg);

  const uint8_t shift  = cfg->channel << 1;
  const uint8_t mask  = (TC0_CCAINTLVL_gm) << shift;
  cfg->timer->INTCTRLB = (cfg->timer->INTCTRLB & ~mask) | (priority << shift);
}

void timer_ch_isr_disable(timer_config_t* cfg) {
  assert(cfg);
  const uint8_t shift = cfg->channel << 2;
  const uint8_t mask  = (TC0_CCAINTLVL_gm) << shift;
  cfg->timer->INTCTRLB &= ~mask;
}

void timer_ovr_isr_enable(timer_config_t* cfg, isr_priority_e priority) {
  assert(cfg);
  cfg->timer->INTCTRLA = (cfg->timer->INTCTRLA & ~TC0_OVFINTLVL_gm) | priority;
}

void timer_ovr_isr_disable(timer_config_t* cfg) {
  assert(cfg);
  cfg->timer->INTCTRLA &= ~TC0_OVFINTLVL_gm;
}

void timer_pwm_set_duty(timer_config_t* cfg, uint8_t duty) {
  assert(cfg);

  if (duty > 100) {
    duty = 100;
  }

  cfg->pwm.duty  = duty;
  uint16_t ccbuf = (cfg->timer->PER * duty) / 100;
  cc_buffer_set(cfg, ccbuf);
}

void timer_pwm_stop(timer_config_t* cfg) {
#warning todo
}

void timer_pwm_start(timer_config_t* cfg) {
#warning todo
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#define NUM_PRESCALERS (sizeof(prescalers) / sizeof(prescalers[0]))
#define MAX_PER        UINT16_MAX

static void get_parameters(unsigned int freq, TC_CLKSEL_t* clk_sel,
                           uint16_t* period) {
  static const uint16_t prescalers[]    = {1, 2, 4, 8, 64, 256, 1024};
  const uint32_t        clocks_per_tick = F_CPU / freq;
  uint32_t              lowest_error    = UINT32_MAX;
  uint32_t              per             = 0;
  uint16_t              best_per        = 0;

  uint8_t i = 0;

  for (; i < NUM_PRESCALERS; ++i) {
    per = clocks_per_tick / prescalers[i];
    per--;
    if (per > MAX_PER)
      continue;

    uint32_t error;
    error =
        abs((int32_t)clocks_per_tick - (int32_t)((per + 1) * prescalers[i]));
    if (error < lowest_error) {
      lowest_error = error;
      best_per     = per;
    }
  }

  *clk_sel = (TC_CLKSEL_t)i + 1;
  *period  = best_per;
}

static uint8_t get_bitmask(timer_peripheral_e periph) {
  switch (periph) {
    case TIMER_TCE0:
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

static register8_t* get_power_reg(timer_peripheral_e periph) {
  switch (periph) {
    case TIMER_TCC0:
    case TIMER_TCC1:
    case TIMER_TCC2: {
      return &PR.PRPC;
    }

    case TIMER_TCD0:
    case TIMER_TCD1:
    case TIMER_TCD2: {
      return &PR.PRPD;
    }

    case TIMER_TCE0: {
      return &PR.PRPE;
    }

    default: return NULL;
  }
  return NULL;
}

/**
 * @brief Determine the correct clock divisor and period settings to
 * use for a timer peripheral, based on a required PWM frequency.
 *
 * Pass two pointers to the function to store the calculated parameters.
 * Check the return code for 0 to ensure success.
 *
 * @param frequency Desired PWM switching frequency.
 * @param clk Calculated divisor.
 * @param per Calculated period
 * @return int 0 = success, anything else = failure.
 */
static int pwm_get_params(uint16_t freq, TC_CLKSEL_t* clk, uint16_t* per) {
  // assert(clk);
  // assert(per);

  // if (freq == 0) {
  //   return ERR_BAD_PARAM;
  // }

  // uint16_t period;
  // uint8_t  prescaler;

  // // calculate the best period and prescaler for the desired PWM frequency
  // for (prescaler = 1; prescaler <= 7; prescaler++) {
  //   period = (uint16_t)(F_CPU / (freq * prescaler));
  //   if (period <= 0xFFFF) {
  //     break;
  //   }
  // }

  // *per = period;
  // *clk = prescaler;

  return 0;
}

static void cc_buffer_set(timer_config_t* cfg, uint16_t val) {
  assert(cfg);

  switch (cfg->channel) {

    case TIMER_CHANNEL_A: {
      cfg->timer->CCABUF = val;
      break;
    }

    case TIMER_CHANNEL_B: {
      cfg->timer->CCBBUF = val;
      break;
    }

    case TIMER_CHANNEL_C: {
      cfg->timer->CCCBUF = val;
      break;
    }

    case TIMER_CHANNEL_D: {
      cfg->timer->CCDBUF = val;
      break;
    }

    default: return;
  }
}
