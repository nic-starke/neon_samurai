/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "hal/sys.h"
#include "system/types.h"
#include "system/config.h"
#include "io/quadrature.h"
#include "io/encoder.h"
#include "io/switch.h"

#include "virtmap/virtmap.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define NUM_ENCODERS							(16)
#define NUM_ENCODER_SWITCHES			(NUM_ENCODERS)
#define NUM_SIDE_SWITCHES					(6)
#define NUM_LEDS									(256)
#define NUM_LEDS_PER_ENCODER			(16)
#define NUM_INDICATOR_LEDS				(11)
#define NUM_LED_SHIFT_REGISTERS		(32)
#define NUM_INPUT_SHIFT_REGISTERS (6)
#define NUM_PWM_FRAMES						(32)

#define MAX_BRIGHTNESS						(NUM_PWM_FRAMES)
#define MIN_BRIGHTNESS						(1)

#define NUM_ENC_BANKS							(3)
#define NUM_ENC_PER_BANK					(NUM_ENCODERS)
#define NUM_VMAPS_PER_ENC					(2)

#define RGB_WHITE									(0x32DF) // red = max, blue = 12, green = 22
#define RGB_MAX_VAL								(NUM_PWM_FRAMES)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum display_mode {
	DIS_MODE_SINGLE,
	DIS_MODE_MULTI,
	DIS_MODE_MULTI_PWM,

	DIS_MODE_NB,
};

enum switch_mode {
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
};

struct mf_encoder {
	// Hardware index (0 to 15)
	u8 idx;

	// Display Configuration
	struct {
		enum display_mode				 mode;
		enum virtmap_display_mode virtmode;
	} display;

	// Encoder
	bool					detent;
	struct encoder			enc_ctx;
	struct quadrature* quad_ctx;

	// Virtual Mappings
	enum virtmap_mode vmap_mode;
	u8						 vmap_active; // Index for the current active vmap
	struct virtmap			 vmaps[NUM_VMAPS_PER_ENC];

	// Encoder Switch
	enum switch_state sw_state;
	enum switch_mode	 sw_mode;
	struct proto_cfg		 sw_cfg;

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
};

/**
 * @brief Runtime data structure for the midifighter global variables.
 */
struct mf_rt {
	u8 curr_bank;
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

extern volatile u16 gFRAME_BUFFER[NUM_PWM_FRAMES][NUM_ENCODERS];
extern struct quadrature gQUAD_ENC[NUM_ENCODER_SWITCHES];
extern struct mf_encoder gENCODERS[NUM_ENC_BANKS][NUM_ENCODERS];
extern struct mf_rt			gRT;
extern struct sys_config gCONFIG;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void hw_led_init(void);

void hw_encoder_init(void);
void hw_encoder_scan(void);

void					 hw_switch_init(void);
void					 hw_switch_update(void);
enum switch_state hw_enc_switch_state(u8 idx);

void mf_input_init(void);
void mf_input_update(void);
bool mf_is_reset_pressed(void);

int mf_display_init(void);
int mf_draw_encoder(struct mf_encoder* enc);

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
int cfg_init(bool reset_cfg);
int cfg_load(void);
int cfg_store(void);
int cfg_update(void);
int mf_cfg_reset(void);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
