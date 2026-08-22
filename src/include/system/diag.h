/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*
	Counters for faults the device can carry on through.

	A failure that stops the firmware being able to do its job calls
	hal_panic(), which lights every LED red and stops. Everything else is a
	fault the next iteration of the main loop can retry: an event queue that was
	full for one pass, a display update that did not go out. Those are worth
	knowing about but not worth stopping for, and there is no display to report
	them on, so they are counted here instead.

	Counters saturate rather than wrap. A count that has stuck at its maximum
	still says "this keeps happening", which is the only thing the number is
	used for, whereas a wrapped count can read as zero while the fault is
	ongoing.

	Reading a counter is a plain load of a value only ever written by main-loop
	code, so no interrupt masking is needed on either side.
*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"
#include "system/error.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Run a call, and count it if it reports a failure.
 *
 * For faults the caller has no better answer to than carrying on: the work is
 * lost, the next pass of the main loop will try again, and the count is what
 * says how often that happened.
 *
 * @param call Call returning an error code. Evaluated once.
 * @param counter Which fault to count on failure.
 */
#define DIAG_ON_ERR(call, counter)                                             \
	do {                                                                         \
		if ((call) != SUCCESS) {                                                   \
			diag_count(counter);                                                     \
		}                                                                          \
	} while (0)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum diag_counter {
	// An event could not be queued because the channel was full.
	DIAG_EVENT_DROPPED,

	// An encoder's LEDs could not be redrawn.
	DIAG_DISPLAY_FAILED,

	// A MIDI message could not be handed to the USB stack.
	DIAG_MIDI_TX_FAILED,

	// Settings could not be written to EEPROM.
	DIAG_CFG_STORE_FAILED,

	DIAG_NB,
};

_Static_assert(DIAG_NB <= UINT8_MAX, "the counters are walked with a u8 index");

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Record one occurrence of a fault.
 * @param counter Which fault occurred.
 */
void diag_count(enum diag_counter counter);

/**
 * @brief Read how many times a fault has been recorded since the last reset.
 * @param counter Which fault to read.
 * @return u16 The count, saturated at UINT16_MAX.
 */
u16 diag_get(enum diag_counter counter);

/**
 * @brief Return the total across every counter.
 * @return u32 Zero if the device has had a clean run.
 */
u32 diag_total(void);

/**
 * @brief Set every counter back to zero.
 */
void diag_reset(void);
