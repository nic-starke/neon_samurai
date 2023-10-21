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
	u8											 priority;
	void										 (*handler)(void* event);
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
	uint								data_size;	// Size of data for a single event (for memcpy)
	event_ch_handler_s* handlers;		// Link list of handlers
	bool onehandler; // Set true if handlers is a single handler for all events
	uint head;			 // (private)
} event_channel_s;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int event_init(void);

int event_channel_register(event_ch_e ch, event_channel_s* def);
int event_channel_process(event_ch_e ch);

int event_channel_subscribe(event_ch_e ch, event_ch_handler_s* new_handler);
int event_channel_unsubscribe(event_ch_e ch, event_ch_handler_s* h);

int event_post(event_ch_e ch, void* event);
int event_post_rt(event_ch_e ch, void* event);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
