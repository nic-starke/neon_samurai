/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* Further optimized re-implementation of mf_draw_encoder using LUTs. */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <math.h>
#include <float.h>
#include <assert.h>

#include "io/encoder.h"
#include "led/led.h"
#include "system/config.h"
#include "system/error.h"
#include "system/hardware.h"
#include "system/time.h"
#include "system/utility.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Ensure these match the hardware bit layout assumed by encoder_led_s
#define RGB_RED_BIT				(3)
#define RGB_GREEN_BIT			(4)
#define RGB_BLUE_BIT			(2)
#define DETENT_RED_BIT		(1)
#define DETENT_BLUE_BIT		(0)

#define INDICATOR_MASK(n) (0x8000 >> ((n) - 1)) // Mask for indicator n (1-11)
#define CENTER_INDICATOR	(6)
#define CENTER_INDICATOR_MASK                                                  \
	INDICATOR_MASK(CENTER_INDICATOR) // Explicit mask for center

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Original union structure (used implicitly for bit positions)
typedef union {
	struct {
		u16 detent_blue	 : 1; // Bit 0
		u16 detent_red	 : 1; // Bit 1
		u16 rgb_blue		 : 1; // Bit 2
		u16 rgb_red			 : 1; // Bit 3
		u16 rgb_green		 : 1; // Bit 4
		u16 indicator_11 : 1; // Bit 5
		u16 indicator_10 : 1; // Bit 6
		u16 indicator_9	 : 1; // Bit 7
		u16 indicator_8	 : 1; // Bit 8
		u16 indicator_7	 : 1; // Bit 9
		u16 indicator_6	 : 1; // Bit 10 - Center Detent Indicator
		u16 indicator_5	 : 1; // Bit 11
		u16 indicator_4	 : 1; // Bit 12
		u16 indicator_3	 : 1; // Bit 13
		u16 indicator_2	 : 1; // Bit 14
		u16 indicator_1	 : 1; // Bit 15
	};
	u16 state;
} encoder_led_s;

/* ~~~~~~~~~~~~~~~~~~~~ Precomputed Lookup Tables (LUTs) ~~~~~~~~~~~~~~~~~~~ */

// LUT for individual indicator masks (index 0 unused)
static const u16 INDICATOR_MASKS[NUM_INDICATOR_LEDS + 1] = {
		0, // Index 0 unused
		INDICATOR_MASK(1),
		INDICATOR_MASK(2),
		INDICATOR_MASK(3),
		INDICATOR_MASK(4),
		INDICATOR_MASK(5),
		INDICATOR_MASK(6),
		INDICATOR_MASK(7),
		INDICATOR_MASK(8),
		INDICATOR_MASK(9),
		INDICATOR_MASK(10),
		INDICATOR_MASK(11),
};

// LUT for standard bar graph patterns (index 0 = off, 1-11 = LEDs 1..index ON)
static const u16 BAR_GRAPH_MASKS[NUM_INDICATOR_LEDS + 1] = {
		0x0000, // 0 LEDs
		0x8000, // 1
		0xC000, // 1-2
		0xE000, // 1-3
		0xF000, // 1-4
		0xF800, // 1-5
		0xFC00, // 1-6
		0xFE00, // 1-7
		0xFF00, // 1-8
		0xFF80, // 1-9
		0xFFC0, // 1-10
		0xFFE0	// 1-11
};

