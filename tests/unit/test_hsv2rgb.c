/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "nstest.h"

#include "led/hsv2rgb.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// The conversion trades a little accuracy for speed, so the primaries are
// checked to within a hair rather than exactly.
#define TOLERANCE (2)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int near(uint8_t actual, int expected) {
	int diff = (int)actual - expected;
	return (diff < 0 ? -diff : diff) <= TOLERANCE;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Tests ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

TEST(no_saturation_is_grey) {
	for (int v = 0; v <= 255; v += 15) {
		uint8_t r, g, b;
		fast_hsv2rgb_8bit(300, 0, (uint8_t)v, &r, &g, &b);
		CHECK_EQ(r, v);
		CHECK_EQ(g, v);
		CHECK_EQ(b, v);
	}
}

TEST(no_value_is_black) {
	for (int h = 0; h < HSV_HUE_STEPS; h += 97) {
		uint8_t r, g, b;
		fast_hsv2rgb_8bit((uint16_t)h, 255, 0, &r, &g, &b);
		CHECK_EQ(r, 0);
		CHECK_EQ(g, 0);
		CHECK_EQ(b, 0);
	}
}

TEST(the_primaries_land_where_they_should) {
	uint8_t r, g, b;

	fast_hsv2rgb_8bit(0 * HSV_HUE_SEXTANT, 255, 255, &r, &g, &b);
	CHECK(near(r, 255) && near(g, 0) && near(b, 0)); // red

	fast_hsv2rgb_8bit(2 * HSV_HUE_SEXTANT, 255, 255, &r, &g, &b);
	CHECK(near(r, 0) && near(g, 255) && near(b, 0)); // green

	fast_hsv2rgb_8bit(4 * HSV_HUE_SEXTANT, 255, 255, &r, &g, &b);
	CHECK(near(r, 0) && near(g, 0) && near(b, 255)); // blue
}

TEST(the_secondaries_land_where_they_should) {
	uint8_t r, g, b;

	fast_hsv2rgb_8bit(1 * HSV_HUE_SEXTANT, 255, 255, &r, &g, &b);
	CHECK(near(r, 255) && near(g, 255) && near(b, 0)); // yellow

	fast_hsv2rgb_8bit(3 * HSV_HUE_SEXTANT, 255, 255, &r, &g, &b);
	CHECK(near(r, 0) && near(g, 255) && near(b, 255)); // cyan

	fast_hsv2rgb_8bit(5 * HSV_HUE_SEXTANT, 255, 255, &r, &g, &b);
	CHECK(near(r, 255) && near(g, 0) && near(b, 255)); // magenta
}

TEST(full_value_always_drives_one_channel_to_the_top) {
	// Whatever the hue, a fully saturated colour at full value has a channel at
	// or very near maximum - otherwise the LEDs dim as the hue sweeps.
	for (int h = 0; h < HSV_HUE_STEPS; ++h) {
		uint8_t r, g, b;
		fast_hsv2rgb_8bit((uint16_t)h, 255, 255, &r, &g, &b);

		uint8_t top = r;
		if (g > top) {
			top = g;
		}
		if (b > top) {
			top = b;
		}
		CHECK(near(top, 255));
	}
}

TEST(full_saturation_always_drives_one_channel_to_the_bottom) {
	for (int h = 0; h < HSV_HUE_STEPS; ++h) {
		uint8_t r, g, b;
		fast_hsv2rgb_8bit((uint16_t)h, 255, 255, &r, &g, &b);

		uint8_t bottom = r;
		if (g < bottom) {
			bottom = g;
		}
		if (b < bottom) {
			bottom = b;
		}
		CHECK(near(bottom, 0));
	}
}

TEST(any_hue_at_all_produces_a_usable_colour) {
	/*
		The sextant guard pins the sextant to the last one, but not the fraction
		within it, so an out-of-range hue is not the same colour as the end of the
		wheel - it is some colour inside the final sextant. Callers clamp the hue
		before this point; what matters here is that no 16-bit hue can drive the
		conversion somewhere it does not handle.
	*/
	for (uint32_t h = 0; h <= 0xFFFF; ++h) {
		uint8_t r = 0xAA, g = 0xAA, b = 0xAA;
		uint8_t r2 = 0x55, g2 = 0x55, b2 = 0x55;

		fast_hsv2rgb_8bit((uint16_t)h, 255, 255, &r, &g, &b);
		fast_hsv2rgb_8bit((uint16_t)h, 255, 255, &r2, &g2, &b2);

		// All three channels written, regardless of what came before.
		CHECK_EQ(r, r2);
		CHECK_EQ(g, g2);
		CHECK_EQ(b, b2);

		uint8_t top = r > g ? (r > b ? r : b) : (g > b ? g : b);
		uint8_t bot = r < g ? (r < b ? r : b) : (g < b ? g : b);
		CHECK(near(top, 255));
		CHECK(near(bot, 0));
	}
}

TEST(lowering_value_never_brightens_a_channel) {
	for (int h = 0; h < HSV_HUE_STEPS; h += 61) {
		uint8_t pr = 255, pg = 255, pb = 255;

		for (int v = 255; v >= 0; v -= 5) {
			uint8_t r, g, b;
			fast_hsv2rgb_8bit((uint16_t)h, 255, (uint8_t)v, &r, &g, &b);
			CHECK(r <= pr);
			CHECK(g <= pg);
			CHECK(b <= pb);
			pr = r;
			pg = g;
			pb = b;
		}
	}
}

TEST(every_hue_and_value_writes_all_three_channels) {
	// The conversion swaps its output pointers around per sextant, so a missed
	// case would leave a channel untouched rather than obviously wrong.
	for (int h = 0; h < HSV_HUE_STEPS; h += 13) {
		for (int v = 0; v <= 255; v += 51) {
			uint8_t r = 0xAA, g = 0xAA, b = 0xAA;
			uint8_t r2 = 0x55, g2 = 0x55, b2 = 0x55;

			fast_hsv2rgb_8bit((uint16_t)h, 200, (uint8_t)v, &r, &g, &b);
			fast_hsv2rgb_8bit((uint16_t)h, 200, (uint8_t)v, &r2, &g2, &b2);

			// Both start from different junk; agreeing means all three were set.
			CHECK_EQ(r, r2);
			CHECK_EQ(g, g2);
			CHECK_EQ(b, b2);
		}
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Main ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

NSTEST_MAIN(RUN(no_saturation_is_grey); RUN(no_value_is_black);
						RUN(the_primaries_land_where_they_should);
						RUN(the_secondaries_land_where_they_should);
						RUN(full_value_always_drives_one_channel_to_the_top);
						RUN(full_saturation_always_drives_one_channel_to_the_bottom);
						RUN(any_hue_at_all_produces_a_usable_colour);
						RUN(lowering_value_never_brightens_a_channel);
						RUN(every_hue_and_value_writes_all_three_channels);)
