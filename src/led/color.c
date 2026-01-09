/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/**
 * @file color.c
 * @brief Implementation of the HSV color system with gamma correction
 */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "system/types.h"
#include "system/error.h"
#include "system/utility.h"
#include "system/hardware.h"
#include "console/console.h"
#include "led/color.h"
#include "led/hsv2rgb.h"
#include "io/encoder.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */

// Gamma brightness lookup table <https://victornpb.github.io/gamma-table-generator>
// gamma = 2.20 steps = 256 range = 0-255
const uint8_t gamma_lut[256] PROGMEM = {
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,
	1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,
	3,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6,   6,
	6,   7,   7,   7,   8,   8,   8,   9,   9,   9,  10,  10,  11,  11,  11,  12,
 12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,
 20,  20,  21,  22,  22,  23,  23,  24,  25,  25,  26,  26,  27,  28,  28,  29,
 30,  30,  31,  32,  33,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,
 42,  43,  43,  44,  45,  46,  47,  48,  49,  49,  50,  51,  52,  53,  54,  55,
 56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,
 73,  74,  75,  76,  77,  78,  79,  81,  82,  83,  84,  85,  87,  88,  89,  90,
 91,  93,  94,  95,  97,  98,  99, 100, 102, 103, 105, 106, 107, 109, 110, 111,
113, 114, 116, 117, 119, 120, 121, 123, 124, 126, 127, 129, 130, 132, 133, 135,
137, 138, 140, 141, 143, 145, 146, 148, 149, 151, 153, 154, 156, 158, 159, 161,
163, 165, 166, 168, 170, 172, 173, 175, 177, 179, 181, 182, 184, 186, 188, 190,
192, 194, 196, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221,
223, 225, 227, 229, 231, 234, 236, 238, 240, 242, 244, 246, 248, 251, 253, 255,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static struct virtmap* get_virtmap_from_indices(uint8_t bank, uint8_t enc,
																								uint8_t vmap_idx);
static void						 update_encoder_display(uint8_t bank, uint8_t enc);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Updates RGB values from HSV color space with gamma correction
 *
 * Converts the HSV color values stored in the virtmap to RGB values,
 * then applies gamma correction to produce BCM-compatible brightness levels.
 *
 * @param vmap Pointer to the virtmap structure containing HSV color values
 */
void color_update_vmap_rgb(struct virtmap* vmap) {
	assert(vmap);

	uint8_t r_linear, g_linear, b_linear; // Temp vars for 0-255 RGB values

	// Convert HSV to linear RGB using the optimized HSV to RGB conversion
	// The function expects hue in 0-1535 range, saturation and value in 0-255
	// range
	fast_hsv2rgb_8bit(vmap->hsv.hue, vmap->hsv.saturation, vmap->hsv.value,
										&r_linear, &g_linear, &b_linear);

	// Apply gamma correction using the lookup tables
	// This maps the linear 0-255 RGB values to gamma-corrected 0-31 BCM values
	vmap->rgb.red		= pgm_read_byte(&gamma_lut[r_linear]) >> 3;
	vmap->rgb.green = pgm_read_byte(&gamma_lut[g_linear]) >> 3;
	vmap->rgb.blue	= pgm_read_byte(&gamma_lut[b_linear]) >> 3;

	// Ensure values are within the valid BCM range (0-31)
	vmap->rgb.red		= CLAMP(vmap->rgb.red, 0, NUM_PWM_FRAMES - 1);
	vmap->rgb.green = CLAMP(vmap->rgb.green, 0, NUM_PWM_FRAMES - 1);
	vmap->rgb.blue	= CLAMP(vmap->rgb.blue, 0, NUM_PWM_FRAMES - 1);
}

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
															 uint8_t g_linear, uint8_t b_linear) {
	assert(vmap);

	// Apply gamma correction using the lookup tables
	vmap->rgb.red		= pgm_read_byte(&gamma_lut[r_linear]) >> 3;
	vmap->rgb.green = pgm_read_byte(&gamma_lut[g_linear]) >> 3;
	vmap->rgb.blue	= pgm_read_byte(&gamma_lut[b_linear]) >> 3;

	// Ensure values are within the valid BCM range
	vmap->rgb.red		= CLAMP(vmap->rgb.red, 0, NUM_PWM_FRAMES - 1);
	vmap->rgb.green = CLAMP(vmap->rgb.green, 0, NUM_PWM_FRAMES - 1);
	vmap->rgb.blue	= CLAMP(vmap->rgb.blue, 0, NUM_PWM_FRAMES - 1);

	// Also update the HSV values to maintain consistency
	// This is a simple approximation since RGB to HSV conversion is more complex
	// and not needed for normal operation, but helps maintain state consistency
	uint16_t max_val = MAX(r_linear, MAX(g_linear, b_linear));

	if (max_val == 0) {
		// Black color case
		vmap->hsv.hue				 = 0;
		vmap->hsv.saturation = 0;
		vmap->hsv.value			 = 0;
		return;
	}

	vmap->hsv.value = max_val;

	uint16_t min_val = MIN(r_linear, MIN(g_linear, b_linear));
	uint16_t delta	 = max_val - min_val;

	if (delta == 0) {
		// Gray color case
		vmap->hsv.hue				 = 0;
		vmap->hsv.saturation = 0;
	} else {
		vmap->hsv.saturation = (255 * delta) / max_val;

		if (r_linear == max_val) {
			vmap->hsv.hue = (((g_linear - b_linear) * 256) / delta) % 1536;
		} else if (g_linear == max_val) {
			vmap->hsv.hue = ((b_linear - r_linear) * 256) / delta + 512;
		} else {
			vmap->hsv.hue = ((r_linear - g_linear) * 256) / delta + 1024;
		}
	}
}

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
														uint8_t b_bcm) {
	assert(vmap);

	// Set BCM values directly with range checking
	vmap->rgb.red		= CLAMP(r_bcm, 0, NUM_PWM_FRAMES - 1);
	vmap->rgb.green = CLAMP(g_bcm, 0, NUM_PWM_FRAMES - 1);
	vmap->rgb.blue	= CLAMP(b_bcm, 0, NUM_PWM_FRAMES - 1);

	// This function doesn't update HSV values since it's a direct BCM setter
	// This can cause state inconsistency between HSV and RGB values
	// Which is acceptable for direct BCM control use cases
}

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
												uint8_t s, uint8_t v) {
	// Get the virtmap pointer
	struct virtmap* vmap = get_virtmap_from_indices(bank, enc, vmap_idx);
	if (!vmap)
		return;

	// Clamp input values to valid ranges
	h = CLAMP(h, 0, HSV_HUE_STEPS - 1); // 0-1535
	s = CLAMP(s, 0, 255);								// 0-255
	v = CLAMP(v, 0, 255);								// 0-255

	// Set the HSV values
	vmap->hsv.hue				 = h;
	vmap->hsv.saturation = s;
	vmap->hsv.value			 = v;

	// Update the RGB values
	color_update_vmap_rgb(vmap);

	// Request display update
	update_encoder_display(bank, enc);
}

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
																				uint8_t g_linear, uint8_t b_linear) {
	// Get the virtmap pointer
	struct virtmap* vmap = get_virtmap_from_indices(bank, enc, vmap_idx);
	if (!vmap)
		return;

	// Set the RGB values with gamma correction
	color_set_vmap_rgb_linear(vmap, r_linear, g_linear, b_linear);

	// Request display update
	update_encoder_display(bank, enc);
}

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
																		 uint8_t g_bcm, uint8_t b_bcm) {
	// Get the virtmap pointer
	struct virtmap* vmap = get_virtmap_from_indices(bank, enc, vmap_idx);
	if (!vmap)
		return;

	// Set the BCM RGB values directly
	color_set_vmap_rgb_bcm(vmap, r_bcm, g_bcm, b_bcm);

	// Request display update
	update_encoder_display(bank, enc);
}

