/*
 * File: USB.c ( 8th November 2021 )
 * Project: Muffin
 * Copyright 2021 Nicolaus Starke  
 * -----
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include "USB/USB.h"
#include "core/types.h"
#include "Display/Display.h"
#include "Input/Encoder.h"
#include "Input/Input.h"
#include "MIDI/MIDI.h"

// #define ENABLE_SERIAL
#include "USB/VirtualSerial.h"

// clang-format off
#ifdef MIDI_ENABLE
USB_ClassInfo_MIDI_Device_t gMIDI_Interface = {
    .Config = 
    {
        .DataINEndpoint = 
        {
            .Address = (USB_EP_MIDI_STREAM_IN | ENDPOINT_DIR_IN),
            .Size    = MIDI_STREAM_EPSIZE,
            .Banks   = 1,
        },
        .DataOUTEndpoint = 
        {
            .Address = (USB_EP_MIDI_STREAM_OUT | ENDPOINT_DIR_OUT),
            .Size    = MIDI_STREAM_EPSIZE,
            .Banks   = 1,
        }
    }
};
#endif

#ifdef VSER_ENABLE
USB_ClassInfo_CDC_Device_t gCDC_Interface = {
    .Config = {
        .ControlInterfaceNumber = CCI_INTERFACE,
        .DataINEndpoint = {
            .Address            = (CDC_IN_EPNUM | ENDPOINT_DIR_IN),
            .Size               = CDC_EPSIZE,
            .Banks              = 1
        },
        .DataOUTEndpoint = {
            .Address            = (CDC_OUT_EPNUM | ENDPOINT_DIR_OUT),
            .Size               = CDC_EPSIZE,
            .Banks              = 1
        },
        .NotificationEndpoint = {
            .Address            = (CDC_NOTIFICATION_EPNUM | ENDPOINT_DIR_IN),
            .Size               = CDC_NOTIFICATION_EPSIZE,
            .Banks              = 1
        }
    }
};
#endif
// clang-format on

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
    bool ConfigSuccess = true;
#ifdef MIDI_ENABLE
    ConfigSuccess &= MIDI_Device_ConfigureEndpoints(&gMIDI_Interface);
    // ConfigSuccess &= Endpoint_ConfigureEndpoint((USB_EP_MIDI_STREAM_OUT | ENDPOINT_DIR_IN), EP_TYPE_BULK, MIDI_STREAM_EPSIZE, 1);
    // ConfigSuccess &= Endpoint_ConfigureEndpoint((USB_EP_MIDI_STREAM_IN | ENDPOINT_DIR_OUT), EP_TYPE_BULK, MIDI_STREAM_EPSIZE, 1);
#endif

#ifdef HID_ENABLE
    ConfigSuccess &= Endpoint_ConfigureEndpoint((SHARED_IN_EPNUM | ENDPOINT_DIR_IN), EP_TYPE_INTERRUPT, SHARED_EPSIZE, 1);
#endif

#ifdef VSER_ENABLE
    ConfigSuccess &= CDC_Device_ConfigureEndpoints(&gCDC_Interface);
    // ConfigSuccess &= Endpoint_ConfigureEndpoint((CDC_NOTIFICATION_EPNUM | ENDPOINT_DIR_IN), EP_TYPE_INTERRUPT, CDC_NOTIFICATION_EPSIZE, 1);
    // ConfigSuccess &= Endpoint_ConfigureEndpoint((CDC_OUT_EPNUM | ENDPOINT_DIR_OUT), EP_TYPE_BULK, CDC_EPSIZE, 1);
    // ConfigSuccess &= Endpoint_ConfigureEndpoint((CDC_IN_EPNUM | ENDPOINT_DIR_IN), EP_TYPE_BULK, CDC_EPSIZE, 1);
#endif
    Serial_Print(ConfigSuccess ? "USB configured successfully\r\n" : "USB failed to configure\r\n");
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
#ifdef MIDI_ENABLE
    MIDI_Device_ProcessControlRequest(&gMIDI_Interface);
#endif

#ifdef HID_ENABLE
#error "Need to handle HID class requests here"
#endif

#ifdef VSER_ENABLE
    CDC_Device_ProcessControlRequest(&gCDC_Interface);
#endif
}