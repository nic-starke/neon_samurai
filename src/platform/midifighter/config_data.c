/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string.h>
#include <avr/eeprom.h>

#include "sys/error.h"
#include "sys/time.h"
#include "platform/midifighter/midifighter.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define EE_VERSION (u16)(11)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Data structure for eeprom storage using EEMEM flag

typedef struct {
	u8 channel : 4;
	u8 mode		 : 4;

	union {
		u8 cc;
		u8 raw;
	};

} mf_eeprom_midi_cfg_s;

typedef union {
	u8									 type;
	mf_eeprom_midi_cfg_s midi;
} mf_eeprom_proto_cfg_s;

typedef struct {
	struct {
		i8 lower;
		i8 upper;
	} range;

	struct {
		u8 start;
		u8 stop;
	} position;
} mf_eeprom_virtmap_cfg_s;

typedef struct {
	// General
	u8 display_mode								 : 2;
	u8 virtmap_mode								 : 1;

	// Encoder
	u8 detent											 : 1;
	u8 accel_mode									 : 1;
	u8 vmap_mode									 : 1;
	u8 vmap_active								 : 1;

	// Encoder Switch
	u8										sw_state : 1;
	u8										sw_mode;
	mf_eeprom_proto_cfg_s sw_cfg;

	struct {
		mf_eeprom_proto_cfg_s cfg;
		u8										pos;
		u8										rgb_r;
		u8										rgb_g;
		u8										rgb_b;
		u8										rb_r;
		u8										rb_b;
	} vmap[MF_NUM_VMAPS_PER_ENC];
} mf_eeprom_encoder_s;

typedef struct {
	u16									version;
	mf_eeprom_encoder_s encoders[MF_NUM_ENC_BANKS][MF_NUM_ENCODERS];
} mf_eeprom_s;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int encode_encoder(const mf_encoder_s* src, mf_eeprom_encoder_s* dst);
int decode_encoder(const mf_eeprom_encoder_s* src, mf_encoder_s* dst);
int decode_proto_cfg(const mf_eeprom_proto_cfg_s* src, proto_cfg_s* dst);
int encode_proto_cfg(const proto_cfg_s* src, mf_eeprom_proto_cfg_s* dst);
int init_eeprom(void);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
	The EEMEM flag is used to store the data in the eeprom memory.
	It adds a linker flag to the data structure to ensure that the linker
	knows the data is only in eeprom

	After linking it is possible to see the memory usage for the eeprom:

	[84/84] Linking C executable neosam.elf
	Memory region         Used Size  Region Size  %age Used
							text:       18640 B       136 KB     13.38%
							data:        5596 B         8 KB     68.31%
						eeprom:         384 B         2 KB     18.75%
	...

	On a completely new device the EEPROM will be initialised with basic values.

	---

	Note - No runtime data is allocated for this variable.
	Note - This variable can be used for reading and writing. The compiler
	can determine the eeprom memoryn addresses to write to if you use
	the appropriate eeprom_read/write/update functions with this variable
	as the source/destination.
*/
EEMEM mf_eeprom_s eeprom_data;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int mf_cfg_init(void) {
	// Check if the eeprom is initialised (the first word == EE_VERSION), if not
	// initialise the eeprom with default values
	u16 version = eeprom_read_word(&eeprom_data.version);

	if (version != EE_VERSION) {
		init_eeprom();
	}

	return 0;
}

int mf_cfg_load(void) {
	// Load encoder banks
	for (int i = 0; i < MF_NUM_ENC_BANKS; i++) {
		for (int j = 0; j < MF_NUM_ENCODERS; j++) {
			mf_eeprom_encoder_s ee_enc = {0};
			eeprom_read_block(&ee_enc, &eeprom_data.encoders[i][j],
												sizeof(mf_eeprom_encoder_s));
			decode_encoder(&ee_enc, &gENCODERS[i][j]);
		}
	}

	return 0;
}

int mf_cfg_store(void) {
	// Encode all configuration data to the eeprom data structure

	for (int i = 0; i < MF_NUM_ENC_BANKS; i++) {
		for (int j = 0; j < MF_NUM_ENCODERS; j++) {
			mf_eeprom_encoder_s enc = {0};
			encode_encoder(&gENCODERS[i][j], &enc);
			eeprom_update_block(&enc, &eeprom_data.encoders[i][j],
													sizeof(mf_eeprom_encoder_s));
		}
	}

	return 0;
}

int mf_cfg_update(void) {
	static uint32_t last_update = 0;
	uint32_t				time_now		= systime_ms();

	// Update every 1 second
	if ((time_now - last_update) > 1000) {
		mf_cfg_store();
		last_update = time_now;
	}

	return 0;
}

