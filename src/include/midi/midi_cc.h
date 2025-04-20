/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define MIDI_CC_MIN				(MIDI_CC_MSB_BANK)
#define MIDI_CC_MAX				(MIDI_CC_MONO2)
#define MIDI_CC_RANGE			(MIDI_CC_MAX - MIDI_CC_MIN)

#define MIDI_CC_14B_MIN		(0)
#define MIDI_CC_14B_MAX		(0x3FFF)
#define MIDI_CC_14B_RANGE (MIDI_CC_14B_MAX - MIDI_CC_14B_MIN)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum midi_cc {
	MIDI_CC_MSB_BANK						 = 0x00,
	MIDI_CC_MSB_MODWHEEL				 = 0x01,
	MIDI_CC_MSB_BREATH					 = 0x02,
	MIDI_CC_MSB_FOOT						 = 0x04,
	MIDI_CC_MSB_PORTAMENTO_TIME	 = 0x05,
	MIDI_CC_MSB_DATA_ENTRY			 = 0x06,
	MIDI_CC_MSB_MAIN_VOLUME			 = 0x07,
	MIDI_CC_MSB_BALANCE					 = 0x08,
	MIDI_CC_MSB_PAN							 = 0x0a,
	MIDI_CC_MSB_EXPRESSION			 = 0x0b,
	MIDI_CC_MSB_EFFECT1					 = 0x0c,
	MIDI_CC_MSB_EFFECT2					 = 0x0d,
	MIDI_CC_MSB_GENERAL_PURPOSE1 = 0x10,
	MIDI_CC_MSB_GENERAL_PURPOSE2 = 0x11,
	MIDI_CC_MSB_GENERAL_PURPOSE3 = 0x12,
	MIDI_CC_MSB_GENERAL_PURPOSE4 = 0x13,
	MIDI_CC_LSB_BANK						 = 0x20,
	MIDI_CC_LSB_MODWHEEL				 = 0x21,
	MIDI_CC_LSB_BREATH					 = 0x22,
	MIDI_CC_LSB_FOOT						 = 0x24,
	MIDI_CC_LSB_PORTAMENTO_TIME	 = 0x25,
	MIDI_CC_LSB_DATA_ENTRY			 = 0x26,
	MIDI_CC_LSB_MAIN_VOLUME			 = 0x27,
	MIDI_CC_LSB_BALANCE					 = 0x28,
	MIDI_CC_LSB_PAN							 = 0x2a,
	MIDI_CC_LSB_EXPRESSION			 = 0x2b,
	MIDI_CC_LSB_EFFECT1					 = 0x2c,
	MIDI_CC_LSB_EFFECT2					 = 0x2d,
	MIDI_CC_LSB_GENERAL_PURPOSE1 = 0x30,
	MIDI_CC_LSB_GENERAL_PURPOSE2 = 0x31,
	MIDI_CC_LSB_GENERAL_PURPOSE3 = 0x32,
	MIDI_CC_LSB_GENERAL_PURPOSE4 = 0x33,
	MIDI_CC_SUSTAIN							 = 0x40,
	MIDI_CC_PORTAMENTO					 = 0x41,
	MIDI_CC_SOSTENUTO						 = 0x42,
	MIDI_CC_SOFT_PEDAL					 = 0x43,
	MIDI_CC_LEGATO_FOOTSWITCH		 = 0x44,
	MIDI_CC_HOLD2								 = 0x45,
	MIDI_CC_SC1_SOUND_VARIATION	 = 0x46,
	MIDI_CC_SC2_TIMBRE					 = 0x47,
	MIDI_CC_SC3_RELEASE_TIME		 = 0x48,
	MIDI_CC_SC4_ATTACK_TIME			 = 0x49,
	MIDI_CC_SC5_BRIGHTNESS			 = 0x4a,
	MIDI_CC_SC6									 = 0x4b,
	MIDI_CC_SC7									 = 0x4c,
	MIDI_CC_SC8									 = 0x4d,
	MIDI_CC_SC9									 = 0x4e,
	MIDI_CC_SC10								 = 0x4f,
	MIDI_CC_GENERAL_PURPOSE5		 = 0x50,
	MIDI_CC_GENERAL_PURPOSE6		 = 0x51,
	MIDI_CC_GENERAL_PURPOSE7		 = 0x52,
	MIDI_CC_GENERAL_PURPOSE8		 = 0x53,
	MIDI_CC_PORTAMENTO_CONTROL	 = 0x54,
	MIDI_CC_E1_REVERB_DEPTH			 = 0x5b,
	MIDI_CC_E2_TREMOLO_DEPTH		 = 0x5c,
	MIDI_CC_E3_CHORUS_DEPTH			 = 0x5d,
	MIDI_CC_E4_DETUNE_DEPTH			 = 0x5e,
	MIDI_CC_E5_PHASER_DEPTH			 = 0x5f,
	MIDI_CC_DATA_INCREMENT			 = 0x60,
	MIDI_CC_DATA_DECREMENT			 = 0x61,
	MIDI_CC_NONREG_PARM_NUM_LSB	 = 0x62,
	MIDI_CC_NONREG_PARM_NUM_MSB	 = 0x63,
	MIDI_CC_REGIST_PARM_NUM_LSB	 = 0x64,
	MIDI_CC_REGIST_PARM_NUM_MSB	 = 0x65,
	MIDI_CC_ALL_SOUNDS_OFF			 = 0x78,
	MIDI_CC_RESET_CONTROLLERS		 = 0x79,
	MIDI_CC_LOCAL_CONTROL_SWITCH = 0x7a,
	MIDI_CC_ALL_NOTES_OFF				 = 0x7b,
	MIDI_CC_OMNI_OFF						 = 0x7c,
	MIDI_CC_OMNI_ON							 = 0x7d,
	MIDI_CC_MONO1								 = 0x7e,
	MIDI_CC_MONO2								 = 0x7f,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
