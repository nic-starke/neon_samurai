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

#define NUM_PRESCALERS (sizeof(prescalers) / sizeof(prescalers[0]))
#define MAX_PER				 (UINT16_MAX)

// Every defined interrupt flag of a type-0 timer - bits 2 and 3 are reserved
// and must be written as zero.
#define TC_INTFLAGS_ALL                                                        \
	(TC0_OVFIF_bm | TC0_ERRIF_bm | TC0_CCAIF_bm | TC0_CCBIF_bm | TC0_CCCIF_bm |  \
	 TC0_CCDIF_bm)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static u8						 get_bitmask(enum timer_peripheral periph);
static register8_t*	 get_power_reg(enum timer_peripheral periph);
static bool					 is_type1(enum timer_peripheral periph);
static int					 cfg_validate(const struct timer_config* cfg);
static volatile u16* cc_reg(struct timer_config* cfg, bool buffered);
static u16					 duty_to_cc(u16 per, u8 duty);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Clock divisors, ordered so that index + 1 == the matching TC_CLKSEL_t.
static const u32 prescalers[] PROGMEM = {1, 2, 4, 8, 64, 256, 1024};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int timer_init(struct timer_config* cfg) {
	assert(cfg);

	int status = cfg_validate(cfg);
	RETURN_ON_ERR(status);

	u16					period	= 0;
	TC_CLKSEL_t clk_sel = TC_CLKSEL_OFF_gc;
	u8					wgmode	= TC_WGMODE_NORMAL_gc;

	// Resolve the hardware parameters before touching the peripheral, so that
	// a bad configuration leaves the timer untouched rather than half-set-up.
	switch (cfg->mode) {
		case TIMER_MODE_OVF: {
			status = timer_get_parameters(cfg->freq, &clk_sel, &period);
			wgmode = TC_WGMODE_NORMAL_gc;
			break;
		}

		case TIMER_MODE_PWM: {
			status = timer_get_parameters(cfg->pwm.freq, &clk_sel, &period);
			wgmode = TC_WGMODE_SINGLESLOPE_gc;

			if (cfg->pwm.duty > 100) {
				cfg->pwm.duty = 100;
			}
			break;
		}

		case TIMER_MODE_NB:
		default: return ERR_BAD_PARAM;
	}
	RETURN_ON_ERR(status);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		register8_t* pp = get_power_reg(cfg->periph);
		*pp &= (u8)~get_bitmask(cfg->periph); // Enable power to the peripheral

		// CTRLA holds the clock source - clearing it stops the timer so that
		// the rest of the configuration is applied atomically from the
		// peripheral's point of view.
		cfg->timer->CTRLA = TC_CLKSEL_OFF_gc;
		cfg->timer->CTRLB = wgmode;
		cfg->timer->CTRLC = 0;
		cfg->timer->CTRLD = 0;
		cfg->timer->CTRLE = 0;

		// Re-init must not inherit interrupt enables or stale pending flags.
		cfg->timer->INTCTRLA = 0;
		cfg->timer->INTCTRLB = 0;
		cfg->timer->INTFLAGS = TC_INTFLAGS_ALL;

		cfg->timer->CNT = 0;
		cfg->timer->PER = period;

		if (cfg->mode == TIMER_MODE_PWM) {
			cfg->timer->CTRLB |= (u8)(TC0_CCAEN_bm << (u8)cfg->channel);

			// Write the compare register directly - a buffered write only
			// transfers on an update event, which cannot occur while stopped.
			*cc_reg(cfg, false) = duty_to_cc(period, cfg->pwm.duty);
		}

		cfg->clksel				= clk_sel;
		cfg->timer->CTRLA = (u8)clk_sel; // Starts the timer
	}

	return SUCCESS;
}

u16 timer_getval(struct timer_config* cfg) {
	assert(cfg);

	u16 cnt;

	// 16-bit register access goes via the peripheral's shared TEMP register,
	// so it must not be interleaved with an ISR touching the same timer.
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		cnt = cfg->timer->CNT;
	}

	return cnt;
}

void timer_ch_isr_enable(struct timer_config* cfg, enum isr_priority priority) {
	assert(cfg);
	assert(cfg->channel < TIMER_CHANNEL_NB);

	const u8 shift = (u8)((u8)cfg->channel << 1u);
	const u8 mask	 = (u8)(TC0_CCAINTLVL_gm << shift);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// Discard a stale flag, otherwise the ISR fires as soon as it is enabled.
		cfg->timer->INTFLAGS = (u8)(TC0_CCAIF_bm << (u8)cfg->channel);
		cfg->timer->INTCTRLB =
				(u8)((cfg->timer->INTCTRLB & (u8)~mask) | (u8)((u8)priority << shift));
	}
}

void timer_ch_isr_disable(struct timer_config* cfg) {
	assert(cfg);
	assert(cfg->channel < TIMER_CHANNEL_NB);

	const u8 shift = (u8)((u8)cfg->channel << 1u);
	const u8 mask	 = (u8)(TC0_CCAINTLVL_gm << shift);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		cfg->timer->INTCTRLB &= (u8)~mask;
	}
}

