#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "core/core_types.h"
#include "protocol/protocol.h"
#include "virtmap/virtmap_types.h"
#include "io/io_device_types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Initialise the virtual parameter mapping manager.
 *
 * @return int 0 on success, !0 on failure.
 */
int virtmap_manager_init(void);

/**
 * @brief Assigns a virtual parameter mapping to a hardware device.
 *
 * @param vmap Pointer to the vmap.
 * @param dev Pointer to the device.
 * @return int 0 on success, 0! on failure.
 */
int virtmap_assign(virtmap_s* vmap, iodev_s* dev);

/**
 * @brief "Toggles" a vmap linked list by counter-clockwise rotation.
 *
 * @param vmap Pointer to a linked list of vmaps.
 * @return int 0 on success, 0! on failure.
 */
int virtmap_toggle(virtmap_s* vmap);
