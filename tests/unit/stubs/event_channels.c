/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"
#include "event/event.h"
#include "event/sys.h"
#include "event/io.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
	event.c refers to the system and IO channels by name, but their definitions
	live in modules that talk to hardware. These stand in for them so the event
	core can be tested on the host, and they keep the shape of the originals -
	in particular the system channel taking a single handler - because the
	behaviour under test depends on it.
*/

#define STUB_QUEUE_LEN (4)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static struct sys_event sys_queue[STUB_QUEUE_LEN];
static struct io_event	io_queue[STUB_QUEUE_LEN];

static int stub_handler(void* event) {
	(void)event;
	return 0;
}

static struct event_ch_handler sys_handler = {
		.priority = 0,
		.handler	= &stub_handler,
		.next			= NULL,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */

struct event_channel sys_event_ch = {
		.queue			= (u8*)sys_queue,
		.queue_size = STUB_QUEUE_LEN,
		.data_size	= sizeof(struct sys_event),
		.handlers		= &sys_handler,
		.onehandler = true,
};

struct event_channel io_event_ch = {
		.queue			= (u8*)io_queue,
		.queue_size = STUB_QUEUE_LEN,
		.data_size	= sizeof(struct io_event),
		.handlers		= NULL,
		.onehandler = false,
};
