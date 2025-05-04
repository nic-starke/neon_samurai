/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
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

enum usb_interface {

#ifdef ENABLE_CONSOLE
	CCI_INTERFACE,
	CDI_INTERFACE,
#endif

	MIDI_AC_INTERFACE, // Audio control interface
	MIDI_AS_INTERFACE, // Audio stream interface

#ifdef HID_ENABLE
	HID_INTERFACE,
#endif

	NUM_USB_INTERFACES
};

enum usb_str_desc {
	STRING_ID_Language		 = 0,
	STRING_ID_Manufacturer = 1,
	STRING_ID_Product			 = 2,
	STRING_ID_Serial			 = 3,
};

enum usb_endpoint {
	USB_EP_RESERVED,

	USB_EP_MIDI_STREAM_IN,
	USB_EP_MIDI_STREAM_OUT,

#ifdef HID_ENABLE
	HID_EPNUM,
#endif

#ifdef ENABLE_CONSOLE
	CDC_NOTIFICATION_EPNUM,
	CDC_IN_EPNUM,
	CDC_OUT_EPNUM,
#endif

	USB_EP_NB, // must be less than 5
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
