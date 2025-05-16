/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"
#include "event/event.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define ANIMATION_MAX_FRAMES 8
#define ANIMATION_DEFAULT_DURATION_MS 250
#define ANIMATION_MAX_CONCURRENT 4

#define ERR_NO_ANIMATION -100  // Error code for when no animation is found

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

extern struct event_channel animation_event_ch;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum animation_event_type {
    ANIM_EVT_BANK_CHANGE,
    ANIM_EVT_ENCODER_RESET,

    ANIM_EVT_NB,
};

// Animation type definitions
enum animation_type {
    ANIM_TYPE_NONE,
    ANIM_TYPE_BANK_CHANGE,

    ANIM_TYPE_NB,
};

// Animation event data structure
struct animation_event {
    u8 type;
    union {
        struct {
            u8 prev_bank;
            u8 new_bank;
        } bank_change;
        struct {
            u8 encoder_idx;
        } encoder_reset;
    } data;
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Initialize the animation system
 *
 * @return int 0 on success, error code otherwise
 */
int animation_init(void);

/**
 * @brief Check if any animation is currently active
 *
 * @return true if at least one animation is active
 * @return false if no animations are active
 */
bool animation_is_active(void);

/**
 * @brief Check if a specific animation is currently active
 *
 * @param anim_idx Animation index (0 to ANIMATION_MAX_CONCURRENT-1)
 * @return true if the animation is active
 * @return false if the animation is not active
 */
bool animation_is_active_idx(u8 anim_idx);

/**
 * @brief Update all animations (call in the main loop)
 *
 * @return int 0 on success, error code otherwise
 */
int animation_update(void);

/**
 * @brief Start a bank change animation
 *
 * @param prev_bank Previous bank index
 * @param new_bank New bank index
 * @return int 0 on success, error code otherwise
 */
int animation_start_bank_change(u8 prev_bank, u8 new_bank);

/**
 * @brief Draw the current animation frame for a specific encoder
 *
 * @param encoder_idx Index of the encoder to draw
 * @return int 0 on success, error code otherwise
 */
int animation_draw_encoder(u8 encoder_idx);

/**
 * @brief Get the number of currently active animations
 *
 * @return u8 Number of active animations (0 to ANIMATION_MAX_CONCURRENT)
 */
u8 animation_get_active_count(void);

/**
 * @brief Start a custom animation on a specific encoder
 *
 * @param type Animation type
 * @param encoder_idx Encoder index to animate
 * @param duration_ms Animation duration in milliseconds
 * @param frames Number of frames in the animation
 * @return int Animation slot index if successful, negative error code otherwise
 */
int animation_start_custom(enum animation_type type, u8 encoder_idx, u32 duration_ms, u8 frames);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
