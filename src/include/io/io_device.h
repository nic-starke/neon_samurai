#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "io/io_device_types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Initialise a single IO device.
 *
 * @param type Type of the device.
 * @param dev Pointer to iodev_s.
 * @param ctx Pointer to device context structure.
 * @param index Hardware index to assign, used for array bound checks.
 * @return int 0 on success, !0 on failure.
 */
int iodev_init(iodev_type_e type, iodev_s* dev, void* ctx, uint index);

/**
 * @brief Assigns a virtual parameter mapping to an io device.
 *
 * @param dev Pointer to the device.
 * @param virtmap Pointer to the virtual mapping structure.
 * @return int 0 on success, !0 on failure.
 */
int iodev_assign_virtmap(iodev_s* dev, virtmap_s* virtmap);

#warning "this should be inside an io event handler, not exposed externally..."
int iodev_update(iodev_type_e type, iodev_s* dev);
