/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"
#include "system/time.h"
#include "system/hardware.h"
#include "animation/idle.h"
#include "event/animation.h"
#include "led/led.h"
#include "led/color.h"
#include "led/rgb.h"
#include "usb/usb.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// idle_wave.c works from the grid in idle.h so it can be tested on a host.
// This is where that grid meets the real panel.
_Static_assert(IDLE_NUM_ENCODERS == NUM_ENCODERS,
							 "the wave grid does not match the number of encoders");

_Static_assert(IDLE_BCM_PLANES == NUM_BCM_PLANES,
							 "the fade helper works on a different number of planes");

_Static_assert(IDLE_LEDS_PER_ENC == NUM_LEDS_PER_ENCODER,
							 "the fade helper works on a different number of LEDs");

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum idle_phase {
	// Not running - the normal display owns the LEDs.
	PHASE_OFF,

	// Dimming whatever was on the panel down to black.
	PHASE_FADE_OUT,

	// Bringing the wave up out of black.
	PHASE_FADE_IN,

	// The wave, at full brightness.
	PHASE_RUN,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static u32						 last_activity_ms = 0;
static u32						 last_step_ms			= 0;
static u16						 wave_step				= 0;
static u8							 phase_step				= 0;
static enum idle_phase phase						= PHASE_OFF;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void draw(u8 fade);
static void dim_panel(void);
static void blank_panel(void);
static void release(void);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void idle_notify_activity(void) {
	last_activity_ms = systime_ms();

	if (phase != PHASE_OFF) {
		release();
	}
}

bool idle_is_active(void) {
	return phase != PHASE_OFF;
}

void idle_update(void) {
	const u32 now = systime_ms();

	/*
		A host that has configured the interface is a host that could be talking
		to the device at any moment, whether or not it happens to be sending
		anything this second. Only take the panel when nothing is in a position
		to use it.
	*/
	if (usb_is_configured()) {
		idle_notify_activity();
		return;
	}

	if ((now - last_activity_ms) < IDLE_TIMEOUT_MS) {
		return;
	}

	if (phase == PHASE_OFF) {
		/*
			A bank change or reset flourish still has the LEDs. In practice it
			cannot be running - starting one is activity, which resets the timer -
			but the idle display overwrites every LED, so it must not be the thing
			that finds out otherwise.
		*/
		if (animation_is_active()) {
			return;
		}

		phase				 = PHASE_FADE_OUT;
		phase_step	 = 0;
		wave_step		 = 0;
		last_step_ms = now;
		return;
	}

	// Advance on the wave's own clock rather than once per main loop, which
	// runs far faster and would race through both the fades and the envelope.
	if ((now - last_step_ms) < IDLE_STEP_MS) {
		return;
	}

	last_step_ms = now;

	switch (phase) {
		case PHASE_FADE_OUT: {
			dim_panel();
			phase_step++;

			if (phase_step >= IDLE_FADE_OUT_STEPS) {
				// The decay leaves a percent or two behind. Clearing it outright
				// costs nothing and means the wave rises out of black rather than
				// out of a residue of the previous screen.
				blank_panel();
				phase			 = PHASE_FADE_IN;
				phase_step = 0;
			}
			break;
		}

		case PHASE_FADE_IN: {
			phase_step++;

			// The wave is already moving as it appears, rather than fading up as
			// a still image and only then starting.
			draw((u8)(((u16)phase_step * MAX_BRIGHTNESS) / IDLE_FADE_IN_STEPS));
			wave_step++;

			if (phase_step >= IDLE_FADE_IN_STEPS) {
				phase = PHASE_RUN;
			}
			break;
		}

		case PHASE_RUN: {
			draw(MAX_BRIGHTNESS);
			wave_step++;
			break;
		}

		case PHASE_OFF:
		default: break;
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void draw(u8 fade) {
	for (u8 e = 0; e < NUM_ENCODERS; e++) {
		u8 level = idle_wave_level(e, wave_step);

		if (fade != MAX_BRIGHTNESS) {
			level = (u8)(((u16)level * fade) / MAX_BRIGHTNESS);
		}

		struct rgb_8 rgb;
		color_hsv_to_rgb(idle_wave_hue(e), IDLE_SATURATION, level, &rgb);

		u8 r = rgb.red;
		u8 g = rgb.green;
		u8 b = rgb.blue;

		/*
			Every other LED is left off. The word is built from nothing rather
			than merged into what was there, so the indicator ring and the detent
			lights go out without needing to be cleared separately.
		*/
		for (u8 p = 0; p < NUM_BCM_PLANES; p++) {
			u16 state = 0;

			if (r & 1u) {
				state |= (1u << RGB_RED_BIT);
			}
			if (g & 1u) {
				state |= (1u << RGB_GREEN_BIT);
			}
			if (b & 1u) {
				state |= (1u << RGB_BLUE_BIT);
			}

			// The frame buffer is active-low - a set bit is an LED that is off.
			gFRAME_BUFFER[p][e] = (u16)~state;

			r >>= 1;
			g >>= 1;
			b >>= 1;
		}
	}
}

static void dim_panel(void) {
	for (u8 e = 0; e < NUM_ENCODERS; e++) {
		u16 planes[IDLE_BCM_PLANES];

		// An encoder's planes are a column of the frame buffer rather than a
		// run of it, so they are gathered before the fade and put back after.
		for (u8 p = 0; p < IDLE_BCM_PLANES; p++) {
			planes[p] = gFRAME_BUFFER[p][e];
		}

		idle_dim_planes(planes);

		for (u8 p = 0; p < IDLE_BCM_PLANES; p++) {
			gFRAME_BUFFER[p][e] = planes[p];
		}
	}
}

static void blank_panel(void) {
	for (u8 p = 0; p < NUM_BCM_PLANES; p++) {
		for (u8 e = 0; e < NUM_ENCODERS; e++) {
			gFRAME_BUFFER[p][e] = 0xFFFF;
		}
	}
}

static void release(void) {
	phase = PHASE_OFF;

	// Every LED was overwritten, so the whole panel has to be drawn again
	// rather than only the encoders that changed while it was idle.
	for (u8 e = 0; e < NUM_ENCODERS; e++) {
		gENCODERS[gRT.curr_bank][e].update_display = 1;
	}
}