/**
 * @brief Print the gamma lookup table for a specific color channel
 *
 * @param channel Color channel ('r', 'g', or 'b')
 */
void color_print_gamma_lut(char channel) {
	char					 buffer[32];
	const uint8_t* lut = gamma_lut;

	// Print header
	console_puts_p(PSTR("Linear | Gamma | BCM\r\n"));
	console_puts_p(PSTR("------+-------+-----\r\n"));

	// Print values at regular intervals to avoid overwhelming the console
	for (uint16_t i = 0; i <= 255; i += 16) {
		uint8_t gamma_val = pgm_read_byte(&lut[i]);
		uint8_t bcm_val		= gamma_val >> 3; // Scale to 0-31 for BCM

		snprintf_P(buffer, sizeof(buffer), PSTR(" %3u  |  %3u  | %2u\r\n"), i,
							 gamma_val, bcm_val);
		console_puts(buffer);
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * Helper function to get a virtmap pointer from bank, encoder, and vmap indices
 *
 * @param bank Bank index
 * @param enc Encoder index
 * @param vmap_idx Virtmap index
 * @return Pointer to the requested virtmap or NULL if indices are invalid
 */
static struct virtmap* get_virtmap_from_indices(uint8_t bank, uint8_t enc,
																								uint8_t vmap_idx) {
	// Check if indices are within valid range
	if (bank >= NUM_ENC_BANKS || enc >= NUM_ENCODERS ||
			vmap_idx >= NUM_VMAPS_PER_ENC) {
		return NULL;
	}

	// Return pointer to the virtmap
	return &gENCODERS[bank][enc].vmaps[vmap_idx];
}

/**
 * Helper function to request display update for an encoder
 *
 * @param bank Bank index
 * @param enc Encoder index
 */
static void update_encoder_display(uint8_t bank, uint8_t enc) {
	// Check if indices are within valid range
	if (bank >= NUM_ENC_BANKS || enc >= NUM_ENCODERS) {
		return;
	}

	// Mark encoder for update by setting timestamp
	// A value of 1 will trigger an immediate update on the next display_update
	// cycle
	gENCODERS[bank][enc].update_display = 1;
}
