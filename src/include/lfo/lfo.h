#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define MAX_LFOS            16
#define LFO_DISABLED        0
#define LFO_DEFAULT_DEPTH   50
#define LFO_DEFAULT_RATE_HZ 1

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum lfo_waveform {
    LFO_WAVE_NONE = 0,
    LFO_WAVE_SINE,
    LFO_WAVE_TRIANGLE,
    LFO_WAVE_SQUARE,
    LFO_WAVE_SAWTOOTH_UP,
    LFO_WAVE_SAWTOOTH_DN,
    LFO_WAVE_SAMPLE_HOLD,

    LFO_WAVE_COUNT
};

struct lfo_state {
    enum lfo_waveform waveform;
    bool  active;
    i8    depth;
    float rate_hz;
    u32   phase_acc;
    i16   last_value;
    u32   phase_increment;

    u8    assigned_bank;
    u8    assigned_encoder;
    u8    assigned_vmap;
    u8    base_position;
    u8    last_output_pos;
};

extern struct lfo_state gLFOs[MAX_LFOS]; // Added extern declaration for g_lfos

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void lfo_init(void);

void lfo_update(void);

void lfo_cycle_waveform(u8 lfo_idx);

void lfo_set_depth(u8 lfo_idx, i8 depth);

i8 lfo_get_depth(u8 lfo_idx);

enum lfo_waveform lfo_get_waveform(u8 lfo_idx);

void lfo_set_rate(u8 lfo_idx, float rate_hz);

float lfo_get_rate(u8 lfo_idx);

void lfo_assign(u8 lfo_idx, u8 bank, u8 encoder, u8 vmap_idx);

bool lfo_is_assigned_to(u8 lfo_idx, u8 bank, u8 encoder, u8 vmap_idx);

bool lfo_is_assigned_to_current_bank(u8 lfo_idx);

bool lfo_encoder_has_active_lfo_on_current_bank(u8 encoder_idx);

void lfo_notify_manual_change(u8 bank, u8 encoder, u8 vmap_idx, u8 new_position);

void lfo_unassign(u8 lfo_idx);

u8 lfo_get_base_position_for_manual_control(u8 bank, u8 encoder, u8 vmap_idx);
