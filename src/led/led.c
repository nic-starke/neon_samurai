/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <math.h>
#include <float.h>

#include "system/hardware.h"
#include "io/encoder.h"
#include "led/led.h"
#include "event/event.h"
#include "event/io.h"
#include "event/midi.h"
#include "system/time.h"
#include "system/utility.h"
#include "console/console.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define MASK_INDICATORS				 (0xFFE0)
#define LEFT_INDICATORS_MASK	 (0xF800) // Indicators 1-5
#define RIGHT_INDICATORS_MASK	 (0x03E0) // Indicators 7-11
#define CLEAR_LEFT_INDICATORS	 (0x03FF) // Mask to clear mid and left-side leds
#define CLEAR_RIGHT_INDICATORS (0xF80F) // Mask to clear mid and right-side leds

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef union {
	struct {
		u16 detent_blue	 : 1;
		u16 detent_red	 : 1;
		u16 rgb_blue		 : 1;
		u16 rgb_red			 : 1;
		u16 rgb_green		 : 1;
		u16 indicator_11 : 1; // Bit 10
		u16 indicator_10 : 1; // Bit 9
		u16 indicator_9	 : 1; // Bit 8
		u16 indicator_8	 : 1; // Bit 7
		u16 indicator_7	 : 1; // Bit 6
		u16 indicator_6	 : 1; // Bit 5 - Center Detent Indicator
		u16 indicator_5	 : 1; // Bit 4
		u16 indicator_4	 : 1; // Bit 3
		u16 indicator_3	 : 1; // Bit 2
		u16 indicator_2	 : 1; // Bit 1
		u16 indicator_1	 : 1; // Bit 0 - MSB relative to indicators (0x8000 base
													// mask)
	};
	u16 state;
} encoder_led_s;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
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

		/* This routine is called every 1ms, and the display is updated every
		 * 1000 / NUM_PWM_FRAMES == 31.25ms */
		if (enc->update_display != 0 &&
				(time_now - enc->update_display) > (1000 / NUM_PWM_FRAMES)) {
			mf_draw_encoder(enc);
			enc->update_display = 0;
		}
	}
}

