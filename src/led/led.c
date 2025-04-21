/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

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
#define MASK_PWM_INDICATORS		 (0xFBE0)

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

// static const u16 led_interval		= ENC_MAX / 11; // Not directly used in
// integer logic
static const u8 max_brightness = MAX_BRIGHTNESS;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void display_update(void) {

	u32 time_now = systime_ms();

	for (uint e = 0; e < NUM_ENCODERS; e++) {
		struct mf_encoder* enc = &gENCODERS[gRT.curr_bank][e];

		/* This routine is called every 1ms, and the display is updated every
		 * 1000 / NUM_PWM_FRAMES == 31.25ms */
		if (enc->update_display != 0 &&
				(time_now - enc->update_display) > (1000 / NUM_PWM_FRAMES)) {
			mf_draw_encoder(enc);
			enc->update_display = 0;
		}
	}
}

int mf_draw_encoder(struct mf_encoder* enc) {
	assert(enc);

	u8 current_pos			 = enc->vmaps[enc->vmap_active].curr_pos;
	u8 current_led_index = 0;

	volatile struct mf_encoder enc_copy = *enc;
	volatile u8 c_led_pos = current_pos;
	volatile u8 c_led_idx = current_led_index;
	if (current_pos == ENC_MAX) {
		current_led_index = NUM_INDICATOR_LEDS;
	} else if (current_pos != 0) {
		current_led_index = (current_pos * NUM_INDICATOR_LEDS) / ENC_MAX;
	}

	u16 remainder	 = (current_pos * NUM_INDICATOR_LEDS) % ENC_MAX;
	u8	max_frames = (remainder * NUM_PWM_FRAMES) / ENC_MAX;

	encoder_led_s leds = {0};

	switch (enc->display.mode) {
		case DIS_MODE_SINGLE:
			leds.state = MASK_INDICATORS & (0x8000 >> (current_led_index - 1));
			break;
		case DIS_MODE_MULTI:
		case DIS_MODE_MULTI_PWM: {
			if (enc->detent) {
				if (current_led_index < 6) {
					leds.state |= (LEFT_INDICATORS_MASK >> (current_led_index - 1)) &
												LEFT_INDICATORS_MASK;
				} else if (current_led_index > 6) {
					leds.state |= (LEFT_INDICATORS_MASK >> (current_led_index - 5)) &
												RIGHT_INDICATORS_MASK;
				}
			} else {
				leds.state = MASK_INDICATORS & ~(0xFFFF >> current_led_index);
			}
			break;
		}
		default: return ERR_BAD_PARAM;
	}

	if (enc->detent && (current_led_index == 6)) {
		leds.indicator_6 = 0;
	}

	for (unsigned int f = 0; f < NUM_PWM_FRAMES; ++f) {
		if (max_brightness < f) {
			leds.state								 = 0;
			gFRAME_BUFFER[f][enc->idx] = ~leds.state;
			continue;
		}

		if (enc->display.mode == DIS_MODE_MULTI_PWM) {
			u8 pwm_shift_index = current_led_index;

			if (enc->detent && current_led_index < 6) {
				pwm_shift_index = (current_led_index > 0) ? (current_led_index - 1) : 0;
			}

			u16	 pwm_mask = (0x8000 >> pwm_shift_index) & MASK_PWM_INDICATORS;
			bool brighten = (f < max_frames);

			if (enc->detent && current_led_index < 6) {
				brighten = !brighten;
			}

			if (brighten) {
				leds.state |= pwm_mask;
			} else {
				leds.state &= ~pwm_mask;
			}
		}

		// Handle RGB LEDs
		struct virtmap* vmap = &enc->vmaps[enc->vmap_active];

		leds.rgb_blue		 = 0;
		leds.rgb_red		 = 0;
		leds.rgb_green	 = 0;
		leds.detent_blue = 0;
		leds.detent_red	 = 0;

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

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*

static void display_update(input_dev_encoder_s* dev) {
	assert(dev);

	// This function remains commented out and still contains float operations.
	// It would need similar integer-based conversion if uncommented and used.

	struct quadrature*		enc = (struct quadrature*)dev->ctx;
	mf_display_s* dis = &enc_dis[dev->idx];

	f32						ind_pwm;		// Partial encoder positions (e.g
position = 5.7) uint					ind_norm;		// Integer encoder
positions (e.g position = 5) uint					pwm_frames; // Number of PWM
frames for partial brightness encoder_led_s leds;				// LED states

	// Clear all LEDs
	leds.state = 0;

	// Determine the position of the indicator led and which led requires
	// partial brightness
	if (curr_val <= led_interval) {
		ind_pwm	 = 0;
		ind_norm = 0;
	} else if (curr_val == ENC_MAX) {
		ind_pwm	 = NUM_INDICATOR_LEDS;
		ind_norm = NUM_INDICATOR_LEDS;
	} else {
		ind_pwm	 = ((f32)curr_val / led_interval);
		ind_norm = (unsigned int)roundf(ind_pwm);
	}

	// Generate the LED states based on the display mode
	switch (dis->led_style) {
		case DIS_MODE_SINGLE: {
			// Set the corresponding bit for the indicator led
			leds.state = MASK_INDICATORS & (0x8000 >> (ind_norm - 1));
			break;
		}

		case DIS_MODE_MULTI: {
			if (enc->enc_ctx.detent) {
				if (ind_norm < 6) {
					leds.state |=
							(LEFT_INDICATORS_MASK >> (ind_norm - 1)) &
LEFT_INDICATORS_MASK; } else if (ind_norm > 6) { leds.state |=
							(LEFT_INDICATORS_MASK >> (ind_norm - 5)) &
RIGHT_INDICATORS_MASK;
				}
			} else {
				// Default mode - set the indicator leds upto "led_index"
				leds.state = MASK_INDICATORS & ~(0xFFFF >> ind_norm);
			}
			break;
		}

		case DIS_MODE_MULTI_PWM: {
			f32 diff	 = ind_pwm - (floorf(ind_pwm));
			pwm_frames = (unsigned int)((diff)*NUM_PWM_FRAMES);
			if (enc->enc_ctx.detent) {
				if (ind_norm < 6) {
					leds.state |=
							(LEFT_INDICATORS_MASK >> (ind_norm - 1)) &
LEFT_INDICATORS_MASK; } else if (ind_norm > 6) { leds.state |=
							(LEFT_INDICATORS_MASK >> (ind_norm - 5)) &
RIGHT_INDICATORS_MASK;
				}
			} else {
				// Default mode - set the indicator leds upto "led_index"
				leds.state = MASK_INDICATORS & ~(0xFFFF >> ind_norm);
			}
			break;
		}

		default: return;
	}

	// When detent mode is enabled the detent LEDs must be displayed, and
	// indicator LED #6 at the 12 o'clock position must be turned off.
	if (enc->enc_ctx.detent && (ind_norm == 6)) {
		leds.indicator_6 = 0;
		leds.detent_blue = 1;
	}

	// Handle PWM for RGB colours, MULTI_PWM mode, and global max brightness
	for (unsigned int p = 0; p < NUM_PWM_FRAMES; ++p) {
		// When the brightness is below 100% then begin to dim the LEDs.
		if (max_brightness < p) {
			leds.state								= 0;
			gFRAME_BUFFER[p][dev->idx] = ~leds.state;
			continue;
		}

		if (dis->led_style == DIS_MODE_MULTI_PWM) {
			if (enc->enc_ctx.detent) {
				if (ind_norm < 6) {
					if (p < pwm_frames) {
						leds.state &=
								~((0x8000 >> (int)(ind_pwm - 1)) &
MASK_PWM_INDICATORS); } else { leds.state |=
								((0x8000 >> (int)(ind_pwm - 1)) &
MASK_PWM_INDICATORS);
					}

				} else if (ind_norm > 6) {
					if (p < pwm_frames) {
						leds.state |= ((0x8000 >> (int)ind_pwm) &
MASK_PWM_INDICATORS); } else { leds.state &= ~((0x8000 >> (int)ind_pwm) &
MASK_PWM_INDICATORS);
					}
				}
			} else {
				if (p < pwm_frames) {
					leds.state |= ((0x8000 >> (int)ind_pwm) &
MASK_PWM_INDICATORS); } else { leds.state &= ~((0x8000 >> (int)ind_pwm) &
MASK_PWM_INDICATORS);
				}
			}
		}

		leds.rgb_blue = leds.rgb_red = leds.rgb_green = 0;
		if ((int)(dis->led_rgb.red - p) > 0) {
			leds.rgb_red = 1;
		}
		if ((int)(dis->led_rgb.green - p) > 0) {
			leds.rgb_green = 1;
		}
		if ((int)(dis->led_rgb.blue - p) > 0) {
			leds.rgb_blue = 1;
		}
		gFRAME_BUFFER[p][dev->idx] = ~leds.state;
	}
}

void mf_debug_encoder_set_indicator(u8 indicator, u8 state) {
	assert(indicator < NUM_INDICATOR_LEDS);

	const u8			debug_encoder = 0;
	encoder_led_s leds;
	leds.state = 0; // turn off all leds

	switch (indicator) {
		case 0: leds.indicator_1 = state; break;
		case 1: leds.indicator_2 = state; break;
		case 2: leds.indicator_3 = state; break;
		case 3: leds.indicator_4 = state; break;
		case 4: leds.indicator_5 = state; break;
		case 5: leds.indicator_6 = state; break;
		case 6: leds.indicator_7 = state; break;
		case 7: leds.indicator_8 = state; break;
		case 8: leds.indicator_9 = state; break;
		case 9: leds.indicator_10 = state; break;
		case 10: leds.indicator_11 = state; break;
	}

	for (int i = 0; i < NUM_PWM_FRAMES; ++i) {
		gFRAME_BUFFER[i][debug_encoder] = ~leds.state;
	}
}

void mf_debug_encoder_set_rgb(bool red, bool green, bool blue) {
	const u8			debug_encoder = 0;
	encoder_led_s leds;
	leds.state		 = 0;
	leds.rgb_blue	 = blue;
	leds.rgb_red	 = red;
	leds.rgb_green = green;

	for (int i = 0; i < NUM_PWM_FRAMES; ++i) {
		gFRAME_BUFFER[i][debug_encoder] |= ~leds.state;
	}
}

*/
