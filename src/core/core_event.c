/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string.h>

#include "core/core_event.h"
#include "core/core_types.h"
#include "core/core_error.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Number of messages that can be stored in the queue
#define EVQ_MAX_MSG (32)

// Number of bytes required to store EVQ_MAX_MSG messages
#define EVQ_SIZE (EVENT_MSG_SIZE * EVQ_MAX_MSG)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static event_handler_s* handlers[EVT_ID_MAX];

static event_s evt_queue[EVQ_MAX_MSG];
static u8      curr_evt = 0;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int event_init(void) {
  return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Thread Function ~~~~~~~~~~~~~~~~~~~~~~~~~ */

int event_process(void) {

  // Iterate through the event queue and call each handler
  for (u8 i = 0; i < curr_evt; i++) {
    // Call all handlers for this event
    event_handler_s* h = handlers[evt_queue[i].id];
    while (h) {
      h->handler(&evt_queue[i]);
      h = h->next;
    }
  }

  curr_evt = 0;
}

int event_post(event_s* evt, int os_timeout) {
  assert(evt);
  if (curr_evt >= EVQ_MAX_MSG) {
    return ERR_NO_MEM;
  }

  memcpy(&evt_queue[curr_evt++], evt, sizeof(event_s));
  return 0;
}

int event_subscribe(event_handler_s* const handler, u16 event_id) {
  assert(handler);
  if (event_id >= EVT_ID_MAX) {
    return ERR_BAD_PARAM;
  }

  // Set the handler next pointer to NULL
  handler->next = NULL;

  // Iterate the linked list of the handler for the given event_id, add new
  // handler to the end
  event_handler_s* h = handlers[event_id];

  // If the list is empty, add the handler to the start
  if (h == NULL) {
    handlers[event_id] = handler;
    goto done;
  }

  // If the list is not empty then insert the new handler based on its
  // priority Priority of 0 means the handler is added to the end of the list
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
  return 0;
}

int event_unsubscribe(event_handler_s* const handler, u16 event_id) {
  // return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~
 */
