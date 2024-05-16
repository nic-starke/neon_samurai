/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <math.h>

#include "board/djtt/midifighter.h"
#include "board/djtt/midifighter_display.h"

#include "event/event.h"
#include "event/events_io.h"
#include "event/events_midi.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define MASK_INDICATORS				 (0xFFE0)
#define LEFT_INDICATORS_MASK	 (0xF800)
#define RIGHT_INDICATORS_MASK	 (0x03E0)
#define CLEAR_LEFT_INDICATORS	 (0x03FF) // Mask to clear mid and left-side leds
#define CLEAR_RIGHT_INDICATORS (0xF80F) // Mask to clear mid and right-side leds
#define MASK_PWM_INDICATORS		 (0xFBE0)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
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

static void display_update(iodev_s* dev);
static void draw_virtmap(iodev_s* dev);
static int	evt_handler_io(void* event);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

EVT_HANDLER(2, io_evt_handler, evt_handler_io);

static const u16 led_interval = ENC_MAX / 11;
#warning "TODO(ns) Move this to a system-config struct..."
static u8						max_brightness = MF_MAX_BRIGHTNESS;
static mf_display_s enc_dis[MF_NUM_ENCODERS];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int mf_display_init(void) {
	// Configure event channels
	int ret = event_channel_subscribe(EVENT_CHANNEL_IO, &io_evt_handler);
	RETURN_ON_ERR(ret);

	return 0;
}

