/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "nstest.h"

#include "system/types.h"
#include "animation/idle.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Encoders in reading order, so the corners are worth naming.
#define TOP_LEFT		 (0)
#define TOP_RIGHT		 (3)
#define BOTTOM_LEFT	 (12)
#define BOTTOM_RIGHT (15)


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Tests ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

TEST(the_wave_leads_at_the_last_encoder) {
	CHECK_EQ(idle_wave_offset(IDLE_NUM_ENCODERS - 1u), 0);

	// Nothing may lead it.
	for (u8 e = 0; e < IDLE_NUM_ENCODERS; e++) {
		CHECK(idle_wave_offset(e) >= idle_wave_offset(IDLE_NUM_ENCODERS - 1u));
	}
}

TEST(the_wave_travels_along_a_row) {
	CHECK(idle_wave_offset(15) < idle_wave_offset(14));
	CHECK(idle_wave_offset(14) < idle_wave_offset(13));
	CHECK(idle_wave_offset(13) < idle_wave_offset(12));
}

TEST(each_row_starts_just_before_the_second_encoder_of_the_row_below) {
	/*
		This is what tilts the wavefront. Encoder 11 heads the row above encoder
		15's, and has to begin fractionally before encoder 14 - the next encoder
		along the row below - rather than after it. Were it later, the wave would
		finish each row before starting the next and read as four separate sweeps
		instead of one diagonal.
	*/
	CHECK(idle_wave_offset(11) < idle_wave_offset(14));
	CHECK(idle_wave_offset(7) < idle_wave_offset(10));
	CHECK(idle_wave_offset(3) < idle_wave_offset(6));
}

TEST(each_row_starts_after_the_one_below_it) {
	CHECK(idle_wave_offset(15) < idle_wave_offset(11));
	CHECK(idle_wave_offset(11) < idle_wave_offset(7));
	CHECK(idle_wave_offset(7) < idle_wave_offset(3));
}

TEST(the_far_corner_lags_the_most) {
	for (u8 e = 0; e < IDLE_NUM_ENCODERS; e++) {
		CHECK(idle_wave_offset(e) <= idle_wave_offset(0));
	}
}

TEST(no_encoder_lags_by_more_than_one_period) {
	// A lag of a whole period would put an encoder back in step with the one
	// leading the wave, and the diagonal would fold back on itself.
	for (u8 e = 0; e < IDLE_NUM_ENCODERS; e++) {
		CHECK(idle_wave_offset(e) < IDLE_WAVE_STEPS);
	}
}

TEST(brightness_stays_within_the_envelope) {
	for (u16 step = 0; step < IDLE_WAVE_STEPS * 4u; step++) {
		for (u8 e = 0; e < IDLE_NUM_ENCODERS; e++) {
			const u8 level = idle_wave_level(e, step);
			CHECK(level >= IDLE_LEVEL_FLOOR);
			CHECK(level <= IDLE_LEVEL_PEAK);
		}
	}
}

TEST(the_encoders_dim_but_never_go_out) {
	// The swell falls back to a floor rather than to black - the difference
	// between water moving and something blinking.
	for (u16 step = 0; step < IDLE_WAVE_STEPS * 4u; step++) {
		for (u8 e = 0; e < IDLE_NUM_ENCODERS; e++) {
			CHECK(idle_wave_level(e, step) > 0);
		}
	}
}

TEST(every_encoder_reaches_both_ends_of_the_swell) {
	for (u8 e = 0; e < IDLE_NUM_ENCODERS; e++) {
		u8 lowest	 = 255;
		u8 highest = 0;

		for (u16 step = 0; step < IDLE_WAVE_STEPS; step++) {
			const u8 level = idle_wave_level(e, step);
			if (level < lowest) {
				lowest = level;
			}
			if (level > highest) {
				highest = level;
			}
		}

		CHECK_EQ(lowest, IDLE_LEVEL_FLOOR);
		CHECK_EQ(highest, IDLE_LEVEL_PEAK);
	}
}

TEST(the_swell_repeats) {
	for (u16 step = 0; step < IDLE_WAVE_STEPS; step++) {
		for (u8 e = 0; e < IDLE_NUM_ENCODERS; e++) {
			CHECK_EQ(idle_wave_level(e, step),
							 idle_wave_level(e, (u16)(step + IDLE_WAVE_STEPS)));
		}
	}
}

TEST(the_wave_moves) {
	// The whole panel must not sit at one level - if every encoder agreed at
	// every step the offsets would be doing nothing and it would read as the
	// entire device breathing in unison.
	bool ever_differed = false;

	for (u16 step = 0; step < IDLE_WAVE_STEPS; step++) {
		const u8 first = idle_wave_level(0, step);
		for (u8 e = 1; e < IDLE_NUM_ENCODERS; e++) {
			if (idle_wave_level(e, step) != first) {
				ever_differed = true;
			}
		}
	}

	CHECK(ever_differed);
}

