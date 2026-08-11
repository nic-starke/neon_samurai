/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"
#include "system/error.h"
#include "system/print.h"
#include "event/midi.h"
#include "midi/midi.h"
#include "usb/usb_lufa.h"

#include "LUFA/Common/Common.h"
#include "LUFA/Drivers/USB/USB.h"
#include "LUFA/Drivers/USB/Class/Common/MIDIClassCommon.h"
#include "LUFA/Drivers/USB/Class/Device/MIDIClassDevice.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define MIDI_EVENT_QUEUE_SIZE 16

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int									midi_out_handler(void* event);
static enum midi_sysex_type midi_sysex_type(u8 evt);
static int									lufa_transmit(u8* data, u8 len);

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

static struct event_ch_handler midi_out_event_handler = {
		.handler	= midi_out_handler,
		.next			= NULL,
		.priority = 0,
};

struct event_channel midi_in_event_ch = {
		.queue			= (u8*)midi_in_event_queue,
		.queue_size = MIDI_EVENT_QUEUE_SIZE,
		.data_size	= sizeof(midi_event_s),
		.handlers		= NULL,
		.onehandler = false,
};

struct event_channel midi_out_event_ch = {
		.queue			= (u8*)midi_out_event_queue,
		.queue_size = MIDI_EVENT_QUEUE_SIZE,
		.data_size	= sizeof(midi_event_s),
		.handlers		= NULL,
		.onehandler = false,
};

// Transmit buffer for USB MIDI packets (must 4 bytes, do not change!)
static u8 tx_buf[4];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int midi_init(void) {
	int ret = event_channel_register(EVENT_CHANNEL_MIDI_IN, &midi_in_event_ch);
	RETURN_ON_ERR(ret);

	ret = event_channel_register(EVENT_CHANNEL_MIDI_OUT, &midi_out_event_ch);
	RETURN_ON_ERR(ret);

	ret =
			event_channel_subscribe(EVENT_CHANNEL_MIDI_OUT, &midi_out_event_handler);
	RETURN_ON_ERR(ret);

	return ret;
}

