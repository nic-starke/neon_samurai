/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string.h>
#include <avr/eeprom.h>

#include "event/event.h"
#include "event/sys.h"
#include "system/error.h"
#include "system/hardware.h"
#include "system/time.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define EE_VERSION (u16)(11)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Data structure for eeprom storage using EEMEM flag

struct eeprom_midi_cfg {
	u8 channel : 4;
	u8 mode		 : 4;

	union {
		u8 cc;
		u8 raw;
	};

};

typedef union {
	u8									 type;
	struct eeprom_midi_cfg midi;
} eeprom_proto_cfg_s;

struct eeprom_virtmap_cfg {
	struct {
		i8 lower;
		i8 upper;
	} range;

	struct {
		u8 start;
		u8 stop;
	} position;
};

struct eeprom_encoder {
	// General
	u8 display_mode								 : 2;
	u8 virtmap_mode								 : 1;

	// Encoder
	u8 detent											 : 1;
	u8 vmap_mode									 : 1;
	u8 vmap_active								 : 1;

	// Encoder Switch
	u8										sw_mode;
	eeprom_proto_cfg_s sw_cfg;

	struct {
		eeprom_proto_cfg_s cfg;
		u8										rgb_r;
		u8										rgb_g;
		u8										rgb_b;
		u8										rb_r;
		u8										rb_b;
	} vmap[NUM_VMAPS_PER_ENC];
};

struct eeprom {
	u16									version;
	u8                  reset_pending; // Flag to indicate pending config reset
	struct eeprom_encoder encoders[NUM_ENC_BANKS][NUM_ENCODERS];
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int encode_encoder(const struct mf_encoder* src, struct eeprom_encoder* dst);
static int decode_encoder(const struct eeprom_encoder* src, struct mf_encoder* dst);
static int decode_proto_cfg(const eeprom_proto_cfg_s* src, struct proto_cfg* dst);
static int encode_proto_cfg(const struct proto_cfg* src, eeprom_proto_cfg_s* dst);
static int init_eeprom(void);

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
EEMEM struct eeprom eeprom_data;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int cfg_init(bool reset_cfg) {
	// Check if the eeprom is initialised (the first word == EE_VERSION), if not
	// initialise the eeprom with default values
	u16 version = eeprom_read_word(&eeprom_data.version);
	u8 reset_flag = eeprom_read_byte(&eeprom_data.reset_pending);

	if (reset_flag == 1 || reset_cfg == 1 || version != EE_VERSION) {
		return init_eeprom(); // This will also clear the reset_pending flag
	}

	return SUCCESS;
}

int cfg_load(void) {
	// Load encoder banks
	for (int i = 0; i < NUM_ENC_BANKS; i++) {
		for (int j = 0; j < NUM_ENCODERS; j++) {
			struct eeprom_encoder ee_enc = {0};
			eeprom_read_block(&ee_enc, &eeprom_data.encoders[i][j],
												sizeof(struct eeprom_encoder));
			decode_encoder(&ee_enc, &gENCODERS[i][j]);
		}
	}

	return SUCCESS;
}

int cfg_store(void) {
	// Encode all configuration data to the eeprom data structure

	for (int i = 0; i < NUM_ENC_BANKS; i++) {
		for (int j = 0; j < NUM_ENCODERS; j++) {
			struct eeprom_encoder enc = {0};
			encode_encoder(&gENCODERS[i][j], &enc);
			eeprom_update_block(&enc, &eeprom_data.encoders[i][j],
													sizeof(struct eeprom_encoder));
		}
	}

	return SUCCESS;
}

int cfg_update(void) {
	static uint32_t last_update = 0;
	uint32_t				time_now		= systime_ms();

	// Update every 1 second if something has changed
	if ((time_now - last_update) > 5000) {
		cfg_store();
		last_update = time_now;
	}

	return SUCCESS;
}

int mf_cfg_reset(void) {
	// Set the reset pending flag in EEPROM. The actual data reset happens on next boot.
	eeprom_update_byte(&eeprom_data.reset_pending, 1);
	hal_system_reset(); // This function does not return
	return SUCCESS;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int encode_encoder(const struct mf_encoder* src, struct eeprom_encoder* dst) {
	dst->display_mode = src->display.mode;
	dst->virtmap_mode = src->display.virtmode;
	dst->detent				= src->detent;
	dst->vmap_mode		= src->vmap_mode;
	dst->sw_mode			= src->sw_mode;
	dst->vmap_active	= src->vmap_active;

	for (int i = 0; i < NUM_VMAPS_PER_ENC; i++) {
		dst->vmap[i].rgb_r = src->vmaps[i].rgb.red;
		dst->vmap[i].rgb_g = src->vmaps[i].rgb.green;
		dst->vmap[i].rgb_b = src->vmaps[i].rgb.blue;
		dst->vmap[i].rb_r	 = src->vmaps[i].rb.red;
		dst->vmap[i].rb_b	 = src->vmaps[i].rb.blue;
		encode_proto_cfg(&src->vmaps[i].cfg, &dst->vmap[i].cfg);
	}

	encode_proto_cfg(&src->sw_cfg, &dst->sw_cfg);

	return SUCCESS;
}

static int decode_encoder(const struct eeprom_encoder* src, struct mf_encoder* dst) {
	dst->display.mode				= src->display_mode;
	dst->display.virtmode		= src->virtmap_mode;
	dst->detent							= src->detent;
	dst->vmap_mode					= src->vmap_mode;
	dst->sw_mode						= src->sw_mode;
	dst->vmap_active				= src->vmap_active;

	for (int i = 0; i < NUM_VMAPS_PER_ENC; i++) {
		dst->vmaps[i].rgb.red		= src->vmap[i].rgb_r;
		dst->vmaps[i].rgb.green = src->vmap[i].rgb_g;
		dst->vmaps[i].rgb.blue	= src->vmap[i].rgb_b;
		dst->vmaps[i].rb.red		= src->vmap[i].rb_r;
		dst->vmaps[i].rb.blue		= src->vmap[i].rb_b;
		decode_proto_cfg(&src->vmap[i].cfg, &dst->vmaps[i].cfg);
	}

	decode_proto_cfg(&src->sw_cfg, &dst->sw_cfg);
	return SUCCESS;
}

static int decode_proto_cfg(const eeprom_proto_cfg_s* src, struct proto_cfg* dst) {
	switch (src->type) {
		case PROTOCOL_NONE: memset(dst, 0, sizeof(struct proto_cfg)); break;

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

	return SUCCESS;
}

static int encode_proto_cfg(const struct proto_cfg* src, eeprom_proto_cfg_s* dst) {
	switch (src->type) {
		case PROTOCOL_NONE: memset(dst, 0, sizeof(eeprom_proto_cfg_s)); break;

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

	return SUCCESS;
}

static int init_eeprom(void) {
	// Erase the eeprom
	for (int i = 0; i < sizeof(struct eeprom); i++) {
		eeprom_write_byte((uint8_t*)i, 0);
	}

	// Write the magic number
	eeprom_write_word(&eeprom_data.version, EE_VERSION);

	// Clear the reset pending flag
	eeprom_write_byte(&eeprom_data.reset_pending, 0); // Use write_byte as EEPROM is already erased to 0xFF

	// Write the initial state of the system to the eeprom
	int ret = cfg_store();

	// Send EVT_SYS_RES_CFG_RESET event
	struct sys_event evt = { .type = EVT_SYS_RES_CFG_RESET, .data.ret = ret };
	event_post(EVENT_CHANNEL_SYS, &evt);
	return ret;
}
