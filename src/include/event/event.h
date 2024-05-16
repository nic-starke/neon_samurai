/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "core/core_types.h"
#include "core/core_utility.h"
#include "core/core_error.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Macro to declare a static event handler.
 *
 * @param p Priority (0-255).
 * @param n Name of the structure
 * @param h Pointer to handler function.
 */
#define EVT_HANDLER(p, n, h)                                                   \
	static event_ch_handler_s n = {                                              \
			.priority = p,                                                           \
			.handler	= h,                                                           \
			.next			= NULL,                                                        \
	}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
	EVENT_CHANNEL_CORE,			// Reserved for system events
	EVENT_CHANNEL_IO,				// IO events
	EVENT_CHANNEL_MIDI_IN,	// MIDI events (rx)
	EVENT_CHANNEL_MIDI_OUT, // MIDI events (tx)

	EVENT_CHANNEL_NB,
} event_ch_e;

/**
 * @brief
 * Priority determines the order in which handlers are called.
 * 0 means the handlers are called in the order they were registered.
 * 1 means max priority, and will be called first.
 * 255 means min priority, and will be called last.
 *
 * Most of the time 0 priority is fine.
 * Events that must be handled sychronously (realtime priority) can
 * be posted with the event_post_rt() function - this will block
 * and call each handler in turn immediately.
 */
typedef struct event_ch_handler {
	u8 priority;
	int (*handler)(void* event);
	struct event_ch_handler* next;
} event_ch_handler_s;

/**
 * @brief Event channel structure.
 * The queue parameter is a pointer to a statically allocated buffer.
 * The buffer size is upto the user, if there are few events then
 * a small buffer is fine. Larger queues increase the latency of
 * events being handled when many events are posted.
 *
 * The handlers parameter is a linked-list of event handlers for this channel.
 *
 * If only one handler is required for all events in the channel then
 * the onehandler parameter can be set to true.
 *
 * The size parameter is the size of the queue buffer.
 *
 * The head parameter is the index of the next event to be handled.
 * It is private and should not be modified by the user.
 */
typedef struct {
	u8*									queue;			// Statically allocated queue buffer
	uint								queue_size; // The size of the array (number of messages)
	uint								data_size; // Size of data for a single event (for memcpy)
	event_ch_handler_s* handlers;	 // Link list of handlers
	bool onehandler; // Set true if handlers is a single handler for all events
	uint head;			 // (private)
} event_channel_s;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Initialise the event-handling system.
 *
 * @return int General error code.
 */
int event_init(void);

/**
 * @brief Processes all queued events in all event channels.
 * Event handlers for each event are called in priority order.
 *
 * @return int General error code.
 */
int event_update(void);

/**
 * @brief Registers a new event channel.
 * @warning The event channel must have an associated enum
 * assigned - see event_ch_e.
 *
 * @param ch The enum for the event channel.
 * @param def Pointer to the channel definition.
 *
 * @return int General error code.
 * @retval ERR_DUPLICATE if enum value is not unique.
 * @retval ERR_BAD_PARAM if channel definition is incorrect.
 * @retval 0 on success.
 */
int event_channel_register(event_ch_e ch, event_channel_s* def);

/**
 * @brief Processes all events for a single channel.
 * Normally there will be no need to do this, but it is available
 * if required.
 *
 * @param ch Enum of the event channel.
 * @return int General error code.
 */
int event_channel_process(event_ch_e ch);

/**
 * @brief Subscribe (assign) a new event handler to an existing event channel.
 *
 * @param ch Enum of the event channel.
 * @param new_handler Pointer to the new event handler.
 *
 * @return int General error code.
 * @retval ERR_UNSUPPORTED cannot assign a new handler to this event channel.
 * @retval 0 on success.
 */
int event_channel_subscribe(event_ch_e ch, event_ch_handler_s* new_handler);

/**
 * @brief Unsubscribe an event handler from an existing event channel.
 *
 * @param ch Enum of the event channel.
 * @param h Pointer to the existing event handler.
 * @return int General error code.
 * @retval 0 on success.
 */
int event_channel_unsubscribe(event_ch_e ch, event_ch_handler_s* h);

/**
 * @brief Post an event to an event queue.
 * The event will be handled later when the event_update function
 * is called as part of the main system loop.
 *
 * This function copies the event object into the queue, the
 * original object can be safely freed (if on the heap).
 *
 * @param ch Enum of the event channel.
 * @param event Pointer to the event.
 * @return int General error code.
 * @retval ERR_NO_MEM queue is full - cannot add a new event.
 * @retval 0 on success.
 */
int event_post(event_ch_e ch, void* event);

/**
 * @brief Process an event immediately (real-time)
 *
 * @param ch Enum of the event channel.
 * @param event Pointer to the event.
 * @return int General error code.
 * @retval 0 on success.
 */
int event_post_rt(event_ch_e ch, void* event);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
