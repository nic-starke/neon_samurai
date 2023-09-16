/*
 * File: VirtualSerial.h ( 19th March 2022 )
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

#pragma once

#include "Descriptors.h"
#include "Drivers/USB/USB.h"

#ifdef VSER_ENABLE
void _Serial_Init(void);
void _Serial_Update(void);
void _Serial_Print(char* s);
void _Serial_PrintValue(uint32_t value);
bool _Serial_HostReady(void);

#define Serial_Init()      _Serial_Init()
#define Serial_Update()    _Serial_Update()
#define Serial_HostReady() _Serial_HostReady()
#ifdef ENABLE_SERIAL
#define Serial_Print(s)      _Serial_Print(s)
#define Serial_PrintValue(v) _Serial_PrintValue(v)
#else
#define Serial_Print(s)
#define Serial_PrintValue(v)
#endif
#else
#define Serial_Init()
#define Serial_Update()
#define Serial_HostReady()
#define Serial_Print(s)
#define Serial_PrintValue(v)
#endif

void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t* const CDCInterfaceInfo);