/*
 * File: USB.h ( 8th November 2021 )
 * Project: Muffin
 * Copyright 2021 - 2021 bxzn (mail@bxzn.one)
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

#pragma once

#include "DataTypes.h"
#include "Encoder.h"

#define DEFAULT_USB_VENDOR_ID  (0x2580) // DJTT Vendor ID
#define DEFAULT_USB_PRODUCT_ID (0x0007)

void USBMidi_Init(void);
void USBMidi_Update(void);
void USBMidi_ProcessLayer(sEncoderState* pEncoderState, sVirtualEncoderLayer* pLayer, u8 ValueToTransmit);

void EVENT_USB_Device_Connect(void);
void EVENT_USB_Device_Disconnect(void);
void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_ControlRequest(void);
