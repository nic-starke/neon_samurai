/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <math.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

#include "lfo/lfo.h"
#include "system/hardware.h"
#include "system/time.h"
#include "hal/timer.h"
#include "io/encoder.h"
#include "system/utility.h"
#include "console/console.h"
#include "event/event.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define LFO_PHASE_MAX										0xFFFFFFFFU
#define LFO_SAMPLE_RATE_HZ							1000
#define LFO_SAMPLE_INTERVAL_MS					(1000U / LFO_SAMPLE_RATE_HZ)
#define LFO_MANUAL_PAUSE_MS							200
#define LFO_DISPLAY_REFRESH_INTERVAL_MS 33

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static i16	lfo_waveform_sine(u32 phase);
static i16	lfo_waveform_triangle(u32 phase);
static i16	lfo_waveform_square(u32 phase);
static i16	lfo_waveform_sawtooth_up(u32 phase);
static i16	lfo_waveform_sawtooth_down(u32 phase);
static i16	lfo_waveform_sample_hold(u8 lfo_idx, u32 phase);
static void lfo_update_vmap_position(u8 lfo_idx);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */

struct lfo_state gLFOs[MAX_LFOS];

static u32 last_sample_time;
static u32 next_sample_time;
static u32 g_lfo_last_display_update_time[MAX_LFOS] = {0};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static const i16 sine_table[256] PROGMEM = {
		0,			804,		1608,		2410,		3212,		4011,		4808,		5602,		6393,
		7179,		7962,		8739,		9512,		10278,	11039,	11793,	12539,	13279,
		14010,	14732,	15446,	16151,	16846,	17530,	18204,	18868,	19519,
		20159,	20787,	21403,	22005,	22594,	23170,	23731,	24279,	24811,
		25329,	25832,	26319,	26790,	27245,	27683,	28105,	28510,	28898,
		29268,	29621,	29956,	30273,	30571,	30852,	31113,	31356,	31580,
		31785,	31971,	32137,	32285,	32412,	32521,	32609,	32678,	32728,
		32757,	32767,	32757,	32728,	32678,	32609,	32521,	32412,	32285,
		32137,	31971,	31785,	31580,	31356,	31113,	30852,	30571,	30273,
		29956,	29621,	29268,	28898,	28510,	28105,	27683,	27245,	26790,
		26319,	25832,	25329,	24811,	24279,	23731,	23170,	22594,	22005,
		21403,	20787,	20159,	19519,	18868,	18204,	17530,	16846,	16151,
		15446,	14732,	14010,	13279,	12539,	11793,	11039,	10278,	9512,
		8739,		7962,		7179,		6393,		5602,		4808,		4011,		3212,		2410,
		1608,		804,		0,			-804,		-1608,	-2410,	-3212,	-4011,	-4808,
		-5602,	-6393,	-7179,	-7962,	-8739,	-9512,	-10278, -11039, -11793,
		-12539, -13279, -14010, -14732, -15446, -16151, -16846, -17530, -18204,
		-18868, -19519, -20159, -20787, -21403, -22005, -22594, -23170, -23731,
		-24279, -24811, -25329, -25832, -26319, -26790, -27245, -27683, -28105,
		-28510, -28898, -29268, -29621, -29956, -30273, -30571, -30852, -31113,
		-31356, -31580, -31785, -31971, -32137, -32285, -32412, -32521, -32609,
		-32678, -32728, -32757, -32767, -32757, -32728, -32678, -32609, -32521,
		-32412, -32285, -32137, -31971, -31785, -31580, -31356, -31113, -30852,
		-30571, -30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683,
		-27245, -26790, -26319, -25832, -25329, -24811, -24279, -23731, -23170,
		-22594, -22005, -21403, -20787, -20159, -19519, -18868, -18204, -17530,
		-16846, -16151, -15446, -14732, -14010, -13279, -12539, -11793, -11039,
		-10278, -9512,	-8739,	-7962,	-7179,	-6393,	-5602,	-4808,	-4011,
		-3212,	-2410,	-1608,	-804};

