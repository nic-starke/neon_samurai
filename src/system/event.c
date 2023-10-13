/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/system.h"
#include "system/os.h"
#include "system/event.h"
#include "system/types.h"
#include "system/utility.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define EVENT_THREAD_STACK_SIZE (128)
#define EVENT_THREAD_PRIORITY   (1)

// Number of messages that can be stored in the queue
#define EVQ_MAX_MSG (128)

// Number of bytes required to store EVQ_MAX_MSG messages
#define EVQ_SIZE (EVENT_MSG_SIZE * EVQ_MAX_MSG)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void event_thread(u32 data);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static os_queue_t evt_queue;
static u8         evt_queue_buf[EVQ_SIZE];

static u8 evt_thread_stack[EVENT_THREAD_STACK_SIZE];

static os_thread_t evt_thread = {
    .next       = NULL,
    .priority   = EVENT_THREAD_PRIORITY,
    .stack      = evt_thread_stack,
    .stack_size = sizeof(evt_thread_stack),
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int event_init(void) {
  int ret = os_thread_start(&evt_thread, event_thread, NULL);
  RETURN_ON_ERR(ret);

  ret = os_queue_create(&evt_queue, &evt_queue_buf[0], EVENT_MSG_SIZE,
                        EVQ_MAX_MSG);
  RETURN_ON_ERR(ret);

  return ret;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Thread Function ~~~~~~~~~~~~~~~~~~~~~~~~~ */

void event_thread(u32 data) {
  event_t evt;
  int     ret;

  while (1) {
    ret = os_queue_get(&evt_queue, OS_TIMEOUT_BLOCK, (u8*)&evt);
    if (ret != 0) {
      continue;
    }

    // Process the event..
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
