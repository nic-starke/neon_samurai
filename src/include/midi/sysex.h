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

// mf_id/cmd/param_enum + the largest param's data, 7-bit packed (see
// sysex_pack7()/sysex_unpack7() in sysex.c): every 7 raw data bytes need an
// extra header byte on the wire, so the packed form is larger than the
// unpacked struct it decodes to. mf_id/cmd/param_enum are never packed
// (always small enough to be legal 7-bit values as-is), only the data
// portion is.
#define MF_SYSEX_PACKED_MAX_DATA_SIZE                                          \
	(MF_SYSEX_MAX_DATA_SIZE + ((MF_SYSEX_MAX_DATA_SIZE + 6) / 7))
#define MF_SYSEX_PACKED_MAX_PKT_SIZE                                           \
	(MF_SYSEX_MIN_PKT_SIZE + MF_SYSEX_PACKED_MAX_DATA_SIZE)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum mf_sysex_cmd {
	MF_SYSEX_GET,
	MF_SYSEX_GET_RESPONSE,

	MF_SYSEX_SET,
	MF_SYSEX_SET_RESPONSE,

	MF_SYSEX_STOP,

	// Outbound-only: an unsolicited value, not a reply to any host
	// request. Used for curr_pos/vmap_active live pushes to a connected
	// web-ui client - same param_enum + data shape a GET_RESPONSE for
	// that param would carry, just tagged so a host can tell "you asked
	// for this" apart from "this happened on its own".
	MF_SYSEX_WEBUI_PUSH,
};

enum mf_sysex_param {
	MF_SYSEX_PARAM_ENCODER_DETENT,
	MF_SYSEX_PARAM_ENCODER_DISPLAY_MODE,
	MF_SYSEX_PARAM_ENCODER_VMAP_DISPLAY_MODE,
	MF_SYSEX_PARAM_ENCODER_VMAP_MODE,
	// Also pushed unsolicited (MF_SYSEX_WEBUI_PUSH) when a switch press
	// changes it - see set_vmap_active() in input_manager.c.
	MF_SYSEX_PARAM_ENCODER_VMAP_ACTIVE,

	MF_SYSEX_PARAM_ENCODER_SWITCH_STATE,
	MF_SYSEX_PARAM_ENCODER_SWITCH_MODE,
	MF_SYSEX_PARAM_ENCODER_SWITCH_PROTO,

	MF_SYSEX_PARAM_VMAP_RANGE,
	MF_SYSEX_PARAM_VMAP_POSITION,
	MF_SYSEX_PARAM_VMAP_RGB,
	MF_SYSEX_PARAM_VMAP_RB,
	MF_SYSEX_PARAM_VMAP_PROTO,
	// Writable HSV color - VMAP_RGB above is read-only/derived (computed
	// from this by color_update_vmap_rgb()); this is the actual way to set
	// an encoder's color over sysex. Previously only reachable via the CDC
	// console's `set_vmap_hsv` command, unusable from a Web MIDI client.
	MF_SYSEX_PARAM_VMAP_HSV,

	MF_SYSEX_PARAM_SIDE_SWITCH,
	MF_SYSEX_PARAM_ACTIVE_BANK,

	// Read-only. Lets a host tool detect firmware version and hardware
	// capability (encoder/bank/vmap/side-switch counts) without hardcoding
	// them, and identify firmware predating this device-info query.
	MF_SYSEX_PARAM_DEVICE_INFO,

	// Read-only, and also pushed *unsolicited* (MF_SYSEX_WEBUI_PUSH):
	// whenever an encoder is physically turned while streaming is enabled
	// (see MF_SYSEX_PARAM_ENCODER_LIVE_POSITION_STREAM below), without
	// having been asked. curr_pos (0-255, struct virtmap) has no other
	// sysex GET - VMAP_POSITION above is the *configured window* a colour
	// layer occupies, not the knob's live rotation. A host may still issue
	// an explicit GET too (e.g. to read the current value once on
	// connect, before any further movement); it behaves like any other
	// read-only param in that case.
	MF_SYSEX_PARAM_VMAP_CURR_POS,

	// Write-only trigger (u8 data: 0 = stop, nonzero = start). Controls
	// whether MF_SYSEX_PARAM_VMAP_CURR_POS is pushed unsolicited - off by
	// default (and forced off on every reboot/reset), so a host that never
	// asks for it sees zero extra sysex traffic. A connected web client
	// should enable this on connect and disable it on disconnect - see
	// gRT.live_position_streaming and vmap_update() in input_manager.c.
	MF_SYSEX_PARAM_ENCODER_LIVE_POSITION_STREAM,

	// Write-only triggers, no meaningful data payload (SET with any/no
	// data byte fires it; GET is rejected). Both reboot the device - the
	// SET ack is sent *before* the reboot actually happens, mirroring how
	// the CDC console's `reset`/`reset_cfg` commands and
	// EVT_SYS_REQ_CFG_RESET already work. Added so a host tool (e.g. a
	// test suite establishing a known starting state, or the web GUI's
	// "factory reset" action) doesn't need a second transport (the CDC
	// serial console) just to reboot or factory-reset the device.
	MF_SYSEX_PARAM_SYSTEM_RESET, // Soft reboot, config unchanged
	MF_SYSEX_PARAM_CONFIG_RESET, // Factory reset (wipes EEPROM) + reboot

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
			i16 lower;
			i16 upper;
		} range;
		struct {
			u8 start;
			u8 stop;
		} position;
		struct {
			u8 red;
			u8 green;
			u8 blue;
		} rgb;
		struct {
			u8 red;
			u8 blue;
		} rb;
		struct {
			u16 hue;				// 0-1535
			u8	saturation; // 0-255
			u8	value;			// 0-255
		} hsv;
		u8 curr_pos; // MF_SYSEX_PARAM_VMAP_CURR_POS - live knob position, 0-255
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

// Unsolicited pushes, gated by gRT.live_position_streaming. Called from
// webui_bridge.c.
void sysex_push_vmap_active(u8 bank, u8 enc, u8 active);
void sysex_push_active_bank(u8 bank);

// 7-to-8-bit sysex data packing - see the definitions in sysex.c for the
// full rationale. Exported for use by midi_lufa.c's TX path.
u8 sysex_pack7(const u8* unpacked, u8 unpacked_len, u8* packed);
u8 sysex_unpack7(const u8* packed, u8 packed_len, u8* unpacked);
