/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/pgmspace.h>
#include "event/event.h"
#include "event/sys.h"
#include "common/console/console.h"
#include "platform/midifighter/midifighter.h"
#include "hal/avr/xmega/128a4u/sys.h" // Include header for sys_reset

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define SYS_EVENT_QUEUE_SIZE 8

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int event_handler(void* event);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */

static sys_event_s sys_event_queue[8];

static event_ch_handler_s sys_event_handler = {
		.handler	= &event_handler,
		.next			= NULL,
		.priority = 0,
};

event_channel_s sys_event_ch = {
		.queue			= (u8*)sys_event_queue,
		.queue_size = SYS_EVENT_QUEUE_SIZE,
		.data_size	= sizeof(sys_event_s),
		.handlers		= &sys_event_handler,
		.onehandler = true,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int event_handler(void* event) {
	assert(event);

	sys_event_s* e = (sys_event_s*)event;
	switch (e->type) {
		case EVT_SYS_REQ_CFG_SAVE: return ERR_NOT_IMPLEMENTED;

		case EVT_SYS_REQ_CFG_RESET: {
			// Post the response event FIRST
			sys_event_s res_evt = { .type = EVT_SYS_RES_CFG_RESET, .data.ret = true };
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
