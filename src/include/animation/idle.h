/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*
	An idle display: a slow swell of light that travels diagonally across the
	panel, in shades of sea green.

	It runs when nothing is connected that could be using the device and
	nothing has been touched for IDLE_TIMEOUT_MS. Any activity stops it
	immediately and the normal display is redrawn, so it can never be seen
	sitting on top of something the user is doing.

	The wave is a single travelling swell rather than sixteen independent
	blinks. Each encoder runs the same brightness envelope, started at a
	different point: later columns lag by IDLE_COL_DELAY_STEPS and higher rows
	by IDLE_ROW_DELAY_STEPS. The row delay is deliberately the shorter of the
	two, so a row begins just before the next encoder along the row below it
	does, which is what tilts the wavefront into a diagonal instead of letting
	it march along one row at a time.

	The brightness envelope never reaches zero. The encoders dim between
	swells rather than going out, which reads as water moving rather than as
	something flashing.
*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
	How long the device must be left alone before the display takes over.

	There are two bars, because how much is known differs.

	With nothing configured - a charger, a dead hub, a host that has not
	enumerated it, or one that has gone to sleep - nothing can possibly be
	using the device, and a short wait is safe.

	With a host attached the device is much less certain. USB MIDI has no way
	to say that an application has opened the port: unlike a serial port, which
	raises DTR, the operating system claims the MIDI interface when it
	enumerates and never tells the device what happens above that. Traffic is
	the only evidence there is, and absence of traffic is weak evidence - a
	mapping that sends no feedback looks exactly like nothing being there. So
	the bar is set far higher before taking the panel from something that might
	still want it.
*/
#define IDLE_TIMEOUT_MS			 (10000u)
#define IDLE_HOST_QUIET_MS	 (120000u)

// Milliseconds per step of the wave.
#define IDLE_STEP_MS				 (40u)

// Steps in one complete swell. A power of two so the phase wraps with a mask.
#define IDLE_WAVE_STEPS			 (64u)

// The wave's own view of the panel. Kept here rather than taken from
// system/hardware.h so the wave maths can be built and tested on a host,
// where the peripheral headers do not exist. idle.c asserts the two agree.
#define IDLE_GRID_COLS			 (4u)
#define IDLE_GRID_ROWS			 (4u)
#define IDLE_NUM_ENCODERS		 (IDLE_GRID_COLS * IDLE_GRID_ROWS)

// Bit planes per encoder, and LEDs per encoder - the width of a plane word.
#define IDLE_BCM_PLANES			 (8u)
#define IDLE_LEDS_PER_ENC		 (16u)

/*
	Start delay between adjacent encoders along a row, and between rows.

	The row delay is the shorter of the two - see the note above about the
	wavefront being diagonal.
*/
#define IDLE_COL_DELAY_STEPS (6u)
#define IDLE_ROW_DELAY_STEPS (5u)

/*
	The panel does not cut straight to the wave. Whatever was on the LEDs is
	faded down to black first, and the wave then fades up out of it.

	The fade out is a decay applied to the frame buffer rather than a ramp
	against a saved copy - keeping a copy would cost 256 bytes of SRAM the
	device has not got, and a decay reads more naturally anyway. Each step
	scales what is there by IDLE_FADE_OUT_NUM/IDLE_FADE_OUT_DEN.

	Coming back the other way is deliberately not faded. Touching a control
	should show its real position immediately - a fade there would be a
	quarter-second of the device appearing not to respond.
*/
#define IDLE_FADE_OUT_STEPS	 (16u)
#define IDLE_FADE_IN_STEPS	 (16u)

// Roughly 0.8 per step, so sixteen steps land near enough black.
#define IDLE_FADE_OUT_NUM		 (205u)
#define IDLE_FADE_OUT_DEN		 (256u)

/*
	The envelope runs between these, rather than down to black.

	A low floor is a deep swell: the encoders drop to a couple of percent of
	full before rising again. It also puts the bottom of the swell into the
	part of the gamma curve where one output level is a large step in apparent
	brightness - at four out of 255, a single level is a quarter as much light
	again - so the dimmest part of the fade steps rather than glides. The
	correction in led/color.c is set to 2.0 rather than 2.2 partly to lift that
	region, which takes the worst of it off.

	Raising this flattens the swell but smooths it. Eighty puts the dimmest
	point at a tenth of full and roughly halves the largest step.
*/
#define IDLE_LEVEL_FLOOR		 (40u)
#define IDLE_LEVEL_PEAK			 (255u)

/*
	Sea green, spread so no two encoders share a hue.

	The wave leads at the last encoder, and that is the one the palette is
	anchored to - it holds the base shade and the rest runs back from it.
	Hue is 0-1535 for a full turn, so this covers roughly 131 to 170 degrees:
	green-teal through to blue-green.
*/
#define IDLE_HUE_BASE				 (560u)
#define IDLE_HUE_STEP				 (11u)
#define IDLE_SATURATION			 (200u)

#define IDLE_HUE_WHEEL			 (1536u)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Note that the device is being used, deferring or stopping the idle
 * display.
 *
 * Called from the input handlers. Cheap enough to call on every event - it
 * only stores a timestamp.
 */
void idle_notify_activity(void);

/**
 * @brief Advance the idle display, starting or stopping it as required.
 *
 * Called once per display update. Does nothing but compare timestamps until
 * the device has actually been idle.
 */
void idle_update(void);

/**
 * @brief Whether the idle display currently owns the LEDs.
 *
 * While it does, the normal per-encoder drawing is skipped - the idle display
 * writes every LED itself.
 */
bool idle_is_active(void);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Wave shape ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*
	Separated from the drawing so the shape of the wave can be tested on a
	host, where the frame buffer and the timer do not exist.
*/

/**
 * @brief How many steps into the wave an encoder starts.
 * @param encoder_idx 0-15.
 * @return u8 Delay in wave steps. The last encoder leads, with 0.
 */
u8 idle_wave_offset(u8 encoder_idx);

/**
 * @brief An encoder's brightness at a point in the wave.
 * @param encoder_idx 0-15.
 * @param step The wave step counter, free-running.
 * @return u8 Brightness between IDLE_LEVEL_FLOOR and IDLE_LEVEL_PEAK.
 */
u8 idle_wave_level(u8 encoder_idx, u16 step);

/**
 * @brief An encoder's hue within the palette.
 * @param encoder_idx 0-15.
 * @return u16 Hue, 0-1535.
 */
u16 idle_wave_hue(u8 encoder_idx);

/**
 * @brief Scale one encoder's LEDs down by a single step of the fade.
 *
 * Brightness is held as bit planes - an LED's level is its bit taken from
 * each plane in turn - so dimming means pulling every level out of the
 * planes, scaling it, and putting it back. Done in place, so repeating it
 * decays the panel without a copy of the original screen being kept
 * anywhere.
 *
 * @param planes The encoder's plane words, active-low, modified in place.
 */
void idle_dim_planes(u16 planes[IDLE_BCM_PLANES]);
