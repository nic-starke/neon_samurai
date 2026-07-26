/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/interrupt.h>
#include <util/atomic.h>

#include "system/time.h"
#include "hal/timer.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define DIV_ROUND(a, b) (((a) + (b) / 2) / (b))

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// static struct timer_config sys_timer = {
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
	/*
		thetime is 32-bit and this is an 8-bit machine, so the read compiles to
		four byte loads. TCE0_OVF_vect fires at 1 kHz and can land between any two
		of them, producing a torn value - e.g. 0x00FF read as 0x01FF just after a
		carry.

		This matters more than it looks: this is the timebase for the display
		throttle, animation frames, encoder acceleration, the config autosave and
		the 200 ms bootloader-entry window in main().
	*/
	u32 t;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		t = thetime;
	}

	return t;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

ISR(TCE0_OVF_vect) {
	thetime += 1;
}
