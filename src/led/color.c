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

extern struct encoder gENCODERS[NUM_ENC_BANKS][NUM_ENCODERS];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */

// Gamma brightness lookup table
/*
	Gamma correction, exponent 2.0.

	The exponent is a compromise rather than a constant of nature. A higher one
	tracks perceived brightness more closely, but it flattens the bottom of the
	curve: several inputs in a row then produce the same output, and the next
	one after that is a large jump in apparent brightness, because at an output
	of four out of 255 a single level is a quarter as much light again. Anything
	fading slowly through that region visibly steps.

	Two point zero keeps most of the perceptual shape while lifting the bottom
	of the curve clear of the worst of it. It is the usual choice for LEDs
	viewed directly rather than for a display being matched to sRGB.
*/
_Static_assert(MAX_BRIGHTNESS == UINT8_MAX,
							 "the gamma LUT is a u8 table indexed by a u8 - it can only span "
							 "the BCM range if that range is exactly a u8");

const uint8_t gamma_lut[256] PROGMEM = {
		0,	 0,		0,	 0,		0,	 0,		0,	 0,		0,	 0,		0,	 0,		1,	 1,		1,
		1,	 1,		1,	 1,		1,	 2,		2,	 2,		2,	 2,		2,	 3,		3,	 3,		3,
		4,	 4,		4,	 4,		5,	 5,		5,	 5,		6,	 6,		6,	 7,		7,	 7,		8,
		8,	 8,		9,	 9,		9,	 10,	10,	 11,	11,	 11,	12,	 12,	13,	 13,	14,
		14,	 15,	15,	 16,	16,	 17,	17,	 18,	18,	 19,	19,	 20,	20,	 21,	21,
		22,	 23,	23,	 24,	24,	 25,	26,	 26,	27,	 28,	28,	 29,	30,	 30,	31,
		32,	 32,	33,	 34,	35,	 35,	36,	 37,	38,	 38,	39,	 40,	41,	 42,	42,
		43,	 44,	45,	 46,	47,	 47,	48,	 49,	50,	 51,	52,	 53,	54,	 55,	56,
		56,	 57,	58,	 59,	60,	 61,	62,	 63,	64,	 65,	66,	 67,	68,	 69,	70,
		71,	 73,	74,	 75,	76,	 77,	78,	 79,	80,	 81,	82,	 84,	85,	 86,	87,
		88,	 89,	91,	 92,	93,	 94,	95,	 97,	98,	 99,	100, 102, 103, 104, 105,
		107, 108, 109, 111, 112, 113, 115, 116, 117, 119, 120, 121, 123, 124, 126,
		127, 128, 130, 131, 133, 134, 136, 137, 139, 140, 142, 143, 145, 146, 148,
		149, 151, 152, 154, 155, 157, 158, 160, 162, 163, 165, 166, 168, 170, 171,
		173, 175, 176, 178, 180, 181, 183, 185, 186, 188, 190, 192, 193, 195, 197,
		199, 200, 202, 204, 206, 207, 209, 211, 213, 215, 217, 218, 220, 222, 224,
		226, 228, 230, 232, 233, 235, 237, 239, 241, 243, 245, 247, 249, 251, 253,
		255,
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
void color_hsv_to_rgb(uint16_t hue, uint8_t sat, uint8_t val,
											struct rgb_8* out) {
	assert(out);

	uint8_t r_linear, g_linear, b_linear; // Temp vars for 0-255 RGB values

	// Convert HSV to linear RGB using the optimized HSV to RGB conversion
	// The function expects hue in 0-1535 range, saturation and value in 0-255
	// range
	fast_hsv2rgb_8bit(hue, sat, val, &r_linear, &g_linear, &b_linear);

	// Apply gamma correction using the lookup tables.
	out->red	 = pgm_read_byte(&gamma_lut[r_linear]);
	out->green = pgm_read_byte(&gamma_lut[g_linear]);
	out->blue	 = pgm_read_byte(&gamma_lut[b_linear]);
}

void color_update_vmap_rgb(struct virtmap* vmap) {
	assert(vmap);

	color_hsv_to_rgb(vmap->hsv.hue, vmap->hsv.saturation, vmap->hsv.value,
									 &vmap->rgb);
}

/**
 * @brief Sets linear RGB values with gamma correction
 *
 * Takes linear RGB values (0-255) and converts them to gamma-corrected
 * BCM values (0-255) for display on the LEDs.
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
	vmap->rgb.red		= pgm_read_byte(&gamma_lut[r_linear]);
	vmap->rgb.green = pgm_read_byte(&gamma_lut[g_linear]);
	vmap->rgb.blue	= pgm_read_byte(&gamma_lut[b_linear]);

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
 * Allows direct setting of the BCM RGB values (0-255) without gamma correction.
 * This is useful for direct control of the LED brightness levels.
 *
 * @param vmap Pointer to the virtmap structure
 * @param r_bcm Red BCM value (0-255)
 * @param g_bcm Green BCM value (0-255)
 * @param b_bcm Blue BCM value (0-255)
 */
void color_set_vmap_rgb_bcm(struct virtmap* vmap, uint8_t r_bcm, uint8_t g_bcm,
														uint8_t b_bcm) {
	assert(vmap);

	vmap->rgb.red		= r_bcm;
	vmap->rgb.green = g_bcm;
	vmap->rgb.blue	= b_bcm;

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

	h = MIN(h, (u16)(HSV_HUE_STEPS - 1));

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
 * @param r_bcm Red BCM value (0-255)
 * @param g_bcm Green BCM value (0-255)
 * @param b_bcm Blue BCM value (0-255)
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
