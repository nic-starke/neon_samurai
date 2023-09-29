/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/system.h"
#include "system/os.h"
#include "drivers/encoder.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
  ENCODER_MODE_DISABLED,
  ENCODER_MODE_MIDI_CC,
  ENCODER_MODE_MIDI_REL_CC,
  ENCODER_MODE_MIDI_NOTE,

  ENCODER_MODE_NB,
} encoder_mode_e;

typedef struct {
  uint8_t channel; // midi channel
  uint8_t value;   // cc, or note value
} encoder_midi_cfg_t;

typedef struct {
  uint16_t val_min;
  uint16_t val_max;
  uint16_t val_start;
  uint16_t val_stop;
  uint8_t  acceleration;
  uint8_t  mode;
  union {
    encoder_midi_cfg_t midi;
  };
} encoder_cfg_t;

typedef struct {
  uint16_t      curr_val;
  uint16_t      prev_val;
  encoder_cfg_t cfg;
  bool          enabled;
} encoder_ctx_t;

typedef struct {
  uint8_t           count;
  encoder_ctx_t*    sw_ctx; // pointer to array of contexts
  hw_encoder_ctx_t* hw_ctx; // pointer to array of contexts
} encoder_desc_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int  encoder_init(encoder_desc_t* desc);
void encoder_update(void);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
