/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string.h>
#include <avr/eeprom.h>

#include "hal/eeprom.h"

#include "event/event.h"
#include "event/sys.h"
#include "system/error.h"
#include "system/diag.h"
#include "system/hardware.h"
#include "system/time.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define EE_VERSION						(u16)(16)

#define CFG_STORE_INTERVAL_MS (5000)

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
	u8										 type;
	struct eeprom_midi_cfg midi;
} eeprom_proto_cfg_s;

struct eeprom_encoder {
	// General
	u8 display_mode : 2;
	u8 virtmap_mode : 1;

	// Encoder
	u8 detent				: 1;
	u8 vmap_mode		: 1;
	u8 vmap_active	: 1;

	// Encoder Switch
	u8								 sw_mode;
	eeprom_proto_cfg_s sw_cfg;

	struct {
		eeprom_proto_cfg_s cfg;
		uint16_t					 hsv_h; // Hue (0-1535)
		u8								 hsv_s; // Saturation (0-255)
		u8								 hsv_v; // Value (0-255)
		u8								 rb_r : 4;
		u8								 rb_b : 4;

		// Value-mapping range and physical rotation window - previously set
		// via sysex (MF_SYSEX_PARAM_VMAP_RANGE/_POSITION) but never persisted,
		// so it was lost on every reboot.
		i16 range_lower;
		i16 range_upper;
		u8	position_start;
		u8	position_stop;
	} vmap[NUM_VMAPS_PER_ENC];
};

struct eeprom {
	u16										version;
	u8										reset_pending; // Flag to indicate pending config reset
	struct eeprom_encoder encoders[NUM_ENC_BANKS][NUM_ENCODERS];
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int encode_encoder(const struct encoder*	 src,
													struct eeprom_encoder* dst);
static int decode_encoder(const struct eeprom_encoder* src,
													struct encoder*							 dst);
static int decode_proto_cfg(const eeprom_proto_cfg_s* src,
														struct proto_cfg*					dst);
static int encode_proto_cfg(const struct proto_cfg* src,
														eeprom_proto_cfg_s*			dst);
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
_Static_assert(sizeof(struct eeprom) <= EEPROM_SIZE,
							 "config no longer fits the EEPROM");

EEMEM struct eeprom eeprom_data;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int cfg_init(bool reset_cfg) {
	// Check if the eeprom is initialised (the first word == EE_VERSION), if not
	// initialise the eeprom with default values
	u16 version;
	u8	reset_flag;

	hal_eeprom_read(EE_ADDR(eeprom_data.version), &version, sizeof(version));
	hal_eeprom_read(EE_ADDR(eeprom_data.reset_pending), &reset_flag,
									sizeof(reset_flag));

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
			hal_eeprom_read(EE_ADDR(eeprom_data.encoders[i][j]), &ee_enc,
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
			hal_eeprom_update(EE_ADDR(eeprom_data.encoders[i][j]), &enc,
												sizeof(struct eeprom_encoder));
		}
	}

	return SUCCESS;
}

int cfg_update(void) {
	static uint32_t last_update = 0;
	uint32_t				time_now		= systime_ms();

	if ((time_now - last_update) > CFG_STORE_INTERVAL_MS) {
		DIAG_ON_ERR(cfg_store(), DIAG_CFG_STORE_FAILED);
		last_update = time_now;
	}

	return SUCCESS;
}

