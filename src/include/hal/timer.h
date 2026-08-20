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
	volatile TC0_t*				timer;
	enum timer_peripheral periph;
	enum timer_channel		channel;
	u32										freq; // Timer frequency (Hz) - overflow mode only

	enum timer_mode mode;
	union {
		struct pwm_config pwm;
	};

	// Private - assigned by timer_init, used to restart the timer.
	TC_CLKSEL_t clksel;
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Configure and start a timer peripheral.
 *
 * The timer is stopped and fully reset before the new configuration is
 * applied, so an already-running timer can be safely re-initialised. Any
 * interrupt previously enabled on the timer is disabled - re-enable it with
 * timer_ch_isr_enable/timer_ovr_isr_enable after this returns.
 *
 * In PWM mode the caller is responsible for configuring the output pin
 * direction (and any PORT remap) - the driver does not touch GPIO.
 *
 * @param cfg Timer configuration.
 * @return int SUCCESS, or ERR_BAD_PARAM if the configuration is invalid.
 */
int timer_init(struct timer_config* cfg);

/**
 * @brief Read the current value of the timer counter.
 *
 * The read is performed atomically - 16-bit register access goes through the
 * peripheral's shared TEMP register, so it must not be interrupted by an ISR
 * that touches the same timer.
 *
 * @param cfg Timer configuration.
 * @return u16 Counter value, 0 to the configured period.
 */
u16 timer_getval(struct timer_config* cfg);

/**
 * @brief Enable the compare/capture interrupt for the configured channel.
 *
 * Any pending flag for the channel is discarded first, so the handler does
 * not fire immediately on a match that occurred before this call. The caller
 * must supply the matching ISR (e.g. ISR(TCC0_CCA_vect)), enable the
 * interrupt level in the PMIC, and call sei().
 *
 * @param cfg Timer configuration.
 * @param priority Interrupt level, or PRIORITY_OFF to leave it disabled.
 */
void timer_ch_isr_enable(struct timer_config* cfg, enum isr_priority priority);

/**
 * @brief Disable the compare/capture interrupt for the configured channel.
 *
 * The other channels of the same timer are left untouched.
 *
 * @param cfg Timer configuration.
 */
void timer_ch_isr_disable(struct timer_config* cfg);

/**
 * @brief Enable the timer overflow interrupt.
 *
 * Any pending overflow flag is discarded first. The caller must supply the
 * matching ISR (e.g. ISR(TCE0_OVF_vect)), enable the interrupt level in the
 * PMIC, and call sei().
 *
 * @param cfg Timer configuration.
 * @param priority Interrupt level, or PRIORITY_OFF to leave it disabled.
 */
void timer_ovr_isr_enable(struct timer_config* cfg, enum isr_priority priority);

/**
 * @brief Disable the timer overflow interrupt.
 *
 * @param cfg Timer configuration.
 */
void timer_ovr_isr_disable(struct timer_config* cfg);

/**
 * @brief Restart a timer stopped by timer_pwm_stop.
 *
 * Does nothing unless the timer has been configured by timer_init.
 *
 * @param cfg Timer configuration.
 */
void timer_pwm_start(struct timer_config* cfg);

/**
 * @brief Stop the timer by disconnecting its clock source.
 *
 * The count, period and compare values are retained.
 *
 * @param cfg Timer configuration.
 */
void timer_pwm_stop(struct timer_config* cfg);

/**
 * @brief Set the PWM duty cycle of the configured channel.
 *
 * Requires the timer to have been initialised in TIMER_MODE_PWM. The write is
 * buffered, so the new value is latched at the next update event rather than
 * truncating the cycle currently in flight. Values above 100 are clamped, and
 * the clamped value is written back to cfg->pwm.duty.
 *
 * @param cfg Timer configuration.
 * @param duty Duty cycle percentage, 0 to 100.
 */
void timer_pwm_set_duty(struct timer_config* cfg, u8 duty);

/**
 * @brief Find the closest clock divisor and period for a target frequency.
 *
 * @param freq Desired timer frequency (Hz).
 * @param clk_sel Calculated clock select.
 * @param period Calculated period (TOP).
 * @return int SUCCESS, or ERR_BAD_PARAM if freq is unreachable.
 */
int timer_get_parameters(u32 freq, TC_CLKSEL_t* clk_sel, u16* period);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
