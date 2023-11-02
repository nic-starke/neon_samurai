/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string.h>

#include "core/core_types.h"
#include "core/core_error.h"

#include "event/event.h"
#include "event/events_core.h"
#include "event/events_io.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Data structure for general event handling
static event_channel_s* channels[EVENT_CHANNEL_NB] = {0};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int event_init(void) {
	// Register the core event channel
	int ret = event_channel_register(EVENT_CHANNEL_CORE, &core_event_ch);
	RETURN_ON_ERR(ret);

	ret = event_channel_register(EVENT_CHANNEL_IO, &io_event_ch);
	RETURN_ON_ERR(ret);

	return 0;
}

int event_channel_register(event_ch_e ch, event_channel_s* def) {
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

/**
 * @brief Processes all messages in the event queue for the specified channel.
 * 
 * @param ch The channel to process.
 * @return int 0 on success, otherwise an error code.
 */
int event_channel_process(event_ch_e ch) {
	event_channel_s* channel = channels[ch];

	// Check if the channel is registered
	assert(channel);

	uint num_msg = channel->head;

	// Process all events in the queue
	for (uint i = 0; i < num_msg; ++i) {
		void* event = &channel->queue[i * channel->data_size];

		// Call the event handlers
		event_ch_handler_s* handler = channel->handlers;

		while (handler) {
			handler->handler(event);
			handler = handler->next;
		}
	}

	// Reset the queue head
	channel->head = 0;
	return 0;
}

int event_channel_subscribe(event_ch_e ch, event_ch_handler_s* new_handler) {
	assert(new_handler);
	assert(ch < EVENT_CHANNEL_NB);

	event_channel_s* channel = channels[ch];

	// Check if the channel is registered
	if (!channel) {
		return ERR_NULL_PTR;
	}

	if (channel->onehandler) {
		return ERR_UNSUPPORTED;
	}

	event_ch_handler_s* curr = channel->handlers;

	// If the list is empty, add the handler to the start
	if (curr == NULL) {
		channel->handlers				= new_handler;
		channel->handlers->next = NULL;
		return 0;
	}

	// If the list is not empty then insert the new handler based on its priority

	// For priority 0, the handler is added to the end of the list
	if (new_handler->priority == 0) {
		while (curr->next) {
			curr = curr->next;
		}
		curr->next = new_handler;
		return 0;
	}

	// If the priority of the new handler is higher than the first handler in the
	// list, then insert it at the start
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

int event_channel_unsubscribe(event_ch_e ch, event_ch_handler_s* h) {
	assert(h);
	assert(ch < EVENT_CHANNEL_NB);

	event_channel_s* channel = channels[ch];

	// Check if the channel is registered
	if (!channel) {
		return ERR_NULL_PTR;
	}

	if (channel->onehandler) {
		return ERR_UNSUPPORTED;
	}

	event_ch_handler_s* curr = channel->handlers;

	// If the list is empty, return
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

int event_post(event_ch_e ch, void* event) {
	assert(event);
	assert(ch < EVENT_CHANNEL_NB);

	event_channel_s* channel = channels[ch];
	assert(channel);

	// Check there is space in the channel event queue
	if (channel->head >= channel->queue_size) {
		return ERR_NO_MEM;
	}

	// Add the event to the queue
	uint index = channel->head * channel->data_size;
	memcpy(&channel->queue[index], event, channel->data_size);
	channel->head++;

	return 0;
}

int event_post_rt(event_ch_e ch, void* event) {
	assert(event);
	assert(ch < EVENT_CHANNEL_NB);

	event_channel_s* channel = channels[ch];
	assert(channel);

	// Call the event handlers directly
	event_ch_handler_s* handler = channel->handlers;
	if (!handler) {
		return ERR_NULL_PTR;
	}

	while (handler) {
		handler->handler(event);
		handler = handler->next;
	}

	return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
