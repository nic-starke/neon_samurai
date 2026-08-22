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
#include "system/print.h"
#include "system/error.h"
#include "hal/timer.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define SYSTIME_FREQ_HZ (1000) // 1 tick == 1 millisecond

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static struct timer_config sys_timer = {
		.timer	 = &TCE0,
		.periph	 = TIMER_TCE0,
		.channel = TIMER_CHANNEL_A,
		.freq		 = SYSTIME_FREQ_HZ,
		.mode		 = TIMER_MODE_OVF,
		.pwm		 = {0},
};

static volatile u32 thetime = 0;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int systime_start(void) {
	int status = timer_init(&sys_timer);
	RETURN_ON_ERR(status);

	timer_ovr_isr_enable(&sys_timer, PRIORITY_LOW);
	return SUCCESS;
}

u32 systime_ms(void) {
	/*
		The counter is four bytes and the AVR loads one byte at a time, so a read
		can be interrupted partway through by the tick below and come back with
		bytes from either side of an increment. That is not a rounding error: a
		carry out of the low byte read at the wrong moment shifts the result by
		256 ms, and a value that appears to move backwards makes an unsigned
		elapsed-time subtraction wrap to something enormous - enough to trip a
		periodic settings write and put a needless erase cycle on the EEPROM.
	*/
	u32 now;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		now = thetime;
	}

	return now;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

ISR(TCE0_OVF_vect) {
	thetime += 1;
}
