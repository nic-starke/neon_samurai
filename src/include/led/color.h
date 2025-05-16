#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/**
 * @file color.h
 * @brief HSV color system with gamma correction for RGB LEDs
 *
 * This module provides functionality for HSV color manipulation with gamma
 * correction to ensure visually linear LED brightness responses. It includes
 * conversion between HSV and RGB color spaces and lookup tables for gamma
 * correction.
 */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

extern const uint8_t gamma_corrected_lut_red[256];
extern const uint8_t gamma_corrected_lut_green[256];
extern const uint8_t gamma_corrected_lut_blue[256];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief HSV Color structure
 *
 * Represents a color in the HSV (Hue, Saturation, Value) color space.
 * Hue: 0-1535 (0-360 degrees) - 0=red, 512=green, 1024=blue
 * Saturation: 0-255 (0-100%)
 * Value: 0-255 (0-100%)
 */
struct hsv_color {
	uint16_t hue;				 // 0-1535 (0-360 degrees)
	uint8_t	 saturation; // 0-255
	uint8_t	 value;			 // 0-255
};

// Forward declare virtmap structure to avoid circular dependencies
struct virtmap;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Updates RGB values from HSV color space with gamma correction
 *
 * Converts the HSV color values stored in the virtmap to RGB values,
 * then applies gamma correction to produce BCM-compatible brightness levels.
 *
 * @param vmap Pointer to the virtmap structure containing HSV color values
 */
void color_update_vmap_rgb(struct virtmap* vmap);

/**
 * @brief Set HSV values for a specific virtmap and update RGB
 *
 * @param bank Bank index
 * @param enc Encoder index
 * @param vmap_idx Virtmap index
 * @param h Hue (0-1535)
 * @param s Saturation (0-255)
 * @param v Value (0-255)
 */
void color_set_vmap_hsv(uint8_t bank, uint8_t enc, uint8_t vmap_idx, uint16_t h,
												uint8_t s, uint8_t v);

/**
 * @brief Set linear RGB values for a specific virtmap with indices
 *
 * @param bank Bank index
 * @param enc Encoder index
 * @param vmap_idx Virtmap index
 * @param r_linear Red linear value (0-255)
 * @param g_linear Green linear value (0-255)
 * @param b_linear Blue linear value (0-255)
 */
void color_set_vmap_rgb_linear_by_index(uint8_t bank, uint8_t enc,
																				uint8_t vmap_idx, uint8_t r_linear,
																				uint8_t g_linear, uint8_t b_linear);

/**
 * @brief Sets linear RGB values with gamma correction
 *
 * Takes linear RGB values (0-255) and converts them to gamma-corrected
 * BCM values (0-31) for display on the LEDs.
 *
 * @param vmap Pointer to the virtmap structure
 * @param r_linear Red component (0-255)
 * @param g_linear Green component (0-255)
 * @param b_linear Blue component (0-255)
 */
void color_set_vmap_rgb_linear(struct virtmap* vmap, uint8_t r_linear,
															 uint8_t g_linear, uint8_t b_linear);

/**
 * @brief Set BCM RGB values directly for a specific virtmap with indices
 *
 * @param bank Bank index
 * @param enc Encoder index
 * @param vmap_idx Virtmap index
 * @param r_bcm Red BCM value (0-31)
 * @param g_bcm Green BCM value (0-31)
 * @param b_bcm Blue BCM value (0-31)
 */
void color_set_vmap_rgb_bcm_by_index(uint8_t bank, uint8_t enc,
																		 uint8_t vmap_idx, uint8_t r_bcm,
																		 uint8_t g_bcm, uint8_t b_bcm);

/**
 * @brief Sets BCM RGB values directly
 *
 * Allows direct setting of the BCM RGB values (0-31) without gamma correction.
 * This is useful for direct control of the LED brightness levels.
 *
 * @param vmap Pointer to the virtmap structure
 * @param r_bcm Red BCM value (0-31)
 * @param g_bcm Green BCM value (0-31)
 * @param b_bcm Blue BCM value (0-31)
 */
void color_set_vmap_rgb_bcm(struct virtmap* vmap, uint8_t r_bcm, uint8_t g_bcm,
														uint8_t b_bcm);

/**
 * @brief Print the gamma lookup table for a specific color channel
 *
 * @param channel Color channel ('r', 'g', or 'b')
 */
void color_print_gamma_lut(char channel);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
