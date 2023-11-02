/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "core/core_types.h"
#include "core/core_error.h"
#include "midi/midi.h"
#include "event/events_midi.h"
#include "usb/lufa/usb_lufa.h"

#include "LUFA/Common/Common.h"
#include "LUFA/Drivers/USB/USB.h"
#include "LUFA/Drivers/USB/Class/Common/MIDIClassCommon.h"
#include "LUFA/Drivers/USB/Class/Device/MIDIClassDevice.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define MIDI_EVENT_QUEUE_SIZE 32

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void midi_out_handler(void* event);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */

USB_ClassInfo_MIDI_Device_t lufa_usb_midi_device = {
		.Config =
				{
						.DataINEndpoint =
								{
										.Address = (USB_EP_MIDI_STREAM_IN | ENDPOINT_DIR_IN),
										.Size		 = USB_MIDI_STREAM_EPSIZE,
										.Banks	 = 1,
								},
						.DataOUTEndpoint =
								{
										.Address = (USB_EP_MIDI_STREAM_OUT | ENDPOINT_DIR_OUT),
										.Size		 = USB_MIDI_STREAM_EPSIZE,
										.Banks	 = 1,
								},
				},
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static midi_event_s midi_in_event_queue[MIDI_EVENT_QUEUE_SIZE];
static midi_event_s midi_out_event_queue[MIDI_EVENT_QUEUE_SIZE];

static event_ch_handler_s midi_out_event_handler = {
		.handler	= midi_out_handler,
		.next			= NULL,
		.priority = 0,
};

event_channel_s midi_in_event_ch = {
		.queue			= (u8*)midi_in_event_queue,
		.queue_size = MIDI_EVENT_QUEUE_SIZE,
		.data_size	= sizeof(midi_event_s),
		.handlers		= NULL,
		.onehandler = false,
};

event_channel_s midi_out_event_ch = {
		.queue			= (u8*)midi_out_event_queue,
		.queue_size = MIDI_EVENT_QUEUE_SIZE,
		.data_size	= sizeof(midi_event_s),
		.handlers		= NULL,
		.onehandler = false,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int midi_init(void) {
	int ret = event_channel_register(EVENT_CHANNEL_MIDI_IN, &midi_in_event_ch);
	RETURN_ON_ERR(ret);

	ret = event_channel_register(EVENT_CHANNEL_MIDI_OUT, &midi_out_event_ch);
	RETURN_ON_ERR(ret);

	ret = event_channel_subscribe(EVENT_CHANNEL_MIDI_OUT, &midi_out_event_handler);
	RETURN_ON_ERR(ret);

	return ret;
}

int midi_update(void) {
	MIDI_EventPacket_t rx;
	if (MIDI_Device_ReceiveEventPacket(&lufa_usb_midi_device, &rx)) {

		midi_cc_event_s cc;
		cc.channel = (rx.Data1 & 0x0F);
		cc.control = rx.Data2;
		cc.value	 = rx.Data3;

		midi_event_s e;
		e.event_id = MIDI_EVENT_CC;
		e.data.cc	 = cc;

		event_post(EVENT_CHANNEL_MIDI_IN, &e);

		switch (rx.Event) {
			case MIDI_EVENT(0, MIDI_COMMAND_CONTROL_CHANGE): {
				midi_cc_event_s cc;
				cc.channel = (rx.Data1 & 0x0F);
				cc.control = rx.Data2;
				cc.value	 = rx.Data3;

				midi_event_s e;
				e.event_id = MIDI_EVENT_CC;
				e.data.cc	 = cc;

				event_post(EVENT_CHANNEL_MIDI_IN, &e);
				break;
			}
		}
	}
	return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static void midi_out_handler(void* event) {
	assert(event);

	midi_event_s* e = (midi_event_s*)event;

	switch (e->event_id) {
		case MIDI_EVENT_CC: {
			midi_cc_event_s* cc = &e->data.cc;

			MIDI_EventPacket_t pkt;
			pkt.Event = MIDI_EVENT(0, MIDI_COMMAND_CONTROL_CHANGE);
			pkt.Data1 = (cc->channel & 0x0F | MIDI_COMMAND_CONTROL_CHANGE);
			pkt.Data2 = (cc->control & 0x7F);
			pkt.Data3 = (cc->value & 0x7F);
			MIDI_Device_SendEventPacket(&lufa_usb_midi_device, &pkt);
			break;
		}

		default: return;
	}
}
