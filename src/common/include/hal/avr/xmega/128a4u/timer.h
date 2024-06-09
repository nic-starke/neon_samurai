/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>

#include "sys/types.h"

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
	u8	duty; // Desired duty cycle percentage (0 to 100)
} pwm_config_s;

typedef struct {
	volatile TC0_t*		 timer;
	timer_peripheral_e periph;
	timer_channel_e		 channel;
	u32								 freq; // Timer frequency (Hz)

	timer_mode_e mode;
	union {
		pwm_config_s pwm;
	};

} timer_config_s;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int timer_init(timer_config_s* cfg);
u16 timer_getval(timer_config_s* cfg);

void timer_ch_isr_enable(timer_config_s* cfg, isr_priority_e priority);
void timer_ch_isr_disable(timer_config_s* cfg);
void timer_ovr_isr_enable(timer_config_s* cfg, isr_priority_e priority);
void timer_ovr_isr_disable(timer_config_s* cfg);

void timer_pwm_start(timer_config_s* cfg);
void timer_pwm_stop(timer_config_s* cfg);
void timer_pwm_set_duty(timer_config_s* cfg, u8 duty);
void timer_get_parameters(unsigned int freq, TC_CLKSEL_t* clk_sel, u16* period);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
