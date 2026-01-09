#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Bit positions for LED components (matches hardware bit layout)
#define RGB_RED_BIT           (3)
#define RGB_GREEN_BIT         (4)
#define RGB_BLUE_BIT          (2)
#define DETENT_RED_BIT        (1)
#define DETENT_BLUE_BIT       (0)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Add this enum for LFO display priority
enum lfo_display_priority {
    LFO_DISPLAY_PRIO_NONE,
    LFO_DISPLAY_PRIO_RATE,
    LFO_DISPLAY_PRIO_DEPTH
};

// Add this to mirror the enum from input_manager.c for type safety with the getter
enum lfo_menu_page {
    LFO_MENU_PAGE_NONE = 0,
    LFO_MENU_PAGE_RATE,
    LFO_MENU_PAGE_DEPTH,
    LFO_MENU_PAGE_COUNT
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Get the active LFO display mode from input manager
 * @return Current LFO display priority
 */
enum lfo_display_priority input_manager_get_active_lfo_display_mode(void);
enum lfo_menu_page input_manager_get_lfo_menu_page(void);

/**
 * @brief Initialise the display hardware and driver.
 *
 * @return int 0 on success, !0 on failure
 */
int display_init(void);

/**
 * @brief Periodic display update.
 */
void display_update(void);
