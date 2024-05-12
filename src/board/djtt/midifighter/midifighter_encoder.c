/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <math.h>

#include "core/core_types.h"
#include "core/core_switch.h"
#include "core/core_rgb.h"
#include "core/core_error.h"

#include "encoder/encoder_virtual.h"

#include "event/event.h"
#include "event/events_io.h"
#include "event/events_midi.h"

#include "hal/avr/xmega/128a4u/gpio.h"

#include "board/djtt/midifighter.h"
#include "board/djtt/midifighter_colours.h"
#include "board/djtt/midifighter_encoder.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PORT_SR_ENC						 (PORTC) // IO port for encoder IO shift registers

#define PIN_SR_ENC_LATCH			 (0) // 74HC595N
#define PIN_SR_ENC_CLOCK			 (1)
#define PIN_SR_ENC_DATA_IN		 (2)

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

static void display_update(midifighter_encoder_s* enc);

static void evt_handler_io(void* event);
static void evt_handler_midi(void* event);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static const u16 led_interval = ENC_MAX / 11;

#warning "TODO(ns) Move this to a system-config struct..."
static u8 max_brightness = MF_MAX_BRIGHTNESS;

EVT_HANDLER(0, io_evt_handler, evt_handler_io);
EVT_HANDLER(0, midi_in_evt_handler, evt_handler_midi);

// PROGMEM static const midifighter_encoder_s default_config = {
// 		.enabled					= true,
// 		.detent						= false,
// 		.encoder_ctx			= {0},
// 		.hwenc_id					= 0,
// 		.led_style				= LED_STYLE_SINGLE,
// 		.led_detent.value = 0,
// 		.led_rgb.value		= 0,
// 		.midi.channel			= 0,
// 		.midi.mode				= MIDI_MODE_CC,
// 		.midi.data.cc			= 0,
// };

// A single hardware context is required for each physical encoder
static midifighter_encoder_s encoders[MF_NUM_ENCODERS];
static vencoder_ctx_s				 vencoders[MF_NUM_ENCODER_BANKS][MF_NUM_ENCODERS]
															 [MF_NUM_VENC_PER_ENCODER];

// A single switch context is required for all 16 switches (in the encoders)
static switch_x16_ctx_s switch_ctx;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void mf_encoder_init(void) {
	// Configure GPIO for encoder IO shift regsiters
	gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_LATCH, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN, GPIO_INPUT);

	// Latch initial encoder data
	gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 0);
	gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 1);

	int ret = event_channel_subscribe(EVENT_CHANNEL_IO, &io_evt_handler);
	// RETURN_ON_ERR(ret);

	ret = event_channel_subscribe(EVENT_CHANNEL_MIDI_IN, &midi_in_evt_handler);
	// RETURN_ON_ERR(ret);

	for (uint i = 0; i < COUNTOF(encoders); i++) {
		ret = encoder_init(&encoders[i].ctx, i);
		// RETURN_ON_ERR(ret);
	}

	for (uint b = 0; b < MF_NUM_ENCODER_BANKS; b++) {
		for (uint e = 0; e < MF_NUM_ENCODERS; e++) {
			for (uint v = 0; v < MF_NUM_VENC_PER_ENCODER; v++) {
				ret = vencoder_init(&vencoders[b][e][v], &encoders[0].ctx);
			}
		}
	}
}

// Scan the hardware state of the midifighter and update local contexts
void mf_encoder_update(void) {

	// Latch the IO levels into the shift registers
	gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 1);

	// Clock the 16 data bits for the encoder switches
	u16 swstates = 0;
	for (size_t i = 0; i < MF_NUM_ENCODER_SWITCHES; i++) {
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);
		u8 state = !(bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
		swstates |= (state << i);
	}

	// Clock the 32 bits for the 2x16 quadrature encoder signals, and update
	// encoder state.
	for (int i = 0; i < MF_NUM_ENCODERS; ++i) {
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
		u8 ch_a = (bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
		u8 ch_b = (bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);

		encoder_update(&encoders[i].ctx, ch_a, ch_b);
	}

	// Close the door!
	gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 0);

	// Execute the debounce and update routine for the switches
	switch_x16_update(&switch_ctx, swstates);
	switch_x16_debounce(&switch_ctx);
}

