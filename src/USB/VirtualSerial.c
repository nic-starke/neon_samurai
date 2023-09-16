/*
 * File: VirtualSerial.c ( 19th March 2022 )
 * Project: Muffin
 * Copyright 2022 Nicolaus Starke  
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

/**
 * @brief Initialise the serial module.
 */
void _Serial_Init(void)
{
    gCDC_Interface.State.ControlLineStates.DeviceToHost = CDC_CONTROL_LINE_IN_DSR;
    CDC_Device_SendControlLineStateChange(&gCDC_Interface);
}

/**
 * @brief Update and process serial transmissions via USB CDC.
 * This must be called once per main loop.
 */
void _Serial_Update(void)
{
    if (USB_IsInitialized)
    {
        /* Must throw away unused bytes from the host, or it will lock up while waiting for the device */
        CDC_Device_ReceiveByte(&gCDC_Interface);
        CDC_Device_USBTask(&gCDC_Interface);
    }
}

/**
 * @brief Print a string to the USB CDC interface.
 * 
 * @param pString A pointer to a null-terminated string.
 */
void _Serial_Print(char* pString)
{
    CDC_Device_SendString(&gCDC_Interface, pString);
    CDC_Device_Flush(&gCDC_Interface);
}

/**
 * @brief Print a value with hex formatting to the USB CDC interface.
 * 
 * @param value The value to printed.
 */
void _Serial_PrintValue(uint32_t value)
{
    static char buf[8] = {0};
    sprintf(buf, "0x%.4lx", value);
    _Serial_Print(buf);
}

/**
 * @brief Check if the USB Host CDC is ready.
 * 
 * @return True if ready, false otherwise.
 */
bool _Serial_HostReady(void)
{
    return mHostReady;
}

/**
 * @brief A callback function to handle changes to the CDC control state.
 * 
 * @param CDCInterfaceInfo A pointer to the CDC device interface.
 */
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