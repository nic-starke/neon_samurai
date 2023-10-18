/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "core/core_types.h"
#include "core/core_utility.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define event_DATA_SIZE (8) // Bytes
#define event_MSG_SIZE  (sizeof(event_t))

#define CHECK_SIZE(x)                                                          \
  STATIC_ASSERT(sizeof(x) <= event_DATA_SIZE, "Event data too large")

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum {
  EVT_ENCODER_ROTATION,
  EVT_ENCODER_SWITCH_STATE,

  EVT_MAX_BRIGHTNESS,

  EVT_SYS_RESERVED,
  EVT_ID_MAX,
};

typedef struct {
  u16 current_value;
  u8  encoder_index;
} encoder_event_s;
CHECK_SIZE(encoder_event_s);

typedef struct {
  u8 state;
  u8 switch_index;
} switch_event_s;
CHECK_SIZE(switch_event_s);

typedef struct {
  u16 id;

  union {
    encoder_event_s encoder;
    switch_event_s  sw;
    u8              max_brightness;
    u8              raw[event_DATA_SIZE];
  } data;
} event_s;

// Typedef for a callback function that handles an event
typedef void (*event_handler_fp)(event_s* event);

// Event handler structure
typedef struct event_handler_sp {
  // Priority determines the order in which handlers are called
  // 0 means the handlers are called in the order they subscribed to an event.
  // 1 means max priority, and will be called first.
  // 255 means min priority, and will be called last.

  u8                       priority; // (public) Priority of the handler.
  event_handler_fp         handler; // (public) Pointer to the handler function.
  struct event_handler_sp* next;    // (private)
} event_handler_s;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int event_init(void);
int event_post(event_s* event);
int event_subscribe(event_handler_s* const handler, u16 event_id);
int event_unsubscribe(event_handler_s* const handler, u16 event_id);
int event_process(void);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
