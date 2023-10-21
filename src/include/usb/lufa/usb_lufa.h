/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "LUFA/Drivers/USB/USB.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define USB_VENDOR_ID								0x2580
#define USB_PRODUCT_ID							0x0007
#define USB_HID_EPSIZE							32
#define USB_KEYBOARD_EPSIZE					8
#define USB_MOUSE_EPSIZE						8
#define USB_MIDI_STREAM_EPSIZE			64
#define USB_CDC_NOTIFICATION_EPSIZE 8
#define USB_CDC_EPSIZE							16
#define USB_MIDI_POLLING_INTERVAL		0x05

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
	MIDI_AC_INTERFACE, // Audio control interface
	MIDI_AS_INTERFACE, // Audio stream interface

#ifdef HID_ENABLE
	HID_INTERFACE,
#endif

#ifdef VSER_ENABLE
	CCI_INTERFACE,
	CDI_INTERFACE,
#endif

	NUM_USB_INTERFACES
} usb_interface_e;

typedef enum {
	STRING_ID_Language		 = 0,
	STRING_ID_Manufacturer = 1,
	STRING_ID_Product			 = 2,
	STRING_ID_Serial			 = 3,
} usb_str_desc_e;

typedef enum {
	USB_EP_RESERVED,

	USB_EP_MIDI_STREAM_IN,
	USB_EP_MIDI_STREAM_OUT,

#ifdef HID_ENABLE
	HID_EPNUM,
#endif

#ifdef VSER_ENABLE
	CDC_NOTIFICATION_EPNUM,
	CDC_IN_EPNUM,
	CDC_OUT_EPNUM,
#endif

	USB_EP_NB,
} usb_endpoint_e;

#if (USB_EP_NB - 1) > ENDPOINT_TOTAL_ENDPOINTS
#error There are not enough available USB endpoints.
#endif

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
