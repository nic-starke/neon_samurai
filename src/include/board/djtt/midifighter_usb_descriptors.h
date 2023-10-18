/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "LUFA/Drivers/USB/USB.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define HID_EPSIZE              32
// #define KEYBOARD_EPSIZE 8
// #define MOUSE_EPSIZE 8
#define MIDI_STREAM_EPSIZE      64
#define CDC_NOTIFICATION_EPSIZE 8
#define CDC_EPSIZE              16

#define MIDI_POLLING_INTERVAL 0x05

#define MIDI_ENABLE 0

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef struct {
  USB_Descriptor_Configuration_Header_t Config;

#ifdef MIDI_ENABLE
  USB_Descriptor_Interface_Association_t Audio_Interface_Association;
  // MIDI Audio Control Interface
  USB_Descriptor_Interface_t             Audio_ControlInterface;
  USB_Audio_Descriptor_Interface_AC_t    Audio_ControlInterface_SPC;

  // MIDI Audio Streaming Interface
  USB_Descriptor_Interface_t                Audio_StreamInterface;
  USB_MIDI_Descriptor_AudioInterface_AS_t   Audio_StreamInterface_SPC;
  USB_MIDI_Descriptor_InputJack_t           MIDI_In_Jack_Emb;
  USB_MIDI_Descriptor_InputJack_t           MIDI_In_Jack_Ext;
  USB_MIDI_Descriptor_OutputJack_t          MIDI_Out_Jack_Emb;
  USB_MIDI_Descriptor_OutputJack_t          MIDI_Out_Jack_Ext;
  USB_Audio_Descriptor_StreamEndpoint_Std_t MIDI_In_Jack_Endpoint;
  USB_MIDI_Descriptor_Jack_Endpoint_t       MIDI_In_Jack_Endpoint_SPC;
  USB_Audio_Descriptor_StreamEndpoint_Std_t MIDI_Out_Jack_Endpoint;
  USB_MIDI_Descriptor_Jack_Endpoint_t       MIDI_Out_Jack_Endpoint_SPC;
#endif

#ifdef HID_ENABLE
  // Keyboard/Mouse
  USB_Descriptor_Interface_t HID_Interface;
  USB_HID_Descriptor_HID_t   HID_Descriptor;
  USB_Descriptor_Endpoint_t  HID_In_Endpoint;
#endif

#ifdef VSER_ENABLE
  // Virtual Serial
  // CDC Control Interface
  USB_Descriptor_Interface_Association_t CDC_Interface_Association;
  USB_Descriptor_Interface_t             CDC_CCI_Interface;
  USB_CDC_Descriptor_FunctionalHeader_t  CDC_Functional_Header;
  USB_CDC_Descriptor_FunctionalACM_t     CDC_Functional_ACM;
  USB_CDC_Descriptor_FunctionalUnion_t   CDC_Functional_Union;
  USB_Descriptor_Endpoint_t              CDC_NotificationEndpoint;
  // CDC Data Interface
  USB_Descriptor_Interface_t             CDC_DCI_Interface;
  USB_Descriptor_Endpoint_t              CDC_DataOutEndpoint;
  USB_Descriptor_Endpoint_t              CDC_DataInEndpoint;
#endif
} USB_Descriptor_Configuration_t;

typedef enum {
#ifdef MIDI_ENABLE
  MIDI_AC_INTERFACE, // Audio control interface
  MIDI_AS_INTERFACE, // Audio stream interface
#endif

#ifdef HID_ENABLE
  HID_INTERFACE,
#endif

#ifdef VSER_ENABLE
  CCI_INTERFACE,
  CDI_INTERFACE,
#endif

  NUM_USB_INTERFACES
} eUSBInterface;

typedef enum {
  STRING_ID_Language =
      0, /**< Supported Languages string descriptor ID (must be zero) */
  STRING_ID_Manufacturer = 1, /**< Manufacturer string ID */
  STRING_ID_Product      = 2, /**< Product string ID */
  STRING_ID_Serial       = 3, /**< Serial string ID */
} eStringDescriptors;

typedef enum {
  ENDPOINT_RESERVED,
#ifdef MIDI_ENABLE
  MIDI_STREAM_IN_EPNUM,
  MIDI_STREAM_OUT_EPNUM,
#endif

#ifdef HID_ENABLE
  HID_EPNUM,
#endif

#ifdef VSER_ENABLE
  CDC_NOTIFICATION_EPNUM,
  CDC_IN_EPNUM,
  CDC_OUT_EPNUM,
#endif

  NUM_USB_ENDPOINTS, // make sure to minus 1 if using this
} eUSBEndpoint;

#if (NUM_USB_ENDPOINTS - 1) > ENDPOINT_TOTAL_ENDPOINTS
#error There are not enough available USB endpoints.
#endif

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