// LUT for center-out detent patterns (index 0 = off, 1-5=idx..5, 6=off,
// 7-11=7..idx)
static const u16 CENTER_OUT_MASKS[NUM_INDICATOR_LEDS + 1] = {
		0x0000, // 0 LEDs (or invalid index)
		0xF800, // 1-5 (Index 1)
		0x7800, // 2-5 (Index 2)
		0x3800, // 3-5 (Index 3)
		0x1800, // 4-5 (Index 4)
		0x0800, // 5   (Index 5)
		0x0000, // Center OFF (Index 6)
		0x0200, // 7   (Index 7)
		0x0300, // 7-8 (Index 8)
		0x0380, // 7-9 (Index 9)
		0x03C0, // 7-10 (Index 10)
		0x03E0	// 7-11 (Index 11)
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int display_init(void) {
	// Request a display update for every encoder
	for (uint e = 0; e < NUM_ENCODERS; e++) {
		struct encoder* enc = &gENCODERS[gRT.curr_bank][e];
		enc->update_display = 1;
	}
	return 0;
}

void display_update(void) {
	u32 time_now = systime_ms();

	for (uint e = 0; e < NUM_ENCODERS; e++) {
		struct encoder* enc = &gENCODERS[gRT.curr_bank][e];

		if (enc->update_display != 0 &&
				(time_now - enc->update_display) > (500 / NUM_PWM_FRAMES)) {
			mf_draw_encoder(enc);
			enc->update_display = 0;
		}
	}
}

/**
 * @brief Draws the LED state for a given encoder. Highly optimized using LUTs.
 * @param enc Pointer to the encoder structure.
 * @return 0 on success, error code otherwise.
 */
int mf_draw_encoder(struct encoder* enc) {
	assert(enc != NULL);
	assert(enc->idx < NUM_ENCODERS);
	assert(NUM_INDICATOR_LEDS == 11); // LUTs assume this size

	// --- 1. Fetch frequently used data ---
	struct virtmap*					vmap				= &enc->vmaps[enc->vmap_active];
	const u8								current_pos = vmap->curr_pos;
	const enum display_mode mode				= enc->display.mode;
	const bool							is_detent		= enc->detent;
	const u8								enc_idx			= enc->idx;

	// --- 2. Calculate leading LED index ---
	// Map encoder position (0..ENC_MAX) -> LED index (1..NUM_INDICATOR_LEDS)
	u8 led_index;
	if (current_pos == 0) {
		led_index = 1;
	} else if (current_pos >= ENC_MAX) {
		led_index = NUM_INDICATOR_LEDS;
	} else {
		// Ceiling division: ((pos * NUM_LEDS) + MAX - 1) / MAX
		led_index = ((u32)current_pos * NUM_INDICATOR_LEDS + ENC_MAX - 1) / ENC_MAX;
		// Clamp (should be redundant if calculation is correct for pos in
		// [1..MAX-1])
		if (led_index < 1)
			led_index = 1;
		if (led_index > NUM_INDICATOR_LEDS)
			led_index = NUM_INDICATOR_LEDS;
	}

	// --- 3. Determine Base Indicator Pattern & PWM Setup ---
	u16	 base_indicator_state			= 0;
	u8	 pwm_brightness						= 0;
	u16	 led_pwm_mask							= 0; // Mask for the single LED being PWM'd
	u8	 effective_pwm_brightness = 0; // Brightness threshold for dimming check
	bool apply_pwm_dimming				= false; // Flag to enable dimming check in loop

	switch (mode) {
		case DIS_MODE_SINGLE:
			base_indicator_state = INDICATOR_MASKS[led_index];
			break;

		case DIS_MODE_MULTI_PWM:
			// Calculate PWM brightness (0 to NUM_PWM_FRAMES-1)
			if (current_pos == ENC_MAX) {
				pwm_brightness = NUM_PWM_FRAMES - 1;
			} else if (current_pos > 0) {
				u32 scaled_pos =
						((u32)current_pos * NUM_INDICATOR_LEDS * 256) / ENC_MAX;
				u16 base_pos_for_led = (led_index - 1) * 256;
				u16 pos_in_led			 = (scaled_pos >= base_pos_for_led)
																	 ? (scaled_pos - base_pos_for_led)
																	 : 0;
				pwm_brightness			 = (pos_in_led * NUM_PWM_FRAMES) >> 8;
				if (pwm_brightness >= NUM_PWM_FRAMES) {
					pwm_brightness = NUM_PWM_FRAMES - 1;
				}
			}
			// else: pwm_brightness = 0 for current_pos == 0

			// Setup for PWM dimming in the loop
			led_pwm_mask						 = INDICATOR_MASKS[led_index];
			effective_pwm_brightness = pwm_brightness;
			apply_pwm_dimming				 = true; // Enable the dimming check

			// Apply brightness inversion quirk for detent mode, left side
			if (is_detent && led_index < CENTER_INDICATOR) {
				effective_pwm_brightness = (NUM_PWM_FRAMES - 1) - pwm_brightness;
			}
			// fallthrough

		case DIS_MODE_MULTI:
			// Lookup base pattern from LUT
			base_indicator_state =
					is_detent ? CENTER_OUT_MASKS[led_index] : BAR_GRAPH_MASKS[led_index];
			break;

		default: return ERR_BAD_PARAM;
	}

	if (is_detent) {
		base_indicator_state &= ~CENTER_INDICATOR_MASK;
	}

	// --- 4. Pre-fetch BCM Brightness Values ---
	const u8 rgb_r = vmap->rgb.red;
	const u8 rgb_g = vmap->rgb.green;
	const u8 rgb_b = vmap->rgb.blue;
	const u8 det_r = is_detent ? vmap->rb.red : 0;
	const u8 det_b = is_detent ? vmap->rb.blue : 0;

	// --- 5. Generate PWM/BCM frames ---
	for (unsigned int f = 0; f < NUM_PWM_FRAMES; ++f) {
		// Start with the base indicator pattern for this frame
		u16 current_indicator_state = base_indicator_state;

		// Apply PWM dimming if needed (turn OFF leading LED if frame >= brightness)
		if (apply_pwm_dimming && (f >= effective_pwm_brightness)) {
			current_indicator_state &= ~led_pwm_mask;
		}

		// Calculate BCM bits for this frame (directly build the lower part of the
		// state)
		u16 current_bcm_bits = 0;
		if (rgb_r > f)
			current_bcm_bits |= (1 << RGB_RED_BIT);
		if (rgb_g > f)
			current_bcm_bits |= (1 << RGB_GREEN_BIT);
		if (rgb_b > f)
			current_bcm_bits |= (1 << RGB_BLUE_BIT);
		// Detent BCM bits are only added if is_detent is true (det_r/det_b are 0
		// otherwise)
		if (det_r > f)
			current_bcm_bits |= (1 << DETENT_RED_BIT);
		if (det_b > f)
			current_bcm_bits |= (1 << DETENT_BLUE_BIT);

		// Combine indicator state and BCM bits
		u16 final_state = current_indicator_state | current_bcm_bits;

		// --- 6. Write to Frame Buffer (Inverted) ---
		gFRAME_BUFFER[f][enc_idx] = ~final_state;
	}

	return 0; // Success
}