int mf_cfg_reset(void) {
	return init_eeprom();
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

int encode_encoder(const mf_encoder_s* src, mf_eeprom_encoder_s* dst) {
	RETURN_ERR_IF_NULL(src);
	RETURN_ERR_IF_NULL(dst);

	dst->display_mode = src->display.mode;
	dst->virtmap_mode = src->display.virtmode;
	dst->detent				= src->detent;
	dst->accel_mode		= src->enc_ctx.accel_mode;
	dst->vmap_mode		= src->vmap_mode;
	dst->sw_mode			= src->sw_mode;
	dst->sw_state			= src->sw_state;
	dst->vmap_active	= src->vmap_active;

	for (int i = 0; i < MF_NUM_VMAPS_PER_ENC; i++) {
		dst->vmap[i].pos	 = src->vmaps[i].curr_pos;
		dst->vmap[i].rgb_r = src->vmaps[i].rgb.red;
		dst->vmap[i].rgb_g = src->vmaps[i].rgb.green;
		dst->vmap[i].rgb_b = src->vmaps[i].rgb.blue;
		dst->vmap[i].rb_r	 = src->vmaps[i].rb.red;
		dst->vmap[i].rb_b	 = src->vmaps[i].rb.blue;
		encode_proto_cfg(&src->vmaps[i].cfg, &dst->vmap[i].cfg);
	}

	encode_proto_cfg(&src->sw_cfg, &dst->sw_cfg);

	return 0;
}

int decode_encoder(const mf_eeprom_encoder_s* src, mf_encoder_s* dst) {
	RETURN_ERR_IF_NULL(src);
	RETURN_ERR_IF_NULL(dst);

	dst->display.mode				= src->display_mode;
	dst->display.virtmode		= src->virtmap_mode;
	dst->detent							= src->detent;
	dst->enc_ctx.accel_mode = src->accel_mode;
	dst->vmap_mode					= src->vmap_mode;
	dst->sw_mode						= src->sw_mode;
	dst->sw_state						= src->sw_state;
	dst->vmap_active				= src->vmap_active;

	for (int i = 0; i < MF_NUM_VMAPS_PER_ENC; i++) {
		dst->vmaps[i].curr_pos	= src->vmap[i].pos;
		dst->vmaps[i].rgb.red		= src->vmap[i].rgb_r;
		dst->vmaps[i].rgb.green = src->vmap[i].rgb_g;
		dst->vmaps[i].rgb.blue	= src->vmap[i].rgb_b;
		dst->vmaps[i].rb.red		= src->vmap[i].rb_r;
		dst->vmaps[i].rb.blue		= src->vmap[i].rb_b;
		decode_proto_cfg(&src->vmap[i].cfg, &dst->vmaps[i].cfg);
	}

	decode_proto_cfg(&src->sw_cfg, &dst->sw_cfg);
	return 0;
}

int decode_proto_cfg(const mf_eeprom_proto_cfg_s* src, proto_cfg_s* dst) {
	RETURN_ERR_IF_NULL(src);
	RETURN_ERR_IF_NULL(dst);

	switch (src->type) {
		case PROTOCOL_NONE: memset(dst, 0, sizeof(proto_cfg_s)); break;

		case PROTOCOL_OSC:
			// Not implemented
			break;

		case PROTOCOL_MIDI:
			dst->type					= PROTOCOL_MIDI;
			dst->midi.mode		= src->midi.mode;
			dst->midi.channel = src->midi.channel;
			dst->midi.raw			= src->midi.raw;
			break;

		default: return ERR_UNSUPPORTED;
	}

	return 0;
}

int encode_proto_cfg(const proto_cfg_s* src, mf_eeprom_proto_cfg_s* dst) {
	RETURN_ERR_IF_NULL(src);
	RETURN_ERR_IF_NULL(dst);

	switch (src->type) {
		case PROTOCOL_NONE: memset(dst, 0, sizeof(mf_eeprom_proto_cfg_s)); break;

		case PROTOCOL_OSC:
			// Not implemented
			break;

		case PROTOCOL_MIDI:
			dst->midi.mode		= src->midi.mode;
			dst->midi.channel = src->midi.channel;
			dst->midi.raw			= src->midi.raw;
			break;

		default: return ERR_UNSUPPORTED;
	}

	return 0;
}

int init_eeprom(void) {
	// Erase the eeprom
	for (int i = 0; i < sizeof(mf_eeprom_s); i++) {
		eeprom_write_byte((uint8_t*)i, 0);
	}

	// Write the magic number
	eeprom_write_word((uint16_t*)0, EE_VERSION);

	// Write the initial state of the system to the eeprom
	mf_cfg_store();

	return 0;
}
