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
#include "led/animation.h"
#include "led/led.h"
#include "system/config.h"
#include "system/error.h"
#include "system/hardware.h"
#include "system/time.h"
#include "system/utility.h"
#include "lfo/lfo.h"  // Include LFO header

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

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
	for (int e = 0; e < NUM_ENCODERS; e++) {
		struct encoder* enc = &gENCODERS[gRT.curr_bank][e];
		enc->update_display = 1;
	}
	return 0;
}

void display_update(void) {
	u32 time_now = systime_ms();

	if (animation_is_active()) {
		int ret = animation_update();
		// At this point we need to be careful about drawing over the animation frame, or the normal encoder frame.
		// For now we just return if any animation is active.
		return;
	}

	// Normal display update when no animation is active
	for (int e = 0; e < NUM_ENCODERS; e++) {
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

	// Check for special LFO display modes
	enum lfo_display_priority lfo_display_mode = input_manager_get_active_lfo_display_mode();
	bool lfo_rate_mode = (lfo_display_mode == LFO_DISPLAY_PRIO_RATE);
	bool lfo_depth_mode = (lfo_display_mode == LFO_DISPLAY_PRIO_DEPTH);

	// Check for LFO menu pages
	enum lfo_menu_page lfo_menu = input_manager_get_lfo_menu_page();

	// --- 1. Fetch frequently used data ---
	struct virtmap*	vmap = &enc->vmaps[enc->vmap_active];
	u8 base_pos = vmap->curr_pos;  // Use actual encoder position (already LFO-modulated if applicable)
	u8 current_pos = base_pos;     // Position to use for display
	const enum display_mode mode = enc->display.mode;
	const bool is_detent = enc->detent;
	const u8 enc_idx = enc->idx;

	const bool is_at_mid = (current_pos == ENC_MID);

	// Special handling for LFO Menu display modes
	if (lfo_menu != LFO_MENU_PAGE_NONE && enc_idx < MAX_LFOS) {
		enum lfo_waveform waveform = lfo_get_waveform(enc_idx);

		// Define colors for each LFO menu mode
		u8 rgb_r = 0, rgb_g = 0, rgb_b = 0;
		u16 base_indicator_state = 0;

		// Set color based on LFO waveform type
		switch (waveform) {
			case LFO_WAVE_SINE:
				rgb_r = 0x1F; // Red for sine
				break;
			case LFO_WAVE_TRIANGLE:
				rgb_g = 0x1F; // Green for triangle
				break;
			case LFO_WAVE_SQUARE:
				rgb_b = 0x1F; // Blue for square
				break;
			case LFO_WAVE_SAWTOOTH_UP:
				rgb_r = 0x1F; rgb_g = 0x1F; // Yellow for saw up
				break;
			case LFO_WAVE_SAWTOOTH_DN:
				rgb_r = 0x1F; rgb_b = 0x1F; // Purple for saw down
				break;
			case LFO_WAVE_SAMPLE_HOLD:
				rgb_r = 0x1F; rgb_g = 0x1F; rgb_b = 0x1F; // White for sample & hold
				break;
			default:
				// For LFO_WAVE_NONE or inactive LFOs, RGB LEDs should be off in all menu pages
				rgb_r = 0; rgb_g = 0; rgb_b = 0; // No color for inactive LFOs
				break;
		}

		// For active LFOs, make the indicator pattern based on menu page
		if (waveform != LFO_WAVE_NONE) {
			// First check if this LFO is assigned to the current bank
			bool lfo_on_current_bank = false;
			for (u8 i = 0; i < MAX_LFOS; i++) {
				if (i == enc_idx && gLFOs[i].active &&
					gLFOs[i].assigned_bank == gRT.curr_bank) {
					lfo_on_current_bank = true;
					break;
				}
			}

			// Only display indicator patterns for LFOs on the current bank
			if (lfo_on_current_bank) {
				// LFO_MENU_PAGE_WAVEFORM has been removed, so only handle existing menu pages
				if (lfo_menu == LFO_MENU_PAGE_RATE) {
					// For rate page, use logarithmic scale and multi-PWM display to show rate precisely
					float rate = lfo_get_rate(enc_idx);
					float max_rate = 10.0f; // Max is now 10Hz

					// Map rate to display using logarithmic scale for better visual distribution
					// At 0.01Hz (min): LED 1
					// At 0.1Hz: LED 4
					// At 1Hz: LED 7
					// At 10Hz (max): LED 11

					// Convert rate to log scale (log10)
					float log_rate = 0;
					if (rate > 0.0f) {
						log_rate = log10f(rate); // -2 (0.01Hz) to 1 (10Hz)
					}

					// Normalize to 0.0-1.0 range for display
					float normalized_log_rate = (log_rate + 2.0f) / 3.0f; // -2 → 0.0, 1 → 1.0

					// Make sure maximum rate (10Hz) can properly reach the full range
					if (rate >= max_rate) {
						normalized_log_rate = 1.0f;
					}

					// Map to LED range (1 to 11)
					float led_position = 1.0f + normalized_log_rate * (NUM_INDICATOR_LEDS - 1);

					// Get integer LED index and fractional part for PWM
					u8 rate_led_index = (u8)led_position;
					float fractional_part = led_position - rate_led_index;

					// Ensure rate_led_index is in valid range 1-11
					rate_led_index = CLAMP(rate_led_index, 1, NUM_INDICATOR_LEDS);

					// Calculate PWM brightness from fractional part
					u8 pwm_brightness = (u8)(fractional_part * (NUM_PWM_FRAMES - 1));

					// For maximum rate, ensure the final LED is fully lit
					if (rate >= max_rate) {
						rate_led_index = NUM_INDICATOR_LEDS;
						pwm_brightness = NUM_PWM_FRAMES - 1;
					}

					// Set up base pattern (all LEDs up to but not including the PWM'd one)
					base_indicator_state = BAR_GRAPH_MASKS[rate_led_index - 1];
					u16 leading_led_mask = INDICATOR_MASKS[rate_led_index];

					// Generate PWM frames for rate display
					for (unsigned int f = 0; f < NUM_PWM_FRAMES; ++f) {
						u16 current_state = base_indicator_state;

						// Add the leading LED with PWM if we're at the right frame
						if (f < pwm_brightness) {
							current_state |= leading_led_mask;
						}

						// Add RGB bits based on current PWM frame
						if (rgb_r > f) current_state |= (1 << RGB_RED_BIT);
						if (rgb_g > f) current_state |= (1 << RGB_GREEN_BIT);
						if (rgb_b > f) current_state |= (1 << RGB_BLUE_BIT);

						// Write to frame buffer (inverted)
						gFRAME_BUFFER[f][enc_idx] = ~current_state;
					}

					return 0; // Skip the rest of the processing
				} else if (lfo_menu == LFO_MENU_PAGE_DEPTH) {
					// For depth page, use multi-PWM display to show depth precisely
					i8 depth = lfo_get_depth(enc_idx);

					// Calculate base LED index and PWM brightness
					u8 depth_led_index;
					u8 pwm_brightness = 0;

					if (depth == 0) {
						// At center, just show the center LED fully lit
						depth_led_index = CENTER_INDICATOR;
						pwm_brightness = NUM_PWM_FRAMES - 1;
					} else if (depth > 0) {
						// Positive depth (1% to 100%)
						float normalized_depth = depth / 100.0f; // Convert to 0.0-1.0 range
						// Adjusted mapping to ensure we can reach LED 11 at 100% depth
						float led_position = CENTER_INDICATOR + normalized_depth * 5.999f; // Map to LED positions 6.0-11.999

						// Calculate the integer and fractional parts
						depth_led_index = (u8)led_position; // Integer part
						if (depth_led_index > NUM_INDICATOR_LEDS) depth_led_index = NUM_INDICATOR_LEDS;

						float fractional = led_position - (float)depth_led_index; // Fractional part
						pwm_brightness = (u8)(fractional * (NUM_PWM_FRAMES - 1));
					} else {
						// Negative depth (-1% to -100%)
						float normalized_depth = -depth / 100.0f; // Convert to 0.0-1.0 range
						// Adjusted mapping to ensure we can reach LED 1 at -100% depth
						float led_position = CENTER_INDICATOR - normalized_depth * 5.999f; // Map to LED positions 6.0-0.001

						// Calculate the integer and fractional parts (reverse direction for negative)
						depth_led_index = (u8)ceil(led_position); // Ceiling for negative direction
						if (depth_led_index < 1) depth_led_index = 1;

						float fractional = depth_led_index - led_position; // Fractional part (reversed)
						pwm_brightness = (u8)(fractional * (NUM_PWM_FRAMES - 1));
					}

					// Get base pattern (all LEDs up to but not including the PWM'd one)
					u16 base_indicator_state = 0;
					u16 leading_led_mask = 0;

					if (depth == 0) {
						// Just center LED for zero depth
						base_indicator_state = CENTER_INDICATOR_MASK;
						leading_led_mask = 0; // No PWM needed
					} else if (depth > 0) {
						// For positive depth, light LEDs 7 through depth_led_index-1 and PWM depth_led_index
						for (u8 i = CENTER_INDICATOR + 1; i < depth_led_index; i++) {
							base_indicator_state |= INDICATOR_MASKS[i];
						}
						base_indicator_state |= CENTER_INDICATOR_MASK; // Always show center
						leading_led_mask = INDICATOR_MASKS[depth_led_index];
					} else {
						// For negative depth, light LEDs 5 through depth_led_index+1 and PWM depth_led_index
						for (u8 i = CENTER_INDICATOR - 1; i > depth_led_index; i--) {
							base_indicator_state |= INDICATOR_MASKS[i];
						}
						base_indicator_state |= CENTER_INDICATOR_MASK; // Always show center
						leading_led_mask = INDICATOR_MASKS[depth_led_index];
					}

					// Generate PWM frames for depth display
					for (unsigned int f = 0; f < NUM_PWM_FRAMES; ++f) {
						u16 current_state = base_indicator_state;

						// Add the leading LED with PWM if we're at the right frame and there is a leading LED
						if (leading_led_mask != 0 && f < pwm_brightness) {
							current_state |= leading_led_mask;
						}

						// Add RGB bits based on current PWM frame
						if (rgb_r > f) current_state |= (1 << RGB_RED_BIT);
						if (rgb_g > f) current_state |= (1 << RGB_GREEN_BIT);
						if (rgb_b > f) current_state |= (1 << RGB_BLUE_BIT);

						// Write to frame buffer (inverted)
						gFRAME_BUFFER[f][enc_idx] = ~current_state;
					}

					return 0; // Skip the rest of the processing
				}
			} else {
				// Inactive LFO patterns per page - no indicators for inactive LFOs in any menu
				base_indicator_state = 0;
			}

			// Generate PWM frames for LFO menu display
			for (unsigned int f = 0; f < NUM_PWM_FRAMES; ++f) {
				u16 current_state = base_indicator_state;

				// Add RGB bits based on current PWM frame
				if (rgb_r > f) current_state |= (1 << RGB_RED_BIT);
				if (rgb_g > f) current_state |= (1 << RGB_GREEN_BIT);
				if (rgb_b > f) current_state |= (1 << RGB_BLUE_BIT);

				// Write to frame buffer (inverted)
				gFRAME_BUFFER[f][enc_idx] = ~current_state;
			}

			return 0; // Successfully drew LFO menu state
		}

		// Special handling for LFO rate display mode
		if (lfo_rate_mode && enc_idx < MAX_LFOS) {
			enum lfo_waveform waveform = lfo_get_waveform(enc_idx);
			if (waveform != LFO_WAVE_NONE) {
				// Get LFO rate (0.01Hz to 1000Hz) and convert to LED display
				float rate = lfo_get_rate(enc_idx);

				// Map rate to LED index using logarithmic scale for better visual distribution
				// 0.01-0.1Hz -> LED 1-2 (very slow)
				// 0.1-1Hz -> LED 3-4 (slow)
				// 1-10Hz -> LED 5-6 (medium)
				// 10-100Hz -> LED 7-8 (fast)
				// 100-1000Hz -> LED 9-11 (very fast)

				u8 rate_led_index;
				if (rate <= 0.1f) {
					// Very slow: 0.01-0.1Hz maps to LEDs 1-2
					rate_led_index = 1 + (u8)((rate - 0.01f) / 0.09f * 1.0f);
				} else if (rate <= 1.0f) {
					// Slow: 0.1-1Hz maps to LEDs 3-4
					rate_led_index = 3 + (u8)((rate - 0.1f) / 0.9f * 1.0f);
				} else if (rate <= 10.0f) {
					// Medium: 1-10Hz maps to LEDs 5-6
					rate_led_index = 5 + (u8)((rate - 1.0f) / 9.0f * 1.0f);
				} else if (rate <= 100.0f) {
					// Fast: 10-100Hz maps to LEDs 7-8
					rate_led_index = 7 + (u8)((rate - 10.0f) / 90.0f * 1.0f);
				} else {
					// Very fast: 100-1000Hz maps to LEDs 9-11
					rate_led_index = 9 + (u8)((rate - 100.0f) / 900.0f * 2.0f);
				}

				// Clamp to valid range
				if (rate_led_index < 1) rate_led_index = 1;
				if (rate_led_index > NUM_INDICATOR_LEDS) rate_led_index = NUM_INDICATOR_LEDS;

				// Use bar graph pattern to show rate level
				u16 base_indicator_state = BAR_GRAPH_MASKS[rate_led_index];

				// Set RGB color based on LFO waveform type (same as depth mode)
				u8 rgb_r = 0, rgb_g = 0, rgb_b = 0;
				switch (waveform) {
					case LFO_WAVE_SINE:
						rgb_r = 0x1F; // Red for sine
						break;
					case LFO_WAVE_TRIANGLE:
						rgb_g = 0x1F; // Green for triangle
						break;
					case LFO_WAVE_SQUARE:
						rgb_b = 0x1F; // Blue for square
						break;
					case LFO_WAVE_SAWTOOTH_UP:
						rgb_r = 0x1F; rgb_g = 0x1F; // Yellow for saw up
						break;
					case LFO_WAVE_SAWTOOTH_DN:
						rgb_r = 0x1F; rgb_b = 0x1F; // Purple for saw down
						break;
					default:
						rgb_r = 0x0F; rgb_g = 0x0F; rgb_b = 0x0F; // Dim white for unknown
						break;
				}

				// Generate PWM frames for LFO rate display
				for (unsigned int f = 0; f < NUM_PWM_FRAMES; ++f) {
					u16 current_state = base_indicator_state;

					// Add RGB bits based on current PWM frame
					if (rgb_r > f) current_state |= (1 << RGB_RED_BIT);
					if (rgb_g > f) current_state |= (1 << RGB_GREEN_BIT);
					if (rgb_b > f) current_state |= (1 << RGB_BLUE_BIT);

					// Set frame buffer (with inverted state as per existing code)
					gFRAME_BUFFER[f][enc_idx] = ~current_state;
				}

				return 0;
			}
		}

		// Special handling for LFO depth display mode
		if (lfo_depth_mode && enc_idx < MAX_LFOS) {
			enum lfo_waveform waveform = lfo_get_waveform(enc_idx);
			if (waveform != LFO_WAVE_NONE) {
				// Get LFO depth (-100% to +100%) and convert to LED display using detent style
				i8 depth = lfo_get_depth(enc_idx);

				// Map depth to LED index exactly like detent encoder positioning
				// Center (12 o'clock) = 0% depth = LED 6 (CENTER_INDICATOR)
				// Positive depth (1% to 100%) = clockwise from center (LEDs 7, 8, 9, 10, 11)
				// Negative depth (-1% to -100%) = counter-clockwise from center (LEDs 5, 4, 3, 2, 1)

				u8 depth_led_index;
				if (depth == 0) {
					depth_led_index = (u8)CENTER_INDICATOR; // Center position for 0% depth
				} else if (depth > 0) {
					// Positive depth: map 1-100% to LEDs 7-11 (clockwise from center)
					// depth 1-20% -> LED 7
					// depth 21-40% -> LED 8
					// depth 41-60% -> LED 9
					// depth 61-80% -> LED 10
					// depth 81-100% -> LED 11
					depth_led_index = (u8)(CENTER_INDICATOR + 1 + ((depth - 1) * 4 / 99));

					// Clamp to valid range (LEDs 7-11)
					if (depth_led_index > NUM_INDICATOR_LEDS) {
						depth_led_index = NUM_INDICATOR_LEDS;
					}
				} else {
					// Negative depth: map -1% to -100% to LEDs 5-1 (counter-clockwise from center)
					// depth -1% to -20% -> LED 5
					// depth -21% to -40% -> LED 4
					// depth -41% to -60% -> LED 3
					// depth -61% to -80% -> LED 2
					// depth -81% to -100% -> LED 1
					i8 abs_depth = (i8)(-depth); // Make positive for calculation
					depth_led_index = (u8)(CENTER_INDICATOR - 1 - ((abs_depth - 1) * 4 / 99));

					// Clamp to valid range (LEDs 1-5)
					if (depth_led_index < 1) {
						depth_led_index = 1;
					}
				}

				// Use detent center-out pattern for the depth display
				// This will light up LEDs from center to the calculated position
				u16 base_indicator_state = CENTER_OUT_MASKS[depth_led_index];

				// Always show center LED for depth display (it represents 0% baseline)
				base_indicator_state |= CENTER_INDICATOR_MASK;

				// Set RGB color based on LFO waveform type
				u8 rgb_r = 0, rgb_g = 0, rgb_b = 0;
				switch (waveform) {
					case LFO_WAVE_SINE:
						rgb_r = 0x1F; // Red for sine
						break;
					case LFO_WAVE_TRIANGLE:
						rgb_g = 0x1F; // Green for triangle
						break;
					case LFO_WAVE_SQUARE:
						rgb_b = 0x1F; // Blue for square
						break;
					case LFO_WAVE_SAWTOOTH_UP:
						rgb_r = 0x1F; rgb_g = 0x1F; // Yellow for saw up
						break;
					case LFO_WAVE_SAWTOOTH_DN:
						rgb_r = 0x1F; rgb_b = 0x1F; // Purple for saw down
						break;
					default:
						rgb_r = 0x0F; rgb_g = 0x0F; rgb_b = 0x0F; // Dim white for unknown
						break;
				}

				// Generate PWM frames for LFO depth display
				for (unsigned int f = 0; f < NUM_PWM_FRAMES; ++f) {
					u16 current_state = base_indicator_state;

					// Add RGB bits based on current PWM frame
					if (rgb_r > f) current_state |= (1 << RGB_RED_BIT);
					if (rgb_g > f) current_state |= (1 << RGB_GREEN_BIT);
					if (rgb_b > f) current_state |= (1 << RGB_BLUE_BIT);

					// Set frame buffer (with inverted state as per existing code)
					gFRAME_BUFFER[f][enc_idx] = ~current_state;
				}

				return 0;
			}
		}
	}

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

	// Adjust center indicator behavior for detent mode
	if (is_detent) {
		if (is_at_mid) {
			// Detent mode, AT middle: Center LED (indicator 6) should be ON.
			// RB LEDs (det_r, det_b) will be 0 because is_at_mid is true,
			// so they won't interfere here.
			base_indicator_state |= CENTER_INDICATOR_MASK;
		} else {
			// Detent mode, NOT at middle: Center LED should be OFF.
			// RB LEDs will be controlled by the det_r/det_b logic below.
			base_indicator_state &= ~CENTER_INDICATOR_MASK;
		}
	}
	// Note: The actual lighting of RB LEDs if not at mid is handled by the det_r/det_b logic below.
	// The key change here is that CENTER_INDICATOR_MASK is ON when at_mid, and OFF otherwise for detent encoders.
	// And the RB LEDs are only active if is_detent AND is_at_mid is FALSE (implicitly by their calculation).
	// Let's adjust the det_r/det_b calculation slightly to reflect this.


	// --- 4. Pre-fetch BCM Brightness Values ---
	// RGB colors are independent of detent status
	const u8 rgb_r = vmap->rgb.red;
	const u8 rgb_g = vmap->rgb.green;
	const u8 rgb_b = vmap->rgb.blue;

	// Show detent RB LEDs when NOT at middle position for detent encoders
	const u8 det_r = (is_detent && !is_at_mid) ? vmap->rb.red : 0;
	const u8 det_b = (is_detent && !is_at_mid) ? vmap->rb.blue : 0;

	// --- 5. Generate PWM/BCM frames ---
	for (unsigned int f = 0; f < NUM_PWM_FRAMES; ++f) {
		// Start with the base indicator pattern for this frame
		u16 current_indicator_state = base_indicator_state;

		// Apply PWM dimming if needed (turn OFF leading LED if frame >= brightness)
		// Exception: For detent encoders at middle position, never dim the center indicator
		if (apply_pwm_dimming && (f >= effective_pwm_brightness)) {
			// If this is a detent encoder at middle position and the PWM mask affects the center indicator,
			// preserve the center indicator while dimming other LEDs
			if (is_detent && is_at_mid && (led_pwm_mask & CENTER_INDICATOR_MASK)) {
				current_indicator_state &= ~(led_pwm_mask & ~CENTER_INDICATOR_MASK);
			} else {
				current_indicator_state &= ~led_pwm_mask;
			}
		}

		// Calculate BCM bits for this frame (directly build the lower part of the state)
		u16 current_bcm_bits = 0;

		// Set RGB bits based on PWM frame - these are unaffected by detent status
		if (rgb_r > f)
			current_bcm_bits |= (1 << RGB_RED_BIT);
		if (rgb_g > f)
			current_bcm_bits |= (1 << RGB_GREEN_BIT);
		if (rgb_b > f)
			current_bcm_bits |= (1 << RGB_BLUE_BIT);

		// Detent RB bits are only added if is_detent is true AND position is at middle
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