TEST(the_peak_travels_from_the_leader_outwards) {
	// Follow the moment each encoder peaks through one period. The encoder
	// that leads must peak first, and the far corner last.
	u16 peak_step[IDLE_NUM_ENCODERS];

	for (u8 e = 0; e < IDLE_NUM_ENCODERS; e++) {
		u8	best			= 0;
		u16 best_step = 0;
		for (u16 step = 0; step < IDLE_WAVE_STEPS; step++) {
			const u8 level = idle_wave_level(e, step);
			if (level > best) {
				best			= level;
				best_step = step;
			}
		}
		peak_step[e] = best_step;
	}

	// Relative to the leader, an encoder's peak is delayed by its offset.
	for (u8 e = 0; e < IDLE_NUM_ENCODERS; e++) {
		const u16 expected =
				(u16)((peak_step[IDLE_NUM_ENCODERS - 1u] + idle_wave_offset(e)) &
							(IDLE_WAVE_STEPS - 1u));
		CHECK_EQ(peak_step[e], expected);
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Palette ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

TEST(the_leading_encoder_holds_the_base_shade) {
	// The wave leads at the last encoder, and that is where the palette is
	// anchored, so it is the one that starts on the sea green itself.
	CHECK_EQ(idle_wave_hue(IDLE_NUM_ENCODERS - 1u), IDLE_HUE_BASE);
}

TEST(every_encoder_has_its_own_hue) {
	for (u8 a = 0; a < IDLE_NUM_ENCODERS; a++) {
		for (u8 b = (u8)(a + 1); b < IDLE_NUM_ENCODERS; b++) {
			CHECK(idle_wave_hue(a) != idle_wave_hue(b));
		}
	}
}

TEST(the_palette_stays_in_the_sea_green_band) {
	/*
		Hue runs 0-1535 for a full turn. Green sits at a quarter turn (384) and
		cyan at a third (512); sea green is between the two and a little past
		it. Holding the whole palette inside this band is what keeps it reading
		as one colour family rather than a rainbow.
	*/
	for (u8 e = 0; e < IDLE_NUM_ENCODERS; e++) {
		const u16 hue = idle_wave_hue(e);
		CHECK(hue >= 512);
		CHECK(hue <= 768);
	}
}

TEST(an_out_of_range_encoder_is_handled) {
	// Nothing should index past the panel, but the wave must not read off the
	// end of its tables if something does.
	CHECK_EQ(idle_wave_offset(IDLE_NUM_ENCODERS), 0);
	CHECK_EQ(idle_wave_offset(200), 0);
	CHECK_EQ(idle_wave_hue(IDLE_NUM_ENCODERS), IDLE_HUE_BASE);

	const u8 level = idle_wave_level(IDLE_NUM_ENCODERS, 7);
	CHECK(level >= IDLE_LEVEL_FLOOR);
	CHECK(level <= IDLE_LEVEL_PEAK);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Fade ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Build an encoder's plane words from a per-LED brightness list.
static void encode(const u8 level[IDLE_LEDS_PER_ENC], u16 planes[IDLE_BCM_PLANES]) {
	for (u8 p = 0; p < IDLE_BCM_PLANES; p++) {
		u16 state = 0;
		for (u8 b = 0; b < IDLE_LEDS_PER_ENC; b++) {
			if (level[b] & (u8)(1u << p)) {
				state |= (u16)(1u << b);
			}
		}
		planes[p] = (u16)~state;
	}
}

// And read them back out again.
static void decode(const u16 planes[IDLE_BCM_PLANES], u8 level[IDLE_LEDS_PER_ENC]) {
	for (u8 b = 0; b < IDLE_LEDS_PER_ENC; b++) {
		level[b] = 0;
	}
	for (u8 p = 0; p < IDLE_BCM_PLANES; p++) {
		const u16 lit = (u16)~planes[p];
		for (u8 b = 0; b < IDLE_LEDS_PER_ENC; b++) {
			if (lit & (u16)(1u << b)) {
				level[b] |= (u8)(1u << p);
			}
		}
	}
}

TEST(the_fade_dims_every_led_it_is_given) {
	u8	level[IDLE_LEDS_PER_ENC];
	u8	after[IDLE_LEDS_PER_ENC];
	u16 planes[IDLE_BCM_PLANES];

	// A different brightness on every LED, so a plane or bit mix-up shows up
	// as one LED taking another's value rather than as a uniform change.
	for (u8 b = 0; b < IDLE_LEDS_PER_ENC; b++) {
		level[b] = (u8)(16 * b + 15);
	}

	encode(level, planes);
	idle_dim_planes(planes);
	decode(planes, after);

	for (u8 b = 0; b < IDLE_LEDS_PER_ENC; b++) {
		CHECK(after[b] < level[b]);
		CHECK_EQ(after[b], (u8)(((u16)level[b] * IDLE_FADE_OUT_NUM) / IDLE_FADE_OUT_DEN));
	}
}

TEST(the_planes_round_trip_without_the_fade_touching_them) {
	// The encode and decode used by the fade must be exact inverses, or a
	// single dim step would corrupt the panel rather than darken it.
	u8	level[IDLE_LEDS_PER_ENC];
	u8	back[IDLE_LEDS_PER_ENC];
	u16 planes[IDLE_BCM_PLANES];

	for (u16 seed = 0; seed < 256; seed++) {
		for (u8 b = 0; b < IDLE_LEDS_PER_ENC; b++) {
			level[b] = (u8)((seed * 31u + b * 17u) & 0xFFu);
		}

		encode(level, planes);
		decode(planes, back);

		for (u8 b = 0; b < IDLE_LEDS_PER_ENC; b++) {
			CHECK_EQ(back[b], level[b]);
		}
	}
}

TEST(the_fade_reaches_black_and_stays_there) {
	u8	level[IDLE_LEDS_PER_ENC];
	u16 planes[IDLE_BCM_PLANES];

	for (u8 b = 0; b < IDLE_LEDS_PER_ENC; b++) {
		level[b] = 255;
	}
	encode(level, planes);

	// Full brightness is the worst case, and it has to be near enough dark by
	// the time the fade-out phase hands over.
	for (u8 step = 0; step < IDLE_FADE_OUT_STEPS; step++) {
		idle_dim_planes(planes);
	}

	decode(planes, level);
	for (u8 b = 0; b < IDLE_LEDS_PER_ENC; b++) {
		CHECK(level[b] < 16);
	}

	// And it must not sit at some low value forever - integer division that
	// rounded up would leave the panel faintly lit indefinitely.
	for (u8 step = 0; step < 64; step++) {
		idle_dim_planes(planes);
	}

	decode(planes, level);
	for (u8 b = 0; b < IDLE_LEDS_PER_ENC; b++) {
		CHECK_EQ(level[b], 0);
	}
}

TEST(the_fade_never_brightens_anything) {
	u8	before[IDLE_LEDS_PER_ENC];
	u8	after[IDLE_LEDS_PER_ENC];
	u16 planes[IDLE_BCM_PLANES];

	for (u8 b = 0; b < IDLE_LEDS_PER_ENC; b++) {
		before[b] = (u8)(b * 17u);
	}
	encode(before, planes);

	for (u8 step = 0; step < 40; step++) {
		decode(planes, before);
		idle_dim_planes(planes);
		decode(planes, after);

		for (u8 b = 0; b < IDLE_LEDS_PER_ENC; b++) {
			CHECK(after[b] <= before[b]);
		}
	}
}

TEST(an_already_dark_panel_stays_dark) {
	u16 planes[IDLE_BCM_PLANES];
	for (u8 p = 0; p < IDLE_BCM_PLANES; p++) {
		planes[p] = 0xFFFF; // active-low: everything off
	}

	idle_dim_planes(planes);

	for (u8 p = 0; p < IDLE_BCM_PLANES; p++) {
		CHECK_EQ(planes[p], 0xFFFF);
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Main ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

NSTEST_MAIN(RUN(the_wave_leads_at_the_last_encoder);
						RUN(the_wave_travels_along_a_row);
						RUN(each_row_starts_just_before_the_second_encoder_of_the_row_below);
						RUN(each_row_starts_after_the_one_below_it);
						RUN(the_far_corner_lags_the_most);
						RUN(no_encoder_lags_by_more_than_one_period);
						RUN(brightness_stays_within_the_envelope);
						RUN(the_encoders_dim_but_never_go_out);
						RUN(every_encoder_reaches_both_ends_of_the_swell);
						RUN(the_swell_repeats);
						RUN(the_wave_moves);
						RUN(the_peak_travels_from_the_leader_outwards);
						RUN(the_leading_encoder_holds_the_base_shade);
						RUN(every_encoder_has_its_own_hue);
						RUN(the_palette_stays_in_the_sea_green_band);
						RUN(an_out_of_range_encoder_is_handled);
						RUN(the_fade_dims_every_led_it_is_given);
						RUN(the_planes_round_trip_without_the_fade_touching_them);
						RUN(the_fade_reaches_black_and_stays_there);
						RUN(the_fade_never_brightens_anything);
						RUN(an_already_dark_panel_stays_dark);)