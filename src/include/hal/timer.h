/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>

#include "system/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum timer_peripheral {
	TIMER_TCC0,
	TIMER_TCC1,
	TIMER_TCC2,
	TIMER_TCD0,
	TIMER_TCD1,
	TIMER_TCD2,
	TIMER_TCE0,

	TIMER_NB,
};

enum timer_channel {
	TIMER_CHANNEL_A, // PWM on pin 0
	TIMER_CHANNEL_B, // PWM on pin 1
	TIMER_CHANNEL_C, // PWM on pin 2
	TIMER_CHANNEL_D, // PWM on pin 3

	TIMER_CHANNEL_NB,
};

enum timer_mode {
	TIMER_MODE_OVF,
	TIMER_MODE_PWM,

	TIMER_MODE_NB,
};

struct pwm_config {
	u16 freq; // Desired PWM frequency
	u8	duty; // Desired duty cycle percentage (0 to 100)
};

struct timer_config {
	volatile TC0_t*		 timer;
	enum timer_peripheral periph;
	enum timer_channel		 channel;
	u32								 freq; // Timer frequency (Hz)

	enum timer_mode mode;
	union {
		struct pwm_config pwm;
	};

};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int timer_init(struct timer_config* cfg);
u16 timer_getval(struct timer_config* cfg);

void timer_ch_isr_enable(struct timer_config* cfg, enum isr_priority priority);
void timer_ch_isr_disable(struct timer_config* cfg);
void timer_ovr_isr_enable(struct timer_config* cfg, enum isr_priority priority);
void timer_ovr_isr_disable(struct timer_config* cfg);

void timer_pwm_start(struct timer_config* cfg);
void timer_pwm_stop(struct timer_config* cfg);
void timer_pwm_set_duty(struct timer_config* cfg, u8 duty);
void timer_get_parameters(unsigned int freq, TC_CLKSEL_t* clk_sel, u16* period);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