static u32 g_lfo_last_midi_update_time[MAX_LFOS] = {0};
static i16 g_sample_hold_values[MAX_LFOS]				 = {0};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void lfo_init(void) {
	for (u8 i = 0; i < MAX_LFOS; i++) {
		gLFOs[i].active						= false;
		gLFOs[i].waveform					= LFO_WAVE_NONE;
		gLFOs[i].depth						= LFO_DEFAULT_DEPTH;
		gLFOs[i].rate_hz					= LFO_DEFAULT_RATE_HZ;
		gLFOs[i].phase_acc				= 0;
		gLFOs[i].phase_increment	= 0;
		gLFOs[i].last_value				= 0;
		gLFOs[i].assigned_bank		= 0xFF;
		gLFOs[i].assigned_encoder = 0xFF;
		gLFOs[i].assigned_vmap		= 0xFF;
		gLFOs[i].base_position		= 0;
		gLFOs[i].last_output_pos	= 0;
		g_sample_hold_values[i]		= 0;
	}
	for (u8 i = 0; i < MAX_LFOS; i++) {
		lfo_set_rate(i, LFO_DEFAULT_RATE_HZ);
	}

	last_sample_time = systime_ms();
	next_sample_time = last_sample_time + LFO_SAMPLE_INTERVAL_MS;

	console_puts_p(PSTR("LFO system initialized\r\n"));
}

void lfo_update(void) {
	u32 current_time = systime_ms();

	if (current_time < next_sample_time) {
		return;
	}

	next_sample_time += LFO_SAMPLE_INTERVAL_MS;

	for (int i = 0; i < MAX_LFOS; i++) {
		if (!gLFOs[i].active || gLFOs[i].waveform == LFO_WAVE_NONE) {
			continue;
		}

		if (gLFOs[i].assigned_bank == 0xFF || gLFOs[i].assigned_encoder == 0xFF ||
				gLFOs[i].assigned_vmap == 0xFF) {
			continue;
		}

		gLFOs[i].phase_acc += gLFOs[i].phase_increment;

		i16 raw_value = 0;
		switch (gLFOs[i].waveform) {
			case LFO_WAVE_SINE:
				raw_value = lfo_waveform_sine(gLFOs[i].phase_acc);
				break;
			case LFO_WAVE_TRIANGLE:
				raw_value = lfo_waveform_triangle(gLFOs[i].phase_acc);
				break;
			case LFO_WAVE_SQUARE:
				raw_value = lfo_waveform_square(gLFOs[i].phase_acc);
				break;
			case LFO_WAVE_SAWTOOTH_UP:
				raw_value = lfo_waveform_sawtooth_up(gLFOs[i].phase_acc);
				break;
			case LFO_WAVE_SAWTOOTH_DN:
				raw_value = lfo_waveform_sawtooth_down(gLFOs[i].phase_acc);
				break;
			case LFO_WAVE_SAMPLE_HOLD:
				raw_value = lfo_waveform_sample_hold(i, gLFOs[i].phase_acc);
				break;
			default: raw_value = 0; break;
		}

		i16 new_value = (i16)((raw_value * (i32)gLFOs[i].depth) / 100);

		if (gLFOs[i].last_value == new_value) {
			continue;
		}
		gLFOs[i].last_value = new_value;

		lfo_update_vmap_position(i);
	}
}

void lfo_cycle_waveform(u8 lfo_idx) {
	if (lfo_idx >= MAX_LFOS) {
		return;
	}

	gLFOs[lfo_idx].waveform =
			(enum lfo_waveform)((gLFOs[lfo_idx].waveform + 1) % LFO_WAVE_COUNT);

	gLFOs[lfo_idx].active = (gLFOs[lfo_idx].waveform != LFO_WAVE_NONE);

	if (gLFOs[lfo_idx].active) {
		gLFOs[lfo_idx].phase_acc = 0;
	}

	const char* wave_names[] = {"OFF",				"SINE",				 "TRIANGLE",
															"SQUARE",			"SAWTOOTH_UP", "SAWTOOTH_DN",
															"SAMPLE_HOLD"};
	console_puts_p(PSTR("LFO "));
	console_put_uint(lfo_idx);
	console_puts_p(PSTR(": Waveform set to "));
	console_puts(wave_names[gLFOs[lfo_idx].waveform]);
	console_puts_p(PSTR("\r\n"));
}

void lfo_set_depth(u8 lfo_idx, i8 depth) {
	if (lfo_idx >= MAX_LFOS) {
		return;
	}
	gLFOs[lfo_idx].depth = CLAMP(depth, -100, 100);

	console_puts_p(PSTR("LFO "));
	console_put_uint(lfo_idx);
	console_puts_p(PSTR(": Depth set to "));
	console_put_int(gLFOs[lfo_idx].depth);
	console_puts_p(PSTR("%\r\n"));
}

i8 lfo_get_depth(u8 lfo_idx) {
	if (lfo_idx >= MAX_LFOS) {
		return 0;
	}
	return gLFOs[lfo_idx].depth;
}

enum lfo_waveform lfo_get_waveform(u8 lfo_idx) {
	if (lfo_idx >= MAX_LFOS) {
		return LFO_WAVE_NONE;
	}
	return gLFOs[lfo_idx].waveform;
}

