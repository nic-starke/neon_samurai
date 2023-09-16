/*
 * File: Comms.h ( 25th March 2022 )
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

#include "CommsTypes.h"
#include "DataTypes.h"

void Comms_Init(void);
void Comms_Update(void);
bool Comms_SendMessage(sMessage* pMessage);
void Comms_RegisterLocalAddress(NetAddress Address);
bool Comms_RegisterProtocol(eCommsProtocol Protocol, fpProtocol_SendMessageHandler SendHandler);
bool Comms_RegisterModule(eModuleID ID, fpMessageHandler MessageHandler);
u16  Comms_MessageSize(sMessage* pMessage);

extern const sMessage MSG_Default;