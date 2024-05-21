/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "sys/types.h"
#include "sys/rgb.h"
#include "input/quadrature.h"
#include "input/encoder.h"
#include "input/switch.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define MF_NUM_ENCODERS							 (16)
#define MF_NUM_ENCODER_SWITCHES			 (MF_NUM_ENCODERS)
#define MF_NUM_SIDE_SWITCHES				 (6)
#define MF_NUM_LEDS									 (256)
#define MF_NUM_LEDS_PER_ENCODER			 (16)
#define MF_NUM_INDICATOR_LEDS				 (11)
#define MF_NUM_LED_SHIFT_REGISTERS	 (32)
#define MF_NUM_INPUT_SHIFT_REGISTERS (6)
#define MF_NUM_PWM_FRAMES						 (32)

#define MF_MAX_BRIGHTNESS						 (MF_NUM_PWM_FRAMES)
#define MF_MIN_BRIGHTNESS						 (1)

#define MF_NUM_ENC_BANKS						 (3)
#define MF_NUM_ENC_PER_BANK					 (MF_NUM_ENCODERS)
#define MF_NUM_VMAPS_PER_ENC				 (2)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

extern volatile u16 mf_frame_buf[MF_NUM_PWM_FRAMES][MF_NUM_ENCODERS];
extern quadrature_s mf_enc_quad[MF_NUM_ENCODER_SWITCHES];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
	DIS_MODE_SINGLE,
	DIS_MODE_MULTI,
	DIS_MODE_MULTI_PWM,

	DIS_MODE_NB,
} display_mode_e;

typedef enum {
	SW_MODE_NONE,

	SW_MODE_VMAP_CYCLE,
	SW_MODE_VMAP_HOLD,

	SW_MODE_RESET_ON_PRESS,
	SW_MODE_RESET_ON_RELEASE,

	SW_MODE_FINE_ADJUST_TOGGLE,
	SW_MODE_FINE_ADJUST_HOLD,

	SW_MODE_MIDI,
} switch_mode_e;

typedef struct {
	// Hardware index (0 to 15)
	u8 idx;

	// Display Configuration
	struct {
		display_mode_e				 mode;
		virtmap_display_mode_e virtmode;
	} display;

	// Encoder
	bool					detent;
	encoder_s			enc_ctx;
	quadrature_s* quad_ctx;

	struct {
		virtmap_s*		 head;
		virtmap_mode_e mode;
	} virtmap;

	// Encoder Switch
	switch_state_e sw_state;
	switch_mode_e	 sw_mode;
	proto_cfg_s		 sw_cfg;

} mf_encoder_s;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void hw_led_init(void);

void hw_encoder_init(void);
void hw_encoder_scan(void);

void					 hw_switch_init(void);
void					 hw_switch_update(void);
switch_state_e hw_enc_switch_state(uint idx);

void mf_input_init(void);
void mf_input_update(void);

int mf_display_init(void);
int mf_draw_encoder(mf_encoder_s* enc);

void mf_debug_encoder_set_indicator(u8 indicator, u8 state);
void mf_debug_encoder_set_rgb(bool red, bool green, bool blue);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
