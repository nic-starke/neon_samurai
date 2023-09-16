/*
 * File: VirtualSerial.c ( 19th March 2022 )
 * Project: Muffin
 * Copyright 2022 bxzn (mail@bxzn.one)
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

#include "VirtualSerial.h"
#include "DataTypes.h"
#include "Descriptors.h"
#include "USB.h"

static bool mHostReady = false;

void _Serial_Init(void)
{
    gCDC_Interface.State.ControlLineStates.DeviceToHost = CDC_CONTROL_LINE_IN_DSR;
    CDC_Device_SendControlLineStateChange(&gCDC_Interface);
}

void _Serial_Update(void)
{
    if (USB_IsInitialized)
    {
        /* Must throw away unused bytes from the host, or it will lock up while waiting for the device */
        CDC_Device_ReceiveByte(&gCDC_Interface);
        CDC_Device_USBTask(&gCDC_Interface);
    }
}

void _Serial_Print(char* pString)
{
    CDC_Device_SendString(&gCDC_Interface, pString);
    CDC_Device_Flush(&gCDC_Interface);
}

bool _Serial_HostReady(void)
{
    return mHostReady;
}

void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo)
{
    /* You can get changes to the virtual CDC lines in this callback; a common
	   use-case is to use the Data Terminal Ready (DTR) flag to enable and
	   disable CDC communications in your application when set to avoid the
	   application blocking while waiting for a host to become ready and read
	   in the pending data from the USB endpoints.
	*/
    mHostReady = (CDCInterfaceInfo->State.ControlLineStates.HostToDevice & CDC_CONTROL_LINE_OUT_DTR) != 0;
}