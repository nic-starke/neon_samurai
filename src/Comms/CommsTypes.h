/*
 * File: CommsTypes.h ( 25th March 2022 )
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

// Currently this implementation only handles 1 transport protocol - usbmidi

#include "DataTypes.h"
#include "ModuleIDs.h"

#define MAX_MUFFIN_CONNS     (8)
#define MAX_EDITOR_CONNS     (1)
#define MAX_CONNECTIONS      (MAX_MUFFIN_CONNS + MAX_EDITOR_CONNS)
#define UNASSIGNED_ADDRESS   (MAX_CONNECTIONS + 1)
#define BROADCAST_ADDRESS    (UNASSIGNED_ADDRESS)

typedef enum
{
    STATE_UNINITIALISED,
    STATE_LISTEN,
    STATE_DISCOVERY,
    STATE_CONNECTED,
    STATE_CLOSING,
    STATE_CLOSED,
    STATE_ERROR,

    NUM_CONNECTION_STATES,
} eConnectionState;

typedef enum
{
    PROTOCOL_USBMIDI,
    NUM_COMMS_PROTOCOLS,

    PROTOCOL_BROADCAST,  // send a message on every protocol (Broadcast)
} eCommsProtocol;

typedef enum
{
    CLIENT_BROADCAST,
    CLIENT_MUFFIN,
    CLIENT_EDITOR,

    NUM_CLIENT_TYPES,
} eClientType;

typedef enum
{
    MSG_DEFAULT,
    MSG_REALTIME,

    NUM_MESSAGE_TYPES,
} eMessageType;

typedef struct
{
    eClientType ClientType;
    uint8_t     ClientAddress;
    eModuleID   ModuleID;
} sNetworkAddress;

typedef struct
{
    sNetworkAddress Source;
    sNetworkAddress Destination;
    eMessageType    Type;
    u8              ModuleParameter;
    eCommsProtocol Protocol;
} sMessageHeader;


typedef struct
{
    sMessageHeader Header;
    uint32_t Value;

    uint8_t*       pData;
    uint16_t       DataSize;
} sMessage;

typedef struct
{
    uint16_t NumEncodedBlocks;
} sMIDIProtocolHeader;

typedef union
{
    sMIDIProtocolHeader MIDI;
} sProtocolHeader;

typedef struct
{
    sProtocolHeader ProtocolHeader;
    sMessage*       pMessage;
} sEncodedMessage;


// Function pointers - protocols must register all of these:
typedef bool (*fpProtocol_SendMessageHandler)(sMessage* pMessage);
typedef bool (*fpProtocol_UpdateHandler)(sMessage* pMessage);

typedef struct
{
    fpProtocol_SendMessageHandler fpSendHandler;
    fpProtocol_UpdateHandler fpUpdateHandler;
} sProtocolRegister;

// Function prototypes - modules must implement these:
typedef bool (*fpMessageHandler)(sMessage* pMessage);