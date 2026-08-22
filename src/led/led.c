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

#include <avr/pgmspace.h>

#include "io/encoder.h"
#include "led/led.h"
#include "system/config.h"
#include "system/error.h"
#include "system/hardware.h"
#include "system/time.h"
#include "system/utility.h"
#include "event/animation.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define INDICATOR_MASK(n) (0x8000 >> ((n) - 1)) // Mask for indicator n (1-11)
#define CENTER_INDICATOR	(6)
#define CENTER_INDICATOR_MASK                                                  \
INDICATOR_MASK(CENTER_INDICATOR) // Explicit mask for center
// Minimum age of a pending update before it is drawn, in milliseconds.
#define DISPLAY_UPDATE_MIN_MS (15)

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
static const u16 INDICATOR_MASKS[NUM_INDICATOR_LEDS + 1] PROGMEM = {
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
static const u16 BAR_GRAPH_MASKS[NUM_INDICATOR_LEDS + 1] PROGMEM = {
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
static const u16 CENTER_OUT_MASKS[NUM_INDICATOR_LEDS + 1] PROGMEM = {
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

static const u8 LED_INDEX_LUT[256] PROGMEM = {
		1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,
		1,	1,	1,	1,	1,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,	2,
		2,	2,	2,	2,	2,	2,	2,	2,	2,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,
		3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	4,	4,	4,	4,	4,	4,
		4,	4,	4,	4,	4,	4,	4,	4,	4,	4,	4,	4,	4,	4,	4,	4,	4,	5,	5,
		5,	5,	5,	5,	5,	5,	5,	5,	5,	5,	5,	5,	5,	5,	5,	5,	5,	5,	5,
		5,	5,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,	6,
		6,	6,	6,	6,	6,	6,	6,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,
		7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	7,	8,	8,	8,	8,	8,	8,	8,	8,
		8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	8,	9,	9,	9,	9,
		9,	9,	9,	9,	9,	9,	9,	9,	9,	9,	9,	9,	9,	9,	9,	9,	9,	9,	9,
		10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
		10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
		11, 11, 11, 11, 11, 11, 11, 11, 11,
};

static const u8 PWM_BRIGHTNESS_LUT[256] PROGMEM = {
		0,	 0,		1,	 3,		5,	 9,		13,	 18,	25,	 32,	40,	 49,	60,	 71,	84,
		98,	 113, 129, 146, 165, 184, 205, 227, 251, 0,		1,	 2,		5,	 8,		12,
		17,	 23,	30,	 39,	48,	 58,	69,	 82,	95,	 110, 126, 143, 161, 181, 201,
		223, 246, 0,	 1,		2,	 4,		7,	 11,	16,	 22,	29,	 37,	46,	 56,	67,
		79,	 93,	107, 123, 140, 158, 177, 197, 219, 242, 0,	 1,		2,	 4,		7,
		11,	 15,	21,	 28,	35,	 44,	54,	 65,	77,	 90,	105, 120, 137, 154, 173,
		194, 215, 238, 0,		0,	 2,		3,	 6,		10,	 14,	20,	 26,	34,	 43,	52,
		63,	 75,	88,	 102, 117, 133, 151, 170, 190, 211, 234, 0,		0,	 1,		3,
		6,	 9,		13,	 19,	25,	 33,	41,	 50,	61,	 73,	85,	 99,	114, 130, 148,
		166, 186, 207, 229, 253, 0,		1,	 3,		5,	 8,		13,	 18,	24,	 31,	39,
		49,	 59,	70,	 83,	97,	 111, 127, 145, 163, 182, 203, 225, 248, 0,		1,
		2,	 5,		8,	 12,	17,	 23,	30,	 38,	47,	 57,	68,	 81,	94,	 109, 124,
		141, 159, 179, 199, 221, 244, 0,	 1,		2,	 4,		7,	 11,	16,	 22,	28,
		36,	 45,	55,	 66,	78,	 91,	106, 121, 138, 156, 175, 196, 217, 240, 0,
		1,	 2,		4,	 6,		10,	 15,	20,	 27,	35,	 43,	53,	 64,	76,	 89,	103,
		119, 135, 153, 172, 192, 213, 236, 0,		0,	 1,		3,	 6,		9,	 14,	19,
		26,	 33,	42,	 51,	62,	 74,	87,	 100, 116, 132, 149, 168, 188, 209, 231,
		255,
};

_Static_assert(ENC_MAX == 255, "LED_INDEX_LUT is generated for ENC_MAX == 255");
_Static_assert(NUM_INDICATOR_LEDS == 11, "LUTs assume 11 indicator LEDs");
_Static_assert(NUM_BCM_PLANES == 8,
							 "the LUTs are generated for 8-bit brightness");

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int display_init(void) {
	// Request a display update for every encoder
	for (int e = 0; e < NUM_ENCODERS; e++) {
		struct encoder* enc = &gENCODERS[gRT.curr_bank][e];
		enc->update_display = 1;
	}
	return 0;
}

void display_update(void) {
	u32 time_now = systime_ms();

	// Check if an animation is active and has priority
	if (animation_is_active()) {
		// Update animation state (frame transitions, timing)
		animation_update();

		// Draw animation frame for each encoder
		for (int e = 0; e < NUM_ENCODERS; e++) {
			animation_draw_encoder(e);
		}
		return;
	}

	// Normal display update when no animation is active
	for (int e = 0; e < NUM_ENCODERS; e++) {
		struct encoder* enc = &gENCODERS[gRT.curr_bank][e];

		if (enc->update_display != 0 &&
				(time_now - enc->update_display) > DISPLAY_UPDATE_MIN_MS) {
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

	// --- 1. Fetch frequently used data ---
	struct virtmap*					vmap				= &enc->vmaps[enc->vmap_active];
	const u8								current_pos = vmap->curr_pos;
	const enum display_mode mode				= enc->display.mode;
	const bool							is_detent		= enc->detent;
	const u8								enc_idx			= enc->idx;
	const bool							is_at_mid		= (current_pos == ENC_MID);

	// --- 2. Look up leading LED index ---
	// Encoder position (0..ENC_MAX) -> LED index (1..NUM_INDICATOR_LEDS).
	const u8 led_index = pgm_read_byte(&LED_INDEX_LUT[current_pos]);

	// --- 3. Determine Base Indicator Pattern & PWM Setup ---
	u16	 base_indicator_state			= 0;
	u16	 led_pwm_mask							= 0; // Mask for the single LED being PWM'd
	u8	 effective_pwm_brightness = 0; // Brightness threshold for dimming check
	bool apply_pwm_dimming				= false; // Flag to enable dimming check in loop

	switch (mode) {
		case DIS_MODE_SINGLE:
			base_indicator_state = pgm_read_word(&INDICATOR_MASKS[led_index]);
			break;

		case DIS_MODE_MULTI_PWM: {
			const u8 pwm_brightness = pgm_read_byte(&PWM_BRIGHTNESS_LUT[current_pos]);

			// Setup for PWM dimming in the loop
			led_pwm_mask						 = pgm_read_word(&INDICATOR_MASKS[led_index]);
			effective_pwm_brightness = pwm_brightness;
			apply_pwm_dimming				 = true; // Enable the dimming check

			// Apply brightness inversion quirk for detent mode, left side
			if (is_detent && led_index < CENTER_INDICATOR) {
				effective_pwm_brightness = MAX_BRIGHTNESS - pwm_brightness;
			}
		}
			// fallthrough

		case DIS_MODE_MULTI:
			// Lookup base pattern from LUT
			base_indicator_state = is_detent
																 ? pgm_read_word(&CENTER_OUT_MASKS[led_index])
																 : pgm_read_word(&BAR_GRAPH_MASKS[led_index]);
			break;

		default: return ERR_BAD_PARAM;
	}

	// Adjust center indicator behavior for detent mode
	if (is_detent) {
		if (!is_at_mid) {
			// Detent mode, not at middle: Center LED should be ON.
			base_indicator_state |= CENTER_INDICATOR_MASK;

			// If in PWM mode and the current LED being PWM'd *is* the center LED,
			// prevent it from being dimmed by the PWM logic.
			if (mode == DIS_MODE_MULTI_PWM && led_index == CENTER_INDICATOR) {
				led_pwm_mask			= 0;		 // Don't target center LED for PWM dimming
				apply_pwm_dimming = false; // Disable PWM dimming for this case
			}
		} else {
			// Detent mode, at middle: Center LED should be OFF to show detent RB
			// LEDs.
			base_indicator_state &= ~CENTER_INDICATOR_MASK;
		}
	}

	// --- 4. Pre-fetch BCM Brightness Values ---
	// RGB colors are independent of detent status
	const u8 rgb_r = vmap->rgb.red;
	const u8 rgb_g = vmap->rgb.green;
	const u8 rgb_b = vmap->rgb.blue;

	// Only show detent RB LEDs when at middle position
	const u8 det_r = (is_detent && is_at_mid) ? vmap->rb.red : 0;
	const u8 det_b = (is_detent && is_at_mid) ? vmap->rb.blue : 0;

	// --- 5. Emit the BCM bit-planes ---
	// Plane p carries bit p of each channel's brightness, so writing a value
	// across the planes is just a bit transpose. Shifting each value right by
	// one per plane keeps this to a test-and-set per channel - indexing with
	// (v >> p) would cost a variable shift, which AVR has no instruction for.
	//
	// Indicator LEDs are binary, so they appear in every plane; only the
	// leading LED is modulated, via effective_pwm_brightness.
	volatile u16* dst = &gFRAME_BUFFER[0][enc_idx];

	u8 r	 = rgb_r;
	u8 g	 = rgb_g;
	u8 b	 = rgb_b;
	u8 dr	 = det_r;
	u8 db	 = det_b;
	u8 pwm = apply_pwm_dimming ? effective_pwm_brightness : MAX_BRIGHTNESS;

	for (u8 p = 0; p < NUM_BCM_PLANES; ++p) {
		u16 state = base_indicator_state;

		if ((pwm & 1u) == 0u) {
			state &= (u16)~led_pwm_mask;
		}

		if (r & 1u) {
			state |= (u16)(1u << RGB_RED_BIT);
		}
		if (g & 1u) {
			state |= (u16)(1u << RGB_GREEN_BIT);
		}
		if (b & 1u) {
			state |= (u16)(1u << RGB_BLUE_BIT);
		}
		if (dr & 1u) {
			state |= (u16)(1u << DETENT_RED_BIT);
		}
		if (db & 1u) {
			state |= (u16)(1u << DETENT_BLUE_BIT);
		}

		// --- 6. Write to Frame Buffer (Inverted) ---
		*dst = (u16)~state;
		dst += NUM_ENCODERS;

		r >>= 1;
		g >>= 1;
		b >>= 1;
		dr >>= 1;
		db >>= 1;
		pwm >>= 1;
	}

	return 0; // Success
}
