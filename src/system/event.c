/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string.h>

#include "system/system.h"
#include "system/os.h"
#include "system/event.h"
#include "system/types.h"
#include "system/utility.h"

#include "board/djtt/midifighter.h"

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

static event_handler_t* handlers[EVENT_ID_MAX];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int event_init(void) {
  int ret = os_thread_start(&evt_thread, event_thread, 0);
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
    memset(&evt, 0, sizeof(evt));
    ret = os_queue_get(&evt_queue, OS_TIMEOUT_BLOCK, (u8*)&evt);
    if (ret != 0) {
      continue;
    }

    // Call all handlers for this event
    event_handler_t* h = handlers[evt.id];
    while (h) {
      h->handler(&evt);
      h = h->next;
    }
  }
}

int event_post(event_t* evt, int os_timeout) {
  assert(evt);
  return os_queue_put(&evt_queue, os_timeout, (u8*)evt);
}

int event_subscribe(event_handler_t* const handler, u16 event_id) {
  assert(handler);
  if (event_id >= EVENT_ID_MAX) {
    return ERR_BAD_PARAM;
  }

  // Set the handler next pointer to NULL
  handler->next = NULL;

  // Iterate the linked list of the handler for the given event_id, add new
  // handler to the end
  event_handler_t* h = handlers[event_id];

  // Disable interrupts while we modify the list
  cli();

  // If the list is empty, add the handler to the start
  if (h == NULL) {
    handlers[event_id] = handler;
    goto done;
  }

  // If the list is not empty then insert the new handler based on its priority
  // Priority of 0 means the handler is added to the end of the list
  if (handler->priority == 0) {
    while (h->next) {
      h = h->next;
    }
    h->next = handler;
    goto done;
  }

  // If the new handler has a higher priority than the first handler in the
  // list, then insert it at the start, and update the local pointer
  if (h->priority < handler->priority) {
    handler->next      = h;
    handlers[event_id] = handler;
    goto done;
  }

  // Otherwise, iterate the list and insert the new handler based on priority
  while (h->next) {
    if (h->next->priority < handler->priority) {
      handler->next = h->next;
      h->next       = handler;
      goto done;
    }
    h = h->next;
  }

done:
  // Re-enable interrupts
  sei();
  return 0;
}

int event_unsubscribe(event_handler_t* const handler, u16 event_id) {
  return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
