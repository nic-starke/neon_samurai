/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string.h>

#include "system/types.h"
#include "system/error.h"

#include "event/event.h"
#include "event/sys.h"
#include "event/io.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Data structure for general event handling
static struct event_channel* channels[EVENT_CHANNEL_NB] = {0};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int event_init(void) {
	// Register the core event channel
	int ret = event_channel_register(EVENT_CHANNEL_SYS, &sys_event_ch);
	RETURN_ON_ERR(ret);

	ret = event_channel_register(EVENT_CHANNEL_IO, &io_event_ch);
	RETURN_ON_ERR(ret);

	return 0;
}

int event_update(void) {

	for (uint i = 0; i < EVENT_CHANNEL_NB; i++) {
		int ret = event_channel_process(i);
		RETURN_ON_ERR(ret);
	}

	return 0;
}

int event_channel_register(enum event_ch ch, struct event_channel* def) {
	assert(def);
	assert(ch < EVENT_CHANNEL_NB);

	// Check if the channel is already registered
	if (channels[ch]) {
		return ERR_DUPLICATE;
	}

	// Validate the channel definition
	if (!def->queue || def->queue_size == 0 || def->data_size == 0) {
		return ERR_BAD_PARAM;
	}

	// Register the channel
	channels[ch] = def;

	return 0;
}

int event_channel_process(enum event_ch ch) {
	struct event_channel* channel = channels[ch];

	// Check if the channel is registered
	assert(channel);

	// Process all events in the queue
	for (uint i = 0; i < channel->head; ++i) {
		void* event = &channel->queue[i * channel->data_size];

		// Call the event handlers
		struct event_ch_handler* handler = channel->handlers;

		while (handler) {
			handler->handler(event);
			handler = handler->next;
		}
	}

	// Reset the queue head
	channel->head = 0;
	return 0;
}

int event_channel_subscribe(enum event_ch ch, struct event_ch_handler* new_handler) {
	assert(new_handler);
	assert(ch < EVENT_CHANNEL_NB);

	struct event_channel* channel = channels[ch];
	assert(channel);

	// If the channel is configured for only one subscriber
	// return an error.
	if (channel->onehandler) {
		return ERR_UNSUPPORTED;
	}

	struct event_ch_handler* curr = channel->handlers;

	// If the list is empty, add the handler to the start
	if (curr == NULL) {
		channel->handlers				= new_handler;
		channel->handlers->next = NULL;
		return 0;
	}

	// If the list is not empty then insert the new handler based on its
	// priority

	// For priority 0, the handler is added to the end of the list
	if (new_handler->priority == 0) {
		while (curr->next) {
			curr = curr->next;
		}
		curr->next = new_handler;
		return 0;
	}

	// If the priority of the new handler is higher than the first handler in
	// the list, then insert it at the start
	if (curr->priority < new_handler->priority) {
		new_handler->next = curr;
		channel->handlers = new_handler;
		return 0;
	}

	// For other priorities the handler is inserted into the correct position
	while (curr->next) {
		if (curr->next->priority < new_handler->priority) {
			new_handler->next = curr->next;
			curr->next				= new_handler;
			return 0;
		}
		curr = curr->next;
	}

	return ERR_UNSUPPORTED;
}

int event_channel_unsubscribe(enum event_ch ch, struct event_ch_handler* h) {
	assert(h);
	assert(ch < EVENT_CHANNEL_NB);

	struct event_channel* channel = channels[ch];
	assert(channel);

	if (channel->onehandler) {
		return ERR_UNSUPPORTED;
	}

	struct event_ch_handler* curr = channel->handlers;

	// If there are no more handlers then return
	if (curr == NULL) {
		return 0;
	}

	// If the first handler is the one to remove, then remove it
	if (curr == h) {
		channel->handlers = curr->next;
		return 0;
	}

	// Otherwise, iterate the list and remove the handler
	while (curr->next) {
		if (curr->next == h) {
			curr->next = curr->next->next;
			return 0;
		}
		curr = curr->next;
	}

	return ERR_UNSUPPORTED;
}

int event_post(enum event_ch ch, void* event) {
	assert(event);
	assert(ch < EVENT_CHANNEL_NB);

	struct event_channel* channel = channels[ch];
	assert(channel);

	// Check there is space in the channel event queue
	if (channel->head >= (channel->queue_size - 1)) {
		return ERR_NO_MEM;
	}

	// Add the event to the queue
	uint index = channel->head * channel->data_size;
	memcpy(&channel->queue[index], event, channel->data_size);
	channel->head++;

	return 0;
}

int event_post_rt(enum event_ch ch, void* event) {
	assert(event);
	assert(ch < EVENT_CHANNEL_NB);

	struct event_channel* channel = channels[ch];
	assert(channel);

	// Call the event handlers directly
	struct event_ch_handler* handler = channel->handlers;

	while (handler) {
		handler->handler(event);
		handler = handler->next;
	}

	return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