void lfo_set_rate(u8 lfo_idx, float rate_hz) {
	if (lfo_idx >= MAX_LFOS) {
		return;
	}
	gLFOs[lfo_idx].rate_hz = CLAMP(rate_hz, 0.01f, 10.0f);

	u32 rate_fixed = (u32)(gLFOs[lfo_idx].rate_hz * 1000.0f);

	u64 temp_calc =
			((u64)rate_fixed * LFO_PHASE_MAX) / (LFO_SAMPLE_RATE_HZ * 1000UL);
	gLFOs[lfo_idx].phase_increment = (u32)temp_calc;
}

float lfo_get_rate(u8 lfo_idx) {
	if (lfo_idx >= MAX_LFOS) {
		return 0.0f;
	}
	return gLFOs[lfo_idx].rate_hz;
}

void lfo_assign(u8 lfo_idx, u8 bank, u8 encoder, u8 vmap_idx) {
	if (lfo_idx >= MAX_LFOS || bank >= NUM_ENC_BANKS || encoder >= NUM_ENCODERS ||
			vmap_idx >= NUM_VMAPS_PER_ENC) {
		return;
	}

	gLFOs[lfo_idx].assigned_bank		= bank;
	gLFOs[lfo_idx].assigned_encoder = encoder;
	gLFOs[lfo_idx].assigned_vmap		= vmap_idx;

	struct encoder* enc						 = &gENCODERS[bank][encoder];
	struct virtmap* vmap					 = &enc->vmaps[vmap_idx];
	gLFOs[lfo_idx].base_position	 = vmap->curr_pos;
	gLFOs[lfo_idx].last_output_pos = vmap->curr_pos;

	console_puts_p(PSTR("LFO "));
	console_put_uint(lfo_idx);
	console_puts_p(PSTR(" assigned to bank "));
	console_put_uint(bank);
	console_puts_p(PSTR(", encoder "));
	console_put_uint(encoder);
	console_puts_p(PSTR(", vmap "));
	console_put_uint(vmap_idx);
	console_puts_p(PSTR("\r\n"));
}

bool lfo_is_assigned_to(u8 lfo_idx, u8 bank, u8 encoder, u8 vmap_idx) {
	if (lfo_idx >= MAX_LFOS) {
		return false;
	}

	return (gLFOs[lfo_idx].assigned_bank == bank &&
					gLFOs[lfo_idx].assigned_encoder == encoder &&
					gLFOs[lfo_idx].assigned_vmap == vmap_idx);
}

bool lfo_is_assigned_to_current_bank(u8 lfo_idx) {
	if (lfo_idx >= MAX_LFOS) {
		return false;
	}
	return (gLFOs[lfo_idx].active && gLFOs[lfo_idx].waveform != LFO_WAVE_NONE &&
					gLFOs[lfo_idx].assigned_bank == gRT.curr_bank);
}

bool lfo_encoder_has_active_lfo_on_current_bank(u8 encoder_idx) {
	if (encoder_idx >= NUM_ENCODERS) {
		return false;
	}

	// Check if any LFO is assigned to this encoder on the current bank
	for (u8 i = 0; i < MAX_LFOS; i++) {
		if (gLFOs[i].active && gLFOs[i].waveform != LFO_WAVE_NONE &&
				gLFOs[i].assigned_bank == gRT.curr_bank &&
				gLFOs[i].assigned_encoder == encoder_idx) {
			return true;
		}
	}
	return false;
}

void lfo_notify_manual_change(u8 bank, u8 encoder, u8 vmap_idx,
															u8 new_position) {
	for (u8 i = 0; i < MAX_LFOS; i++) {
		if (gLFOs[i].active && gLFOs[i].assigned_bank == bank &&
				gLFOs[i].assigned_encoder == encoder &&
				gLFOs[i].assigned_vmap == vmap_idx) {

			gLFOs[i].base_position = new_position;

			break;
		}
	}
}