void timer_ovr_isr_enable(struct timer_config* cfg,
													enum isr_priority		 priority) {
	assert(cfg);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		cfg->timer->INTFLAGS = TC0_OVFIF_bm;
		cfg->timer->INTCTRLA =
				(u8)((cfg->timer->INTCTRLA & (u8)~TC0_OVFINTLVL_gm) | (u8)priority);
	}
}

void timer_ovr_isr_disable(struct timer_config* cfg) {
	assert(cfg);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		cfg->timer->INTCTRLA &= (u8)~TC0_OVFINTLVL_gm;
	}
}

void timer_pwm_set_duty(struct timer_config* cfg, u8 duty) {
	assert(cfg);

	if (duty > 100) {
		duty = 100;
	}

	cfg->pwm.duty = duty;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// Buffered write - the new compare value is latched on the next update
		// event, so the cycle in flight is not truncated.
		*cc_reg(cfg, true) = duty_to_cc(cfg->timer->PER, duty);
	}
}

void timer_pwm_stop(struct timer_config* cfg) {
	assert(cfg);
	cfg->timer->CTRLA = TC_CLKSEL_OFF_gc;
}

void timer_pwm_start(struct timer_config* cfg) {
	assert(cfg);
	cfg->timer->CTRLA = (u8)cfg->clksel;
}

int timer_get_parameters(u32 freq, TC_CLKSEL_t* clk_sel, u16* period) {
	assert(clk_sel);
	assert(period);

	if ((freq == 0) || (freq > (u32)F_CPU)) {
		return ERR_BAD_PARAM;
	}

	const u32 clocks_per_tick = (u32)F_CPU / freq;
	u32				lowest_error		= UINT32_MAX;
	u16				best_per				= 0;
	u8				best_idx				= 0;
	bool			found						= false;

	for (u8 i = 0; i < NUM_PRESCALERS; ++i) {
		const u32 prescaler = pgm_read_dword(&prescalers[i]);
		const u32 ticks			= clocks_per_tick / prescaler;

		// The counter runs 0..PER inclusive, so an n-clock period needs
		// PER = n - 1. Skip divisors that cannot represent the period.
		if ((ticks == 0) || ((ticks - 1u) > MAX_PER)) {
			continue;
		}

		const u32 actual = ticks * prescaler;
		const u32 error	 = (actual > clocks_per_tick) ? (actual - clocks_per_tick)
																									: (clocks_per_tick - actual);

		if (error < lowest_error) {
			lowest_error = error;
			best_per		 = (u16)(ticks - 1u);
			best_idx		 = i;
			found				 = true;
		}
	}

	if (!found) {
		return ERR_BAD_PARAM;
	}

	*clk_sel = (TC_CLKSEL_t)(best_idx + 1u);
	*period	 = best_per;

	return SUCCESS;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

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
}

static bool is_type1(enum timer_peripheral periph) {
	return (periph == TIMER_TCC1) || (periph == TIMER_TCD1);
}

/**
 * @brief Reject a configuration the hardware cannot honour.
 *
 * Runs before any register is touched - the checks are runtime (not assert)
 * because asserts compile out of release builds, and a bad peripheral would
 * otherwise dereference a NULL power-reduction register.
 *
 * @param cfg Configuration to check.
 * @return int SUCCESS, or ERR_BAD_PARAM.
 */
static int cfg_validate(const struct timer_config* cfg) {
	if (cfg->timer == NULL) {
		return ERR_BAD_PARAM;
	}

	if ((cfg->periph >= TIMER_NB) || (cfg->channel >= TIMER_CHANNEL_NB)) {
		return ERR_BAD_PARAM;
	}

	if (get_power_reg(cfg->periph) == NULL) {
		return ERR_BAD_PARAM;
	}

	// The type-1 timers (TCC1/TCD1) only implement compare channels A and B.
	if (is_type1(cfg->periph) && (cfg->channel > TIMER_CHANNEL_B)) {
		return ERR_BAD_PARAM;
	}

	return SUCCESS;
}

static volatile u16* cc_reg(struct timer_config* cfg, bool buffered) {
	assert(cfg);

	switch (cfg->channel) {
		case TIMER_CHANNEL_A: {
			return buffered ? &cfg->timer->CCABUF : &cfg->timer->CCA;
		}

		case TIMER_CHANNEL_B: {
			return buffered ? &cfg->timer->CCBBUF : &cfg->timer->CCB;
		}

		case TIMER_CHANNEL_C: {
			return buffered ? &cfg->timer->CCCBUF : &cfg->timer->CCC;
		}

		case TIMER_CHANNEL_D: {
			return buffered ? &cfg->timer->CCDBUF : &cfg->timer->CCD;
		}

		default: return buffered ? &cfg->timer->CCABUF : &cfg->timer->CCA;
	}
}

/**
 * @brief Convert a duty cycle percentage into a single-slope compare value.
 *
 * The output is asserted while CNT < CCx, so a full-scale duty needs a
 * compare value above TOP.
 *
 * @param per Timer period (TOP).
 * @param duty Duty cycle percentage, 0 to 100.
 * @return u16 Value to write to the compare (or compare buffer) register.
 */
static u16 duty_to_cc(u16 per, u8 duty) {
	if (duty >= 100) {
		return (per < MAX_PER) ? (u16)(per + 1u) : MAX_PER;
	}

	return (u16)((((u32)per + 1u) * duty) / 100u);
}
