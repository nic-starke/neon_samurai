/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/system.h"
#include "system/types.h"
#include "system/utility.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define EVENT_DATA_SIZE (8) // Bytes
#define EVENT_MSG_SIZE  (sizeof(event_t))

#define CHECK_SIZE(x)                                                          \
  STATIC_ASSERT(sizeof(x) <= EVENT_DATA_SIZE, "Event data too large")

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum {
  EVENT_ID_RESERVED = 50,
  EVENT_ID_ENCODER_CHANGE,

  EVENT_ID_MAX,
};

typedef struct {
  u16 current_value;
  u8  encoder_index;
} encoder_event_t;

CHECK_SIZE(encoder_event_t);

typedef struct {
  u16 id;

  union {
    encoder_event_t encoder;
    u8              raw[EVENT_DATA_SIZE];
  } data;
} event_t;

// Typedef for a callback function that handles an event
typedef void (*event_handler_fp)(event_t* event);

// Event handler structure
typedef struct event_handler {
  // Priority determines the order in which handlers are called
  // 0 means the handlers are called in the order they subscribed to an event.
  // 1 means max priority, and will be called first.
  // 255 means min priority, and will be called last.

  u8                    priority; // (public) Priority of the handler.
  event_handler_fp      handler;  // (public) Pointer to the handler function.
  struct event_handler* next;     // (private)
} event_handler_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int event_init(void);
int event_post(event_t* event, int os_timeout);
int event_subscribe(event_handler_t* const handler, u16 event_id);
int event_unsubscribe(event_handler_t* const handler, u16 event_id);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