void mf_display_draw_encoder(iodev_s* dev) {
	assert(dev);
	draw_virtmap(dev);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void draw_virtmap(iodev_s* dev) {
	assert(dev);

	virtmap_s* p = dev->vmap;

	u16 p_val = convert_range(p->curr_value, p->range.lower, p->range.upper,
														p->position.start, p->position.stop);

	encoder_led_s leds = {0};

	for (uint frame = 0; frame < MF_NUM_PWM_FRAMES; frame++) {
		leds.state = 0;
		leds.state = MASK_INDICATORS & (0x8000 >> ((p_val / led_interval)));
		// leds.state |= MASK_INDICATORS & (0x8000 >> ((ctx->curr_val /
		// led_interval)));

		virtmap_s* s = p->next;
		while (s != NULL) {
			if (frame < MF_NUM_PWM_FRAMES / 3) {
				u16 s_val = convert_range(s->curr_value, s->range.lower, s->range.upper,
																	s->position.start, s->position.stop);
				leds.state |= MASK_INDICATORS & (0x8000 >> ((s_val / led_interval)));
			}
			s = s->next;
		}
		mf_frame_buf[frame][dev->idx] = ~leds.state;
	}
}

static int evt_handler_io(void* event) {
	assert(event);

	io_event_s* e = (io_event_s*)event;
	switch (e->type) {
		case EVT_IO_ENCODER_ROTATION: {
			// display_update(e->dev);
			draw_virtmap(e->dev);
			break;
		}
	}

	return 0;
}

/*

static void display_update(iodev_s* dev) {
	assert(dev);

	encoder_s*		enc = (encoder_s*)dev->ctx;
	mf_display_s* dis = &enc_dis[dev->idx];

	f32						ind_pwm;		// Partial encoder positions (e.g position = 5.7)
	uint					ind_norm;		// Integer encoder positions (e.g position = 5)
	uint					pwm_frames; // Number of PWM frames for partial brightness
	encoder_led_s leds;				// LED states

	// Clear all LEDs
	leds.state = 0;

	// Determine the position of the indicator led and which led requires
	// partial brightness
	if (enc->curr_val <= led_interval) {
		ind_pwm	 = 0;
		ind_norm = 0;
	} else if (enc->curr_val == ENC_MAX) {
		ind_pwm	 = MF_NUM_INDICATOR_LEDS;
		ind_norm = MF_NUM_INDICATOR_LEDS;
	} else {
		ind_pwm	 = ((f32)enc->curr_val / led_interval);
		ind_norm = (unsigned int)roundf(ind_pwm);
	}

	// Generate the LED states based on the display mode
	switch (dis->led_style) {
		case LED_STYLE_SINGLE: {
			// Set the corresponding bit for the indicator led
			leds.state = MASK_INDICATORS & (0x8000 >> (ind_norm - 1));
			break;
		}

		case LED_STYLE_MULTI: {
			if (enc->detent) {
				if (ind_norm < 6) {
					leds.state |=
							(LEFT_INDICATORS_MASK >> (ind_norm - 1)) & LEFT_INDICATORS_MASK;
				} else if (ind_norm > 6) {
					leds.state |=
							(LEFT_INDICATORS_MASK >> (ind_norm - 5)) & RIGHT_INDICATORS_MASK;
				}
			} else {
				// Default mode - set the indicator leds upto "led_index"
				leds.state = MASK_INDICATORS & ~(0xFFFF >> ind_norm);
			}
			break;
		}

		case LED_STYLE_MULTI_PWM: {
			f32 diff	 = ind_pwm - (floorf(ind_pwm));
			pwm_frames = (unsigned int)((diff)*MF_NUM_PWM_FRAMES);
			if (enc->detent) {
				if (ind_norm < 6) {
					leds.state |=
							(LEFT_INDICATORS_MASK >> (ind_norm - 1)) & LEFT_INDICATORS_MASK;
				} else if (ind_norm > 6) {
					leds.state |=
							(LEFT_INDICATORS_MASK >> (ind_norm - 5)) & RIGHT_INDICATORS_MASK;
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
	if (enc->detent && (ind_norm == 6)) {
		leds.indicator_6 = 0;
		leds.detent_blue = 1;
	}

	// Handle PWM for RGB colours, MULTI_PWM mode, and global max brightness
	for (unsigned int p = 0; p < MF_NUM_PWM_FRAMES; ++p) {
		// When the brightness is below 100% then begin to dim the LEDs.
		if (max_brightness < p) {
			leds.state								= 0;
			mf_frame_buf[p][dev->idx] = ~leds.state;
			continue;
		}

		if (dis->led_style == LED_STYLE_MULTI_PWM) {
			if (enc->detent) {
				if (ind_norm < 6) {
					if (p < pwm_frames) {
						leds.state &=
								~((0x8000 >> (int)(ind_pwm - 1)) & MASK_PWM_INDICATORS);
					} else {
						leds.state |=
								((0x8000 >> (int)(ind_pwm - 1)) & MASK_PWM_INDICATORS);
					}

				} else if (ind_norm > 6) {
					if (p < pwm_frames) {
						leds.state |= ((0x8000 >> (int)ind_pwm) & MASK_PWM_INDICATORS);
					} else {
						leds.state &= ~((0x8000 >> (int)ind_pwm) & MASK_PWM_INDICATORS);
					}
				}
			} else {
				if (p < pwm_frames) {
					leds.state |= ((0x8000 >> (int)ind_pwm) & MASK_PWM_INDICATORS);
				} else {
					leds.state &= ~((0x8000 >> (int)ind_pwm) & MASK_PWM_INDICATORS);
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
		mf_frame_buf[p][dev->idx] = ~leds.state;
	}
}

void mf_debug_encoder_set_indicator(u8 indicator, u8 state) {
	assert(indicator < MF_NUM_INDICATOR_LEDS);

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

	for (int i = 0; i < MF_NUM_PWM_FRAMES; ++i) {
		mf_frame_buf[i][debug_encoder] = ~leds.state;
	}
}

void mf_debug_encoder_set_rgb(bool red, bool green, bool blue) {
	const u8			debug_encoder = 0;
	encoder_led_s leds;
	leds.state		 = 0;
	leds.rgb_blue	 = blue;
	leds.rgb_red	 = red;
	leds.rgb_green = green;

	for (int i = 0; i < MF_NUM_PWM_FRAMES; ++i) {
		mf_frame_buf[i][debug_encoder] |= ~leds.state;
	}
}

*/
