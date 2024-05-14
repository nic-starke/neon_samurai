/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "core/core_error.h"
#include "io/io_device.h"
#include "io/encoder/encoder_quadrature.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static iodev_s* devices[DEV_TYPE_NB] = {0};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int iodev_init(iodev_type_e type, iodev_s* dev, void* ctx, uint index) {
	assert(dev);
	assert(ctx);

	if (type >= DEV_TYPE_NB) {
		return ERR_BAD_PARAM;
	}

	switch (type) {

		case DEV_TYPE_ENCODER: {
			dev->encoder = ctx;
			dev->vmap		 = NULL;
			return encoder_init(ctx, index);
		}

		case DEV_TYPE_ENCODER_SWITCH: {
			break;
		}

		case DEV_TYPE_SWITCH: {
			break;
		}

		default: return ERR_BAD_PARAM;
	}
}

int iodev_register_arr(iodev_type_e type, iodev_s* arr) {
	assert(arr);

	if (type >= DEV_TYPE_NB) {
		return ERR_BAD_PARAM;
	}

	// Check if this device type was already registered.
	if (devices[type] != NULL) {
		return ERR_DUPLICATE;
	}

	devices[type] = arr;

	return 0;
}

int iodev_assign_virtmap(iodev_s* dev, virtmap_s* virtmap) {
	assert(dev);
	assert(virtmap);

	virtmap_s* v = dev->vmap;

	// If nothing assigned yet then assign.
	if (v == NULL) {
		v = virtmap;
		return 0;
	}

	// Traverse the linked list of vmaps..
	while (v->next != NULL) {
		v = v->next;
	}

	// Assign to first empty slot
	v->next = virtmap;
	return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