int mf_cfg_reset(void) {
	// Set the reset pending flag in EEPROM. The actual data reset happens on next
	// boot.
	const u8 pending = 1;
	hal_eeprom_update(EE_ADDR(eeprom_data.reset_pending), &pending,
										sizeof(pending));
	hal_system_reset(); // This function does not return
	return SUCCESS;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int encode_encoder(const struct encoder*	 src,
													struct eeprom_encoder* dst) {
	dst->display_mode = src->display.mode;
	dst->virtmap_mode = src->display.virtmode;
	dst->detent				= src->detent;
	dst->vmap_mode		= src->vmap_mode;
	dst->sw_mode			= src->sw_mode;
	dst->vmap_active	= src->vmap_active;

	for (int i = 0; i < NUM_VMAPS_PER_ENC; i++) {
		dst->vmap[i].hsv_h = src->vmaps[i].hsv.hue;
		dst->vmap[i].hsv_s = src->vmaps[i].hsv.saturation;
		dst->vmap[i].hsv_v = src->vmaps[i].hsv.value;

		dst->vmap[i].rb_r = (u8)(src->vmaps[i].rb.red >> 4);
		dst->vmap[i].rb_b = (u8)(src->vmaps[i].rb.blue >> 4);
		encode_proto_cfg(&src->vmaps[i].cfg, &dst->vmap[i].cfg);

		// Save the value-mapping range and physical rotation window
		dst->vmap[i].range_lower		= src->vmaps[i].range.lower;
		dst->vmap[i].range_upper		= src->vmaps[i].range.upper;
		dst->vmap[i].position_start = src->vmaps[i].position.start;
		dst->vmap[i].position_stop	= src->vmaps[i].position.stop;
	}

	encode_proto_cfg(&src->sw_cfg, &dst->sw_cfg);

	return SUCCESS;
}

static int decode_encoder(const struct eeprom_encoder* src,
													struct encoder*							 dst) {
	dst->display.mode			= src->display_mode;
	dst->display.virtmode = src->virtmap_mode;
	dst->detent						= src->detent;
	dst->vmap_mode				= src->vmap_mode;
	dst->sw_mode					= src->sw_mode;
	dst->vmap_active			= src->vmap_active;

	for (int i = 0; i < NUM_VMAPS_PER_ENC; i++) {
		dst->vmaps[i].hsv.hue				 = src->vmap[i].hsv_h;
		dst->vmaps[i].hsv.saturation = src->vmap[i].hsv_s;
		dst->vmaps[i].hsv.value			 = src->vmap[i].hsv_v;

		// Update RGB values from HSV values to ensure consistency
		color_update_vmap_rgb(&dst->vmaps[i]);

		dst->vmaps[i].rb.red	= (u8)((src->vmap[i].rb_r << 4) | src->vmap[i].rb_r);
		dst->vmaps[i].rb.blue = (u8)((src->vmap[i].rb_b << 4) | src->vmap[i].rb_b);
		decode_proto_cfg(&src->vmap[i].cfg, &dst->vmaps[i].cfg);

		// Load the value-mapping range and physical rotation window
		dst->vmaps[i].range.lower		 = src->vmap[i].range_lower;
		dst->vmaps[i].range.upper		 = src->vmap[i].range_upper;
		dst->vmaps[i].position.start = src->vmap[i].position_start;
		dst->vmaps[i].position.stop	 = src->vmap[i].position_stop;
	}

	decode_proto_cfg(&src->sw_cfg, &dst->sw_cfg);
	return SUCCESS;
}

static int decode_proto_cfg(const eeprom_proto_cfg_s* src,
														struct proto_cfg*					dst) {
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

static int encode_proto_cfg(const struct proto_cfg* src,
														eeprom_proto_cfg_s*			dst) {
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

	const u16 version = EE_VERSION;
	const u8	cleared = 0;

	hal_eeprom_update(EE_ADDR(eeprom_data.version), &version, sizeof(version));
	hal_eeprom_update(EE_ADDR(eeprom_data.reset_pending), &cleared,
										sizeof(cleared));

	// Write the initial state of the system to the eeprom
	int ret = cfg_store();

	// Send EVT_SYS_RES_CFG_RESET event
	struct sys_event evt = {.type = EVT_SYS_RES_CFG_RESET, .data.ret = ret};
	DIAG_ON_ERR(event_post(EVENT_CHANNEL_SYS, &evt), DIAG_EVENT_DROPPED);
	return ret;
}