void lfo_unassign(u8 lfo_idx) {
	if (lfo_idx >= MAX_LFOS) {
		return;
	}

	if (gLFOs[lfo_idx].assigned_bank != 0xFF &&
			gLFOs[lfo_idx].assigned_encoder != 0xFF &&
			gLFOs[lfo_idx].assigned_vmap != 0xFF) {

		struct encoder* enc = &gENCODERS[gLFOs[lfo_idx].assigned_bank]
																		[gLFOs[lfo_idx].assigned_encoder];
		struct virtmap* vmap = &enc->vmaps[gLFOs[lfo_idx].assigned_vmap];
		vmap->curr_pos			 = gLFOs[lfo_idx].base_position;

		enc->update_display = 1;
	}

	gLFOs[lfo_idx].assigned_bank		= 0xFF;
	gLFOs[lfo_idx].assigned_encoder = 0xFF;
	gLFOs[lfo_idx].assigned_vmap		= 0xFF;
	gLFOs[lfo_idx].base_position		= 0;
	gLFOs[lfo_idx].last_output_pos	= 0;

	gLFOs[lfo_idx].waveform = LFO_WAVE_NONE;
	gLFOs[lfo_idx].active		= false;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void lfo_update_vmap_position(u8 lfo_idx) {
	if (lfo_idx >= MAX_LFOS || !gLFOs[lfo_idx].active) {
		return;
	}

	struct lfo_state* lfo = &gLFOs[lfo_idx];

	struct encoder* enc	 = &gENCODERS[lfo->assigned_bank][lfo->assigned_encoder];
	struct virtmap* vmap = &enc->vmaps[lfo->assigned_vmap];

	i16 new_pos = (i16)lfo->base_position + lfo->last_value;

	new_pos = CLAMP(new_pos, vmap->position.start, vmap->position.stop);

	u8 new_pos_u8 = (u8)new_pos;

	if (new_pos_u8 != lfo->last_output_pos) {
		lfo->last_output_pos = new_pos_u8;
		vmap->curr_pos			 = new_pos_u8;

		u32 current_time = systime_ms();

		if (current_time - g_lfo_last_display_update_time[lfo_idx] >=
				LFO_DISPLAY_REFRESH_INTERVAL_MS) {
			enc->update_display											= 1;
			g_lfo_last_display_update_time[lfo_idx] = current_time;
		}

		if (current_time - g_lfo_last_midi_update_time[lfo_idx] >=
				gCONFIG.midi_throttle_time) {
			g_lfo_last_midi_update_time[lfo_idx] = current_time;

			// struct io_event midi_update_evt;
			// midi_update_evt.type										 = EVT_IO_VMAP_NEEDS_MIDI_UPDATE;
			// midi_update_evt.data.vmap_midi_data.bank = lfo->assigned_bank;
			// midi_update_evt.data.vmap_midi_data.encoder_idx = lfo->assigned_encoder;
			// midi_update_evt.data.vmap_midi_data.vmap_idx		= lfo->assigned_vmap;
			// event_post(EVENT_CHANNEL_GEN, &midi_update_evt);
		}
	}
}

static i16 lfo_waveform_sine(u32 phase) {
	u8 index = (u8)(phase >> 24);

	i16 value = (i16)pgm_read_word(&sine_table[index]);

	return (i16)(((i32)value * (i32)ENC_MAX) / 32767L);
}

static i16 lfo_waveform_triangle(u32 phase) {
	u8 pos = (u8)(phase >> 24);

	if (pos < 128) {
		return (i16)(-(i32)ENC_MAX + ((i32)pos * 2 * (i32)ENC_MAX) / 127);
	} else {
		return (i16)((i32)ENC_MAX - (((i32)pos - 128) * 2 * (i32)ENC_MAX) / 127);
	}
}

static i16 lfo_waveform_square(u32 phase) {
	return (phase < 0x80000000UL) ? ENC_MAX : -(i16)ENC_MAX;
}

static i16 lfo_waveform_sawtooth_up(u32 phase) {
	u32 scaled = phase / (LFO_PHASE_MAX / (2UL * (u32)ENC_MAX));
	return (i16)(-(i32)ENC_MAX + (i16)scaled);
}

static i16 lfo_waveform_sawtooth_down(u32 phase) {
	u32 scaled = phase / (LFO_PHASE_MAX / (2UL * (u32)ENC_MAX));
	return (i16)((i32)ENC_MAX - (i16)scaled);
}

static i16 lfo_waveform_sample_hold(u8 lfo_idx, u32 phase) {
	static u32 last_phase[MAX_LFOS] = {0};

	if ((phase >> 24) != (last_phase[lfo_idx] >> 24)) {
		g_sample_hold_values[lfo_idx] =
				(i16)((rand() % (2 * ENC_MAX + 1)) - ENC_MAX);
		last_phase[lfo_idx] = phase;
	}

	return g_sample_hold_values[lfo_idx];
}

u8 lfo_get_base_position_for_manual_control(u8 bank, u8 encoder, u8 vmap_idx) {
	for (u8 i = 0; i < MAX_LFOS; i++) {
		if (gLFOs[i].active && gLFOs[i].assigned_bank == bank &&
				gLFOs[i].assigned_encoder == encoder &&
				gLFOs[i].assigned_vmap == vmap_idx) {

			return gLFOs[i].base_position;
		}
	}

	struct encoder* enc	 = &gENCODERS[bank][encoder];
	struct virtmap* vmap = &enc->vmaps[vmap_idx];
	return vmap->curr_pos;
}
