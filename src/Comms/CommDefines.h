/*
 * File: CommDefines.h ( 25th March 2022 )
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
#define UNASSIGNED_ID        (MAX_CONNECTIONS + 1)
#define BROADCAST_ID         (UNASSIGNED_ID)
#define DISCOVERY_TIMEOUT_MS (3000)

#define MSG_START (0x77)
#define MSG_END   (0x88)

typedef enum
{
    STATE_INIT,
    STATE_DISCONNECTED,
    STATE_DISCOVERY,
    STATE_READY,

    NUM_NETWORK_STATES,
} eNetworkState;

typedef enum
{
    TRANSPORT_USBMIDI,

    NUM_TRANSPORT_PROTOCOLS,
} eTransportProtocol;

typedef enum
{
    CLIENT_MUFFIN,
    CLIENT_EDITOR,

    NUM_CLIENT_TYPES,
} eClientType;

typedef enum
{
    MSG_NETWORK,
    MSG_DEFAULT,

    NUM_MESSAGE_TYPES,
} eMessageType;

typedef enum
{
    DISCOVERY_REQUEST,
    DISCOVERY_REPLY,

    NUM_NETWORK_MESSAGE_TYPES,
} eNetworkMessage;

typedef union
{
    eMuffinModuleID MuffinModule;
    eEditorModuleID EditorModule;
} uModuleID;

typedef struct
{
    eClientType ClientType;
    uint8_t     ClientID;
    uModuleID   ModuleID;
} sNetworkAddress;

typedef struct
{
    sNetworkAddress Source;
    sNetworkAddress Destination;
    eMessageType    Type;
    union
    {
        eNetworkMessage Network;
    } SubType;
} sMessageHeader;

typedef struct
{
    sMessageHeader Header;
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

// typedef struct {
//     sMessage Message;
//     union {

//         uint8_t* pData;
//     }
// } sIncomingMessage;

typedef struct
{
    uint8_t       LocalID;
    eNetworkState State;
    bool          MuffinEditorConnection;
    struct
    {
        uint8_t Peer0 : 1;
        uint8_t Peer1 : 1;
        uint8_t Peer2 : 1;
        uint8_t Peer3 : 1;
        uint8_t Peer4 : 1;
        uint8_t Peer5 : 1;
        uint8_t Peer6 : 1;
        uint8_t Peer7 : 1;
    } MuffinConnections;
} sCommsState;
