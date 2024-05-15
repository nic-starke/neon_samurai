#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*
	This module implements a system for management of input devices.

	The expected usage is that the board implementation allocates one array
	of io_dev_s for EACH device type to be handled. Device types should
	not be mixed in a single array.

	Each array of io_dev_s must be registered, this allows the system to
	manage io device events without the board implementation getting involved.

	Additionally, each io device requires a run-time context, which must also be
	allocated by the board implementation. The context is used to process the
	io device events and keep track of its state.

	IMPORTANT:
		- all devices of the same type MUST be in the same array, i.e only
			one array per device type.
		- the register array function accepts the first registered array
			per device type, additional calls for the same type are ignored.
		- each io_dev_s must be initialised with a call to io_dev_init.
			The board implementation must handle this.

	Use the following example as a guide for your board implementation:

	static io_dev_s my_encoder_devices[ENCODER_COUNT];
	static enc_ctx_s my_encoder_contexts[ENCODER_COUNT];

	io_dev_register_arr(DEV_TYPE_ENCODER, my_encoder_devices);

	for (uint i = 0; i < ENCODER_COUNT; i++) {
		io_dev_init(DEV_TYPE_ENCODER, &my_encoder_devices[i],
			&my_encoder_contexts[i], i);
	}

	--------

	Further comments on the design/intention:

	The purpose of this module is to enable mapping between a physical
	hardware device, its run-time context, and a set of virtual parameters
	that have been mapped to the device.

	The board implementation allocates all memory requirements up-front,
	thus avoiding any dynamic allocations in the core system code. This also
	ensures that all memory constraints are managed during production, not
	run-time.

	At run-time the board implementation registers all io devices, and
	assigns all virtual parameter mappings to their corresponding device.
	Then, when the board process the IO device hardware it only needs
	to call a single function for each device type, such as "encoder_update".
	These functions generate the necessary IO events, which then cascade
	into other systems such as virtmap that can then generate their own events.

	Thus the design enables the board code to be decoupled from the core
	code by interacting solely with the event system.
 */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "core/core_types.h"
#include "virtmap/virtmap_types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
	DEV_TYPE_ENCODER,
	DEV_TYPE_ENCODER_SWITCH,
	DEV_TYPE_SWITCH,

	DEV_TYPE_NB,
} iodev_type_e;

typedef struct io_dev_s {
	void* ctx; // pointer to device context (such as encoder_s)
	uint	idx; // hardware index

	virtmap_mode_e vmap_mode;
	virtmap_s*		 vmap;
} iodev_s;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