int midi_update(void) {
	MIDI_EventPacket_t rx;

	while (MIDI_Device_ReceiveEventPacket(&lufa_usb_midi_device, &rx)) {

		switch (rx.Event) {
			case MIDI_EVENT(0, MIDI_COMMAND_CONTROL_CHANGE): {
				// println_pmem("Rx CC:");
				midi_cc_event_s cc;
				cc.channel = (rx.Data1 & 0x0F);
				cc.control = rx.Data2;
				cc.value	 = rx.Data3;

				midi_event_s e;
				e.type		= MIDI_EVENT_CC;
				e.data.cc = cc;

#ifdef ENABLE_CONSOLE
#warning "Clib printf functions use lots of memory."
				// char										 buf[64];
				// static const char* const formatstr = "CH: %u, CC: %u, VAL:
				// %u"; sprintf(buf, formatstr, cc.channel, cc.control,
				// cc.value); println(buf);
#endif

				event_post(EVENT_CHANNEL_MIDI_IN, &e);
				break;
			}

			case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_1BYTE):
			case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_2BYTE):
			case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_3BYTE):
			case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_START_3BYTE): //  >= 4 bytes
			case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_2BYTE):
			case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_3BYTE): {
				midi_event_s e = {
						.type = MIDI_EVENT_SYSEX,
						.data.sysex_in =
								{
										.type = midi_sysex_type(rx.Event),
										.data = {rx.Data1, rx.Data2, rx.Data3},
								},
				};

				event_post(EVENT_CHANNEL_MIDI_IN, &e);

				// transmit back to host
				// MIDI_Device_SendEventPacket(&lufa_usb_midi_device, &rx);
				break;
			}

			default: return 0;
		}
	}

	return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int midi_out_handler(void* event) {
	assert(event);

	midi_event_s* e = (midi_event_s*)event;

	MIDI_EventPacket_t pkt = {0};

	switch (e->type) {
		case MIDI_EVENT_CC: {
			midi_cc_event_s* cc = &e->data.cc;
			// println_pmem("Tx CC:");

			pkt.Event = MIDI_EVENT(0, MIDI_COMMAND_CONTROL_CHANGE);
			pkt.Data1 = ((cc->channel & 0x0F) | MIDI_COMMAND_CONTROL_CHANGE);
			pkt.Data2 = (cc->control & 0x7F);
			pkt.Data3 = (cc->value & 0x7F);

			MIDI_Device_SendEventPacket(&lufa_usb_midi_device, &pkt);
			break;
		}

		case MIDI_EVENT_SYSEX: {
			midi_sysex_out_event_s* sysex = &e->data.sysex_out;

			tx_buf[0] = MIDI_EVENT(0, MIDI_COMMAND_SYSEX_START_3BYTE);
			tx_buf[1] = MIDI_STATUS_SYSTEM_EXCLUSIVE;
			tx_buf[2] = MIDI_MFR_ID_1;
			tx_buf[3] = MIDI_MFR_ID_2;
			lufa_transmit(tx_buf, sizeof(tx_buf));

			tx_buf[0] = MIDI_EVENT(0, MIDI_COMMAND_SYSEX_START_3BYTE);
			tx_buf[1] = MIDI_MFR_ID_3;
			tx_buf[2] = sysex->cmd;
			tx_buf[3] = sysex->param;
			lufa_transmit(tx_buf, sizeof(tx_buf));

			// Send data_len, followed by the data bytes themselves, then the
			// F7 terminator - up to 3 payload bytes (tx_buf[1..3]) per
			// USB-MIDI packet, same as the mfid/cmd/param packets above.
			// data_len occupies the first payload byte of the first packet.
			//
			// This replaced a hand-unrolled version with several bugs: the
			// data_len==2 case never marked itself done (or decremented the
			// remaining-byte count), so it fell into the follow-up loop and
			// sent bogus extra bytes; the follow-up loop re-read *payload as
			// an index into sysex->data[] a second time instead of using the
			// byte it already pointed at (reading wildly out of bounds
			// beyond the 8-byte data[] array); and that loop's
			// lufa_transmit() call was placed after case 3's break - dead
			// code, so those packets were never actually sent at all.
			u8	 remaining = sysex->data_len;
			u8*	 payload	 = sysex->data;
			bool first		 = true;
			bool done			 = false;

			// USB-MIDI SysEx CIN codes are not sequential by 1 (END_1BYTE=0x50,
			// END_2BYTE=0x60, END_3BYTE=0x70 - each a distinct nibble-shifted
			// code, not END_1BYTE+N), so look the terminating event byte up
			// by packet fill count rather than computing it arithmetically.
			static const u8 end_event[4] = {
					MIDI_COMMAND_SYSEX_END_1BYTE, // 0 data bytes this packet (just F7)
					MIDI_COMMAND_SYSEX_END_1BYTE, // 1 data byte
					MIDI_COMMAND_SYSEX_END_2BYTE, // 2 data bytes
					MIDI_COMMAND_SYSEX_END_3BYTE, // unused (3 full bytes -> not terminal)
			};

			// Bytes still to place in this packet: data_len (only in the
			// first packet) plus whatever remains of the data payload.
			while (!done) {
				u8 pending				 = (first ? 1 : 0) + remaining;
				u8 in_this_packet = (pending < 3) ? pending : 3;
				bool is_last			 = (in_this_packet < 3);

				tx_buf[0] = MIDI_EVENT(0, is_last ? end_event[in_this_packet]
																					 : MIDI_COMMAND_SYSEX_START_3BYTE);

				u8 slot = 1;
				if (first) {
					tx_buf[slot++] = remaining; // data_len
					first						= false;
				}
				while (slot <= in_this_packet) {
					tx_buf[slot++] = *payload++;
					remaining--;
				}
				// Pad any unused trailing slot(s) with the terminator, and
				// only the terminating packet actually ends the message.
				for (; slot < 4; slot++) {
					tx_buf[slot] = MIDI_STATUS_END_OF_EXCLUSIVE;
				}

				done = is_last;
				lufa_transmit(tx_buf, sizeof(tx_buf));
			}

			break;
		}

		default: {
			return ERR_BAD_PARAM;
		}
	}

	// printbuf(&pkt.Data1, 1);
	// printbuf(&pkt.Data2, 1);
	// printbuf(&pkt.Data3, 1);

	return 0;
}

static enum midi_sysex_type midi_sysex_type(u8 evt) {
	switch (evt) {
		case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_1BYTE): return SYSEX_TYPE_1BYTE;
		case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_2BYTE): return SYSEX_TYPE_2BYTE;
		case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_3BYTE): return SYSEX_TYPE_3BYTE;
		case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_START_3BYTE):
			return SYSEX_TYPE_START_3BYTE;
		case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_2BYTE):
			return SYSEX_TYPE_END_2BYTE;
		case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_3BYTE):
			return SYSEX_TYPE_END_3BYTE;
		default: return SYSEX_TYPE_INVALID;
	}
}

static int lufa_transmit(u8* data, u8 len) {
	if (USB_DeviceState != DEVICE_STATE_Configured)
		return ENDPOINT_RWSTREAM_DeviceDisconnected;

	uint8_t ErrorCode;

	Endpoint_SelectEndpoint(lufa_usb_midi_device.Config.DataINEndpoint.Address);

	if ((ErrorCode = Endpoint_Write_Stream_LE(data, len, NULL)) !=
			ENDPOINT_RWSTREAM_NoError)
		return ErrorCode;

	if (!(Endpoint_IsReadWriteAllowed()))
		Endpoint_ClearIN();

	return MIDI_Device_Flush(&lufa_usb_midi_device);
}
