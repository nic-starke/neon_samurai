#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/hardware.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define MF_SYSEX_MAX_PKT_SIZE	 (sizeof(mf_sysex_msg_s))
#define MF_SYSEX_MIN_PKT_SIZE	 (sizeof(mf_sysex_msg_s) - MF_SYSEX_MAX_DATA_SIZE)
#define MF_SYSEX_MAX_DATA_SIZE (sizeof(mf_sysex_param_s))

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum mf_sysex_cmd {
	MF_SYSEX_GET,
	MF_SYSEX_GET_RESPONSE,

	MF_SYSEX_SET,
	MF_SYSEX_SET_RESPONSE,

	MF_SYSEX_STOP,
};

enum mf_sysex_param {
	MF_SYSEX_PARAM_ENCODER_DETENT,
	MF_SYSEX_PARAM_ENCODER_DISPLAY_MODE,
	MF_SYSEX_PARAM_ENCODER_VMAP_DISPLAY_MODE,
	MF_SYSEX_PARAM_ENCODER_VMAP_MODE,
	MF_SYSEX_PARAM_ENCODER_VMAP_ACTIVE,

	MF_SYSEX_PARAM_ENCODER_SWITCH_STATE,
	MF_SYSEX_PARAM_ENCODER_SWITCH_MODE,
	MF_SYSEX_PARAM_ENCODER_SWITCH_PROTO,

	MF_SYSEX_PARAM_VMAP_RANGE,
	MF_SYSEX_PARAM_VMAP_POSITION,
	MF_SYSEX_PARAM_VMAP_RGB,
	MF_SYSEX_PARAM_VMAP_RB,
	MF_SYSEX_PARAM_VMAP_PROTO,

	MF_SYSEX_PARAM_SIDE_SWITCH,
	MF_SYSEX_PARAM_ACTIVE_BANK,

	// Read-only. Lets a host tool detect firmware version and hardware
	// capability (encoder/bank/vmap/side-switch counts) without hardcoding
	// them, and identify firmware predating this device-info query.
	MF_SYSEX_PARAM_DEVICE_INFO,

	MF_SYSEX_PARAM_NB,
};

// Payload for MF_SYSEX_PARAM_DEVICE_INFO GET responses.
typedef struct __attribute__((packed)) {
	u8 fw_version_major;
	u8 fw_version_minor;
	u8 fw_version_patch;
	u8 num_encoders;
	u8 num_banks;
	u8 num_vmaps_per_encoder;
	u8 num_side_switches;
} mf_sysex_device_info_s;

typedef struct __attribute__((packed)) {
	u8 mode;
	u8 channel;
	u8 data; // CC/Note etc..
} mf_sysex_midi_cfg_s;

typedef struct __attribute__((packed)) {
	u8 type;
	union {
		mf_sysex_midi_cfg_s midi;
	};
} mf_sysex_proto_cfg_s;

typedef struct __attribute__((packed)) {
	u8 bank_idx;
	u8 enc_idx;
	union {
		u8												detent;
		enum display_mode					display_mode;
		enum virtmap_display_mode vmap_display_mode;
		enum virtmap_mode					vmap_mode;
		u8												vmap_active;
	} data;
} mf_sysex_encoder_param_s;

typedef struct __attribute__((packed)) {
	u8 bank_idx;
	u8 enc_idx;
	union {
		u8 data; // placeholder
	};
} mf_sysex_switch_param_s;

typedef struct __attribute__((packed)) {
	u8 sw_idx;
	u8 data; // enum side_switch_mode
} mf_sysex_sideswitch_param_s;

typedef struct __attribute__((packed)) {
	u8 data; // Active bank index (0..NUM_ENC_BANKS-1)
} mf_sysex_bank_param_s;

typedef struct __attribute__((packed)) {
	u8 bank_idx;
	u8 enc_idx;
	u8 vmap_idx;
	union {
		struct {
			u8 lower;
			u8 upper;
		} range;
		struct {
			u8 start;
			u8 stop;
		} position;
		// u8 fields here, not u16: these mirror struct rgb_8/rb_8 (led/rgb.h)
		// exactly, which store gamma-corrected BCM brightness (0..NUM_PWM_FRAMES-1,
		// i.e. 0-31) per channel, not full 16-bit color.
		struct {
			u8 red;
			u8 green;
			u8 blue;
		} rgb;
		struct {
			u8 red;
			u8 blue;
		} rb;
	} data;
} mf_sysex_vmap_param_s;

typedef union {
	mf_sysex_encoder_param_s		enc;
	mf_sysex_sideswitch_param_s sw;
	mf_sysex_vmap_param_s				vmap;
	mf_sysex_bank_param_s				bank;
} mf_sysex_param_s;

typedef struct __attribute__((packed)) {
	u8							 mf_id[3];
	u8							 cmd;
	u8							 param_enum;
	mf_sysex_param_s param;
} mf_sysex_msg_s;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int mf_sysex_init(void);
