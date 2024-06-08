/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/interrupt.h>

#include "sys/time.h"
#include "sys/print.h"
#include "hal/avr/xmega/128a4u/timer.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define DIV_ROUND(a, b) (((a) + (b) / 2) / (b))

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// static timer_config_s sys_timer = {
// 	.timer = &TCE0,
// 	.periph = TIMER_TCE0,
// 	.channel = TIMER_CHANNEL_A,
// 	.freq = 1000,
// 	.mode = TIMER_MODE_OVF,
// 	.pwm = {0},
// };

static volatile u32 thetime = 0;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void systime_start(void) {
	TCE0.PER			= DIV_ROUND(F_CPU, 1000);
	TCE0.CTRLB		= TC_WGMODE_NORMAL_gc;
	TCE0.INTCTRLA = TC_OVFINTLVL_LO_gc;
	TCE0.CNT			= 0;
	TCE0.CTRLA		= TC_CLKSEL_DIV1_gc;
}

u32 systime_ms(void) {
	return thetime;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

ISR(TCE0_OVF_vect) {
	thetime += 1;
}
