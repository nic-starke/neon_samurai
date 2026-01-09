/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/pgmspace.h>
#include "console/console.h"
#include "event/event.h"
#include "event/general.h"
#include "event/sys.h"
#include "hal/sys.h"
#include "system/hardware.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int event_handler(void* event);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */

static struct sys_event sys_event_queue[SYS_EVENT_QUEUE_SIZE];
static struct gen_event gen_event_queue[GEN_EVENT_QUEUE_SIZE];

static struct event_ch_handler sys_event_handler = {
		.handler	= &event_handler,
		.next			= NULL,
		.priority = 0,
};

struct event_channel sys_event_ch = {
		.queue			= (u8*)sys_event_queue,
		.queue_size = sizeof(sys_event_queue) / sizeof(struct sys_event),
		.data_size	= sizeof(struct sys_event),
		.handlers		= &sys_event_handler,
		.onehandler = true,
};

struct event_channel gen_event_ch = {
		.queue			= (u8*)gen_event_queue,
		.queue_size = sizeof(gen_event_queue) / sizeof(struct gen_event),
		.data_size	= sizeof(struct gen_event),
		.handlers		= NULL,
		.onehandler = false,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int event_handler(void* event) {
	assert(event);

	struct sys_event* e = (struct sys_event*)event;
	switch (e->type) {
		case EVT_SYS_REQ_CFG_SAVE: return ERR_NOT_IMPLEMENTED;

		case EVT_SYS_REQ_CFG_RESET: {
			// Post the response event FIRST
			struct sys_event res_evt = {.type			= EVT_SYS_RES_CFG_RESET,
																	.data.ret = true};
			event_post(EVENT_CHANNEL_SYS, &res_evt);

			// Set the reset flag in EEPROM
			mf_cfg_reset();

			// Code should not reach here after reset
			return SUCCESS;
		}

		default: return ERR_BAD_PARAM;
	}

	return 0;
}