void mf_debug_encoder_set_indicator(u8 indicator, u8 state) {
	const u8 debug_encoder = 0;
	assert(indicator < MF_NUM_INDICATOR_LEDS);
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

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// static void virtual_encoder_update(midifighter_encoder_s* enc) {
// 	if (!enc->enabled) {
// 		return;
// 	}

// 	// Step 1 - handler encoder rotation
// 	u8 id = enc->hwenc_id;

// 	int direction = 0;
// 	if (hw_ctx[id].dir == DIR_CW) {
// 		direction = 1;
// 	} else if (hw_ctx[id].dir == DIR_CCW) {
// 		direction = -1;
// 	}

// 	core_encoder_update(&enc->encoder_ctx, direction);
// 	if (enc->encoder_ctx.curr_val != enc->encoder_ctx.prev_val) {
// 		// Post the rotation event to the IO event channel
// 		event_io_s evt;
// 		evt.event_id								= EVT_IO_ENCODER_ROTATION;
// 		evt.data.enc_rotation.index = enc->hwenc_id;
// 		evt.data.enc_rotation.value = enc->encoder_ctx.curr_val;
// 		event_post(EVENT_CHANNEL_IO, &evt);

// 		// Post a MIDI event if the encoder is configured for MIDI
// 		switch (enc->midi.mode) {
// 			case MIDI_MODE_CC: {
// 				// Convert 16-bit encoder value to 7-bit MIDI value
// 				u8 val = (u8)(((f32)enc->encoder_ctx.curr_val / ENC_MAX) * 127);

// 				if (enc->midi.prev_val != val) {
// 					midi_event_s midi_evt;
// 					midi_evt.event_id				 = MIDI_EVENT_CC;
// 					midi_evt.data.cc.channel = enc->midi.channel;
// 					midi_evt.data.cc.control = enc->midi.data.cc;
// 					midi_evt.data.cc.value	 = val;
// 					enc->midi.prev_val			 = val;
// 					event_post(EVENT_CHANNEL_MIDI_OUT, &midi_evt);
// 				}
// 				break;
// 			}

// 				// case MIDI_MODE_NOTE: {
// 				// 	midi_event_s midi_evt;
// 				// 	midi_evt.event_id						= MIDI_EVENT_NOTE;
// 				// 	midi_evt.data.note.channel	= enc->midi.channel;
// 				// 	midi_evt.data.note.note			= enc->hwenc_id;
// 				// 	midi_evt.data.note.velocity = enc->encoder_ctx.curr_val;
// 				// 	event_post(EVENT_CHANNEL_MIDI_OUT, &midi_evt);
// 				// 	break;
// 				// }

// 			default: break;
// 		}
// 	}

// 	// Step 2 - Handle encoder switch
// 	u16 mask			= (1u << id);
// 	u8	prevstate = (switch_ctx.previous & mask) ? 1 : 0;
// 	u8	state			= (switch_ctx.current & mask) ? 1 : 0;
// 	if (state != prevstate) {
// 		event_io_s evt;
// 		evt.event_id							= EVT_IO_ENCODER_SWITCH;
// 		evt.data.enc_switch.index = enc->hwenc_id;
// 		evt.data.enc_switch.value = state;
// 		event_post(EVENT_CHANNEL_IO, &evt);
// 	}
// }

static void display_update(midifighter_encoder_s* enc) {
	f32						ind_pwm;		// Partial encoder positions (e.g position = 5.7)
	unsigned int	ind_norm;		// Integer encoder positions (e.g position = 5)
	unsigned int	pwm_frames; // Number of PWM frames for partial brightness
	encoder_led_s leds;				// LED states

	// Clear all LEDs
	leds.state = 0;

	// Determine the position of the indicator led and which led requires
	// partial brightness
	if (enc->ctx.curr_val <= led_interval) {
		ind_pwm	 = 0;
		ind_norm = 0;
	} else if (enc->ctx.curr_val == ENC_MAX) {
		ind_pwm	 = MF_NUM_INDICATOR_LEDS;
		ind_norm = MF_NUM_INDICATOR_LEDS;
	} else {
		ind_pwm	 = ((f32)enc->ctx.curr_val / led_interval);
		ind_norm = (unsigned int)roundf(ind_pwm);
	}

	// Generate the LED states based on the display mode
	switch (enc->led_style) {
		case LED_STYLE_SINGLE: {
			// Set the corresponding bit for the indicator led
			leds.state = MASK_INDICATORS & (0x8000 >> (ind_norm - 1));
			break;
		}

		case LED_STYLE_MULTI: {
			if (enc->ctx.detent) {
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
			if (enc->ctx.detent) {
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
	if (enc->ctx.detent && (ind_norm == 6)) {
		leds.indicator_6 = 0;
		leds.detent_blue = 1;
	}

	// Handle PWM for RGB colours, MULTI_PWM mode, and global max brightness
	for (unsigned int p = 0; p < MF_NUM_PWM_FRAMES; ++p) {
		// When the brightness is below 100% then begin to dim the LEDs.
		if (max_brightness < p) {
			leds.state											= 0;
			mf_frame_buf[p][enc->ctx.index] = ~leds.state;
			continue;
		}

		if (enc->led_style == LED_STYLE_MULTI_PWM) {
			if (enc->ctx.detent) {
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
		if ((int)(enc->led_rgb.red - p) > 0) {
			leds.rgb_red = 1;
		}
		if ((int)(enc->led_rgb.green - p) > 0) {
			leds.rgb_green = 1;
		}
		if ((int)(enc->led_rgb.blue - p) > 0) {
			leds.rgb_blue = 1;
		}
		mf_frame_buf[p][enc->ctx.index] = ~leds.state;
	}
}

static void evt_handler_io(void* event) {
	assert(event);

	event_io_s* e = (event_io_s*)event;
	switch (e->event_id) {
		case EVT_IO_ENCODER_ROTATION: {

			display_update(&encoders[e->data.enc_rotation.index]);
			break;
		}

			// case EVT_CORE_ENCODER_SWITCH_STATE: {
			// 	u8 index							 = event->data.sw.switch_index;
			// 	mf_ctx[index].led_mode = (mf_ctx[index].led_mode + 1) % LED_STYLE_NB;
			// 	update_display(index);
			// break;
			// }

			// case EVE: {
			// 	max_brightness = event->data.max_brightness;
			// 	break;
			// }
	}
}

static void evt_handler_midi(void* event) {
	// assert(event);

	// midi_event_s* e = (midi_event_s*)event;

	// switch (e->event_id) {
	// 	case MIDI_EVENT_CC: {
	// 		midi_cc_event_s* cc = &e->data.cc;
	// 		for (int i = 0; i < MF_NUM_ENCODERS; ++i) {
	// 			if (mf_ctx[i].midi.mode == MIDI_MODE_CC &&
	// 					mf_ctx[i].midi.channel == cc->channel &&
	// 					mf_ctx[i].midi.data.cc == cc->control) {
	// 				mf_ctx[i].encoder_ctx.curr_val =
	// 						(u16)(((f32)cc->value / 127) * ENC_MAX);
	// 				display_update(&mf_ctx[i]);
	// 				break;
	// 			}
	// 		}
	// 		break;
	// 	}

	// 	default: return;
	// }
}
