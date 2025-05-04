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

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define MASK_INDICATORS				 (0xFFE0)
#define LEFT_INDICATORS_MASK	 (0xF800) // 1111
#define RIGHT_INDICATORS_MASK	 (0x03E0)
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
		u16 indicator_11 : 1;
		u16 indicator_10 : 1;
		u16 indicator_9	 : 1;
		u16 indicator_8	 : 1;
		u16 indicator_7	 : 1;
		u16 indicator_6	 : 1;
		u16 indicator_5	 : 1;
		u16 indicator_4	 : 1;
		u16 indicator_3	 : 1;
		u16 indicator_2	 : 1;
		u16 indicator_1	 : 1;
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

	u8	current_pos = enc->vmaps[enc->vmap_active].curr_pos;
	u8	led_index		= 0;
	f32 pwm					= 0;

	if (current_pos == ENC_MAX) {
		led_index = NUM_INDICATOR_LEDS;
	} else if (current_pos != 0) {
		led_index = ceilf(((float)current_pos * NUM_INDICATOR_LEDS) / ENC_MAX);
	}

	encoder_led_s leds = {0};

	switch (enc->display.mode) {
		case DIS_MODE_SINGLE:
			if (enc->detent) {
				leds.state |= (0x8000 >> (led_index - 1));
			} else {
				leds.state = (0x8000 >> (led_index - 1));
			}
			break;

		case DIS_MODE_MULTI_PWM:
			// Determine the brightness to apply to the current indicator LED
			// Calculate fractional position between LEDs (0-31 range)
			if (current_pos == 0) {
				// At position 0, ensure the first LED is off
				pwm = NUM_PWM_FRAMES; // Set to max to ensure LED is off
			} else {
				pwm = ((float)current_pos * NUM_INDICATOR_LEDS * NUM_PWM_FRAMES) / ENC_MAX;
				pwm = NUM_PWM_FRAMES - (pwm - (led_index - 1) * NUM_PWM_FRAMES);
				if (pwm >= NUM_PWM_FRAMES) pwm = NUM_PWM_FRAMES - 1;
				if (pwm < 0) pwm = 0;
			}
			// fallthrough
		case DIS_MODE_MULTI: {
			if (enc->detent) {
				if (led_index < 6) {
					leds.state |=
							(LEFT_INDICATORS_MASK >> (led_index - 1)) & LEFT_INDICATORS_MASK;
				} else if (led_index > 6) {
					leds.state |=
							(LEFT_INDICATORS_MASK >> (led_index - 5)) & RIGHT_INDICATORS_MASK;
				}
			} else {
				leds.state = MASK_INDICATORS & ~(0xFFFF >> (led_index - 1));
			}
			break;
		}

		default: return ERR_BAD_PARAM;
	}

	if (enc->detent) {
		leds.indicator_6 = 0;
	}

	for (unsigned int f = 0; f < NUM_PWM_FRAMES; ++f) {
		/*
			In PWM display mode we control the brightness of the current indicator LED
			to display when the encoder is in between crossing between two indicators.
			For example, when the half way between indicator 1 and 2, indicator 2 is
			set to 50% brightness. To achieve this we just turn off the LED for 50% of
			the frames, and turn it on for the other 50%. But to do this we have to
			calculate a mask for the current indicator LED, and set it.
		*/
		if ((enc->display.mode == DIS_MODE_MULTI_PWM) && (pwm < f)) {
			leds.state |= (0x8000 >> (led_index - 1)) & MASK_INDICATORS;
			if (enc->detent) {
				leds.indicator_6 = 0;
			}
		}

		leds.rgb_blue		 = 0;
		leds.rgb_red		 = 0;
		leds.rgb_green	 = 0;
		leds.detent_blue = 0;
		leds.detent_red	 = 0;

		struct virtmap* vmap = &enc->vmaps[enc->vmap_active];
		if (vmap->rgb.red > f) {
			leds.rgb_red = 1;
		}

		if (vmap->rgb.green > f) {
			leds.rgb_green = 1;
		}

		if (vmap->rgb.blue > f) {
			leds.rgb_blue = 1;
		}

		if (enc->detent) {
			if (vmap->rb.red > f) {
				leds.detent_red = 1;
			}

			if (vmap->rb.blue > f) {
				leds.detent_blue = 1;
			}
		}

		// Write the LED state to the frame buffer
		// As 0 = LED on, 1 = LED off we invert all the states before writing
		gFRAME_BUFFER[f][enc->idx] = ~leds.state;
	}

	return 0;
}
