/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>

#include "hal/timer.h"

#include "system/types.h"
#include "system/error.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static u8						get_bitmask(enum timer_peripheral periph);
static register8_t* get_power_reg(enum timer_peripheral periph);

static int pwm_get_params(u16 freq, TC_CLKSEL_t* clk, u16* per);

static void cc_buffer_set(struct timer_config* cfg, u16 val);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int timer_init(struct timer_config* cfg) {
	assert(cfg);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		register8_t* pp = get_power_reg(cfg->periph);
		*pp &= ~get_bitmask(cfg->periph); // Enable power to timer peripheral

		switch (cfg->mode) {

			case TIMER_MODE_OVF: {
				// Set waveform generation mode
				cfg->timer->CTRLB |= TC_WGMODE_NORMAL_gc;

				// Configure timer period and divisor
				u16					period;
				TC_CLKSEL_t clk_sel;

				timer_get_parameters((unsigned int)cfg->freq, &clk_sel, &period);
				cfg->timer->CTRLA |= (u8)clk_sel;
				cfg->timer->PER = period;
				break;
			}

			case TIMER_MODE_PWM: {
				// Configure timer period and divisor
				TC_CLKSEL_t clk_sel = 0;
				u16					period	= 0;

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
				cfg->timer->CTRLA |= (u8)clk_sel;
				break;
			}
			case TIMER_MODE_NB:
			default: return ERR_BAD_PARAM;
		}
	} // ATOMIC_BLOCK(ATOMIC_RESTORESTATE)

	return 0;
}

u16 timer_getval(struct timer_config* cfg) {
	return cfg->timer->CNT;
}

void timer_ch_isr_enable(struct timer_config* cfg, enum isr_priority priority) {
	assert(cfg);

	const u8 shift = (u8)cfg->channel << 1;
	const u8 mask	 = (TC0_CCAINTLVL_gm) << shift;
	cfg->timer->INTCTRLB =
			(u8)(cfg->timer->INTCTRLB & ~mask) | (u8)(priority << shift);
}

void timer_ch_isr_disable(struct timer_config* cfg) {
	assert(cfg);
	const u8 shift = (u8)cfg->channel << 2;
	const u8 mask	 = (TC0_CCAINTLVL_gm) << shift;
	cfg->timer->INTCTRLB &= ~mask;
}

void timer_ovr_isr_enable(struct timer_config* cfg,
													enum isr_priority		 priority) {
	assert(cfg);
	cfg->timer->INTCTRLA =
			(cfg->timer->INTCTRLA & (u8)~TC0_OVFINTLVL_gm) | (u8)priority;
}

void timer_ovr_isr_disable(struct timer_config* cfg) {
	assert(cfg);
	cfg->timer->INTCTRLA &= (u8)~TC0_OVFINTLVL_gm;
}

void timer_pwm_set_duty(struct timer_config* cfg, u8 duty) {
	assert(cfg);

	if (duty > 100) {
		duty = 100;
	}

	cfg->pwm.duty = duty;
	u16 ccbuf			= (cfg->timer->PER * duty) / 100;
	cc_buffer_set(cfg, ccbuf);
}

#warning "TODO"
void timer_pwm_stop(struct timer_config* cfg) {
	(void)cfg;
}

#warning "TODO"
void timer_pwm_start(struct timer_config* cfg) {
	(void)cfg;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#define NUM_PRESCALERS (sizeof(prescalers) / sizeof(prescalers[0]))
#define MAX_PER				 UINT16_MAX

#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
void timer_get_parameters(unsigned int freq, TC_CLKSEL_t* clk_sel,
													u16* period) {
	PROGMEM static const u32 prescalers[]		 = {1, 2, 4, 8, 64, 256, 1024};
	const u32								 clocks_per_tick = F_CPU / freq;
	u32											 lowest_error		 = UINT32_MAX;
	u32											 per						 = 0;
	u16											 best_per				 = 0;

	u8 i = 0;

	for (; i < NUM_PRESCALERS; ++i) {
		per = clocks_per_tick / prescalers[i];
		per--;
		if (per > MAX_PER)
			continue;

		u32 error;

		error = abs(clocks_per_tick - ((per + 1u) * prescalers[i]));
		if (error < lowest_error) {
			lowest_error = error;
			best_per		 = per;
		}
	}

	*clk_sel = (TC_CLKSEL_t)i + 1;
	*period	 = best_per;
}
#pragma GCC diagnostic pop

static u8 get_bitmask(enum timer_peripheral periph) {
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

static register8_t* get_power_reg(enum timer_peripheral periph) {
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
static int pwm_get_params(u16 freq, TC_CLKSEL_t* clk, u16* per) {
	assert(clk);
	assert(per);

	if (freq == 0) {
		return ERR_BAD_PARAM;
	}

	u16 period;
	u8	prescaler;

	// calculate the best period and prescaler for the desired PWM frequency
	for (prescaler = 1; prescaler <= 7; prescaler++) {
		period = (u16)(F_CPU / (freq * prescaler));
		if (period <= 0xFFFF) {
			break;
		}
	}

	*per = period;
	*clk = prescaler;

	return 0;
}

static void cc_buffer_set(struct timer_config* cfg, u16 val) {
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
