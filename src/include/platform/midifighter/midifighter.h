/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "hal/avr/xmega/128a4u/sys.h"
#include "sys/types.h"
#include "sys/config.h"
#include "input/quadrature.h"
#include "input/encoder.h"
#include "input/switch.h"

#include "platform/midifighter/virtmap.h"

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

#define MF_RGB_WHITE								 (0x32DF) // red = max, blue = 12, green = 22
#define MF_RGB_MAX_VAL							 (MF_NUM_PWM_FRAMES)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
	DIS_MODE_SINGLE,
	DIS_MODE_MULTI,
	DIS_MODE_MULTI_PWM,

	DIS_MODE_NB,
} display_mode_e;

typedef enum {
	SW_MODE_NONE,

	// Cycle between virtual mappings on press
	SW_MODE_VMAP_CYCLE,

	// Alternative virtual mapping active while switch held
	SW_MODE_VMAP_HOLD,

	// Reset encoder value on press
	SW_MODE_RESET_ON_PRESS,

	// Reset encoder value on release
	SW_MODE_RESET_ON_RELEASE,

	// Toggle fine adjust mode on press
	SW_MODE_FINE_ADJUST_TOGGLE,

	// Fine adjust mode active while switch held
	SW_MODE_FINE_ADJUST_HOLD,

	// Switch actives a MIDI event (CC, Note, etc)
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

	// Virtual Mappings
	virtmap_mode_e vmap_mode;
	u8						 vmap_active; // Index for the current active vmap
	virtmap_s			 vmaps[MF_NUM_VMAPS_PER_ENC];

	// Encoder Switch
	switch_state_e sw_state;
	switch_mode_e	 sw_mode;
	proto_cfg_s		 sw_cfg;

	/*
		update_display is (as its name suggests) used to determine when to redraw
		the LEDs for this encoder. This is to prevent the display from being updated
		too frequently, and to allow for a smooth display update.
		It is effectively a timestamp (in ms) which can then be used to determine
		if the display should be updated.
		Normally the display only needs to be updated if the time delta between
		the last update and the current time is greater than a certain threshold.

		A value of 0 means the display is up-to-date (and can therefore be skipped
		by the update routine).
	*/
	u32 update_display;
} mf_encoder_s;

/**
 * @brief Runtime data structure for the midifighter global variables.
 */
typedef struct {
	u8 curr_bank;
} mf_rt_s;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

extern volatile u16 gFRAME_BUFFER[MF_NUM_PWM_FRAMES][MF_NUM_ENCODERS];
extern quadrature_s gQUAD_ENC[MF_NUM_ENCODER_SWITCHES];
extern mf_encoder_s gENCODERS[MF_NUM_ENC_BANKS][MF_NUM_ENCODERS];
extern mf_rt_s			gRT;
extern sys_config_s gCONFIG;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void hw_led_init(void);

void hw_encoder_init(void);
void hw_encoder_scan(void);

void					 hw_switch_init(void);
void					 hw_switch_update(void);
switch_state_e hw_enc_switch_state(u8 idx);

void mf_input_init(void);
void mf_input_update(void);
bool mf_is_reset_pressed(void);

int mf_display_init(void);
int mf_draw_encoder(mf_encoder_s* enc);

void mf_debug_encoder_set_indicator(u8 indicator, u8 state);
void mf_debug_encoder_set_rgb(bool red, bool green, bool blue);

/**
 * @brief Initialise the midifighter configuration data.
 * This will read/write to the EEPROM.
 * The function must be called AFTER all other initialisation functions,
 * to ensure that default configurations will be correctly written
 * the very first time a user boots the device.
 *
 * @return int 0 on success, !0 on failure.
 */
int mf_cfg_init(bool reset_cfg);
int mf_cfg_load(void);
int mf_cfg_store(void);
int mf_cfg_update(void);
int mf_cfg_reset(void);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