int mf_draw_encoder(struct encoder* enc) {
	assert(enc);

	u8 current_pos = enc->vmaps[enc->vmap_active].curr_pos;
	u8 led_index	 = 0;
	u8 pwm_brightness =
			0; // Calculated brightness for the leading LED (0=dim, 31=max)

	// Calculate which indicator LED corresponds to the current encoder position
	if (current_pos == ENC_MAX) {
		led_index = NUM_INDICATOR_LEDS; // 11
	} else if (current_pos > 0) {
		// Map 1..ENC_MAX to 1..NUM_INDICATOR_LEDS using integer ceiling division
		led_index = ((u32)current_pos * NUM_INDICATOR_LEDS + ENC_MAX - 1) / ENC_MAX;
	} else {				 // current_pos == 0
		led_index = 1; // Map position 0 to LED 1
	}
	// Clamp index to valid range [1, 11]
	if (led_index > NUM_INDICATOR_LEDS)
		led_index = NUM_INDICATOR_LEDS;
	if (led_index < 1)
		led_index = 1;

	encoder_led_s leds = {0}; // Holds the base LED state for the entire PWM cycle

	// Calculate Base LED State Pattern and PWM Brightness
	switch (enc->display.mode) {
		case DIS_MODE_SINGLE: {
			// Turn on only the single LED corresponding to led_index
			if (led_index >= 1 && led_index <= NUM_INDICATOR_LEDS) {
				leds.state = (0x8000 >> (led_index - 1));
			}
			break; // Exit switch
		}

			// Fallthrough intended: MULTI_PWM calculates brightness then uses MULTI
			// logic for base pattern
		case DIS_MODE_MULTI_PWM: {
			// Calculate desired brightness (0-31, Dim->Bright) for the leading LED
			if (current_pos == ENC_MAX) {
				pwm_brightness = NUM_PWM_FRAMES - 1; // Max brightness at max position
			} else if (current_pos == 0) {
				pwm_brightness = 0; // Min brightness (off/dimmest) at position 0
			} else {
				// Calculate fractional position (0-255) within the current LED's range
				// using 8.8 fixed point
				u32 scaled_pos =
						((u32)current_pos * NUM_INDICATOR_LEDS * 256) / ENC_MAX;
				u16 base_pos_for_led = (led_index > 0) ? ((led_index - 1) * 256) : 0;
				u16 pos_in_led			 = (scaled_pos >= base_pos_for_led)
																	 ? (scaled_pos - base_pos_for_led)
																	 : 0;

				// Scale fractional position (0-255) to brightness (0-31)
				pwm_brightness = (pos_in_led * NUM_PWM_FRAMES) >> 8;
				if (pwm_brightness >= NUM_PWM_FRAMES) { // Clamp to max
					pwm_brightness = NUM_PWM_FRAMES - 1;
				}
			}
		} // fallthrough

		case DIS_MODE_MULTI: {
			// Calculate base indicator pattern (which LEDs are fully ON)
			if (enc->detent) {
				// Center-out pattern from middle (LED 6)
				if (led_index < 6) { // Light LEDs index..5
					for (int i = led_index; i <= 5; ++i) {
						if (i >= 1)
							leds.state |= (0x8000 >> (i - 1));
					}
				} else if (led_index > 6) { // Light LEDs 7..index
					for (int i = 7; i <= led_index; ++i) {
						if (i <= NUM_INDICATOR_LEDS)
							leds.state |= (0x8000 >> (i - 1));
					}
				}
			} else { // Non-detent bar graph: Light LEDs 1..index
				for (int i = 1; i <= led_index; ++i) {
					if (i <= NUM_INDICATOR_LEDS)
						leds.state |= (0x8000 >> (i - 1));
				}
			}
			break; // Break for MULTI modes
		}
		default: return ERR_BAD_PARAM;
	}

	// Ensure center LED (indicator 6) is always off in detent mode
	if (enc->detent) {
		leds.indicator_6 = 0;
	}

	// Generate states for each PWM frame and store in buffer
	for (unsigned int f = 0; f < NUM_PWM_FRAMES; ++f) {
		encoder_led_s frame_leds =
				leds; // Start with the calculated base pattern for this frame

		// Apply PWM dimming to the leading indicator LED if in MULTI_PWM mode
		if (enc->display.mode == DIS_MODE_MULTI_PWM) {
			if (led_index >= 1 && led_index <= NUM_INDICATOR_LEDS) {
				u16 index_mask =
						(0x8000 >> (led_index - 1)); // Mask for the specific LED

				// Invert calculated brightness only for detent mode, left side (<6)
				// This corrects the perceived brightness profile for this specific
				// case.
				u8 effective_pwm_brightness = pwm_brightness;
				if (enc->detent && led_index < 6) {
					effective_pwm_brightness = (NUM_PWM_FRAMES - 1) - pwm_brightness;
				}

				// Dimming Logic: Turn OFF the leading LED if the current frame 'f'
				// is at or beyond the desired brightness level (number of ON frames).
				if (f >= effective_pwm_brightness) {
					frame_leds.state &=
							~index_mask; // Clear the bit (turn LED off for this frame)
				}
			}
		}

		// Apply Binary Code Modulation (BCM) for RGB and Detent LEDs
		frame_leds.rgb_blue		 = 0;
		frame_leds.rgb_red		 = 0;
		frame_leds.rgb_green	 = 0;
		frame_leds.detent_blue = 0;
		frame_leds.detent_red	 = 0;
		struct virtmap* vmap	 = &enc->vmaps[enc->vmap_active];
		// Turn LED ON for this frame if its brightness value > current frame index
		if (vmap->rgb.red > f)
			frame_leds.rgb_red = 1;
		if (vmap->rgb.green > f)
			frame_leds.rgb_green = 1;
		if (vmap->rgb.blue > f)
			frame_leds.rgb_blue = 1;
		if (enc->detent) {
			if (vmap->rb.red > f)
				frame_leds.detent_red = 1;
			if (vmap->rb.blue > f)
				frame_leds.detent_blue = 1;
		}

		// Write the final LED state to the frame buffer
		// Hardware requires inverted state (0 = LED ON, 1 = LED OFF)
		gFRAME_BUFFER[f][enc->idx] = ~frame_leds.state;
	}

	return 0;
}
