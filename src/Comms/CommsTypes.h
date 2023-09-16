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
#include "UsefulBuf.h"

#define MAX_MUFFIN_CONNS    (16)
#define MAX_EDITOR_CONNS    (1)
#define MAX_CONNECTIONS     (MAX_MUFFIN_CONNS + MAX_EDITOR_CONNS)
#define UNASSIGNED_ADDRESS  (0)
#define BROADCAST_ADDRESS   (0)
#define SIZEOF_CBORLABEL    (sizeof(char[2])) // all cbor labels/keys are char[2] size

#define MAX_DISCOVERY_ATTEMPTS (3)
#define DISCOVERY_WAIT_TIME    (3000) // Number of milliseconds to wait for discovery replies from peers.

typedef uint16_t NetAddress;

typedef enum
{
    STATE_UNINITIALISED,
    STATE_DISCOVERY,
    STATE_HANDSHAKE,
    STATE_CONNECTED,
    STATE_CLOSING,
    STATE_CLOSED,

    STATE_LISTEN,
    STATE_ERROR,

    NUM_CONNECTION_STATES,
} eConnectionState;

// clang-format off
/**
 * @brief Discovery Process
 * At bootup the local address will be recalled from EEPROM.
 * This address will be the last valid address assigned during any previous discovery process.
 * If this unit has never been assigned an address then its local address will be == UNASSIGNED_ADDRESS
 * 
 * The local address is just a uint16 bitfield.
 * A total of 16 muffin units can be connected in a network (designed to be 1 per midi channel)
 * 
 * The discovery process works as follows: 
 * [On Bootup]                  - Retrieve the previously used network address from non-volatile storage, set local discovery state to DISCOVERY_START
 * [DISCOVERY_START]            - Send a network discovery message, set state to DISCOVERY_AWAITING_REPLIES
 * [DISCOVERY_AWAITING_REPLIES] - wait DISCOVERY_WAIT_TIME milliseconds then go to DISCOVERY_COMPLETE
 * [DISCOVERY_COMPLETE]         - 
 *      ? Did we receive any discovery replies?
 *      Yes - Execute AddressResolution() and set state to DISCOVERY_STOPPED
 *      No - Set state to DISCOVERY_START, but only upto 3 times total
 * 
 * The address resolution process works as follows:
 * Check the peers in mConnections, find the first available position (first bit set to zero in uint16 bitfield)
 * Set the local address equal to this value (i.e 1U << FreeConnectionIndex)
 * 
 * Send a DEVICE_REGISTRATION message (the Header.Source.ClientAddress field will now be updated with the local address)
 * This will force any devices that share the local address to restart the discovery process.
 * This should not happen
 */
// clang-format on
typedef enum
{
    DISCOVERY_STOPPED,
    DISCOVERY_START,
    DISCOVERY_AWAITING_REPLIES,
    DISCOVERY_COMPLETE,

    NUM_DISCOVERY_STATES,
} eDiscoveryState;

typedef enum
{
    ADDRESS_UNINITIALISED,
    ADDRESS_DISCOVERY,
    ADDRESS_RESOLUTION,
    ADDRESS_ASSIGNED,

    NUM_ADDRESS_STATES,
} eAddressState;

typedef enum
{
    PROTOCOL_USBMIDI,
    NUM_COMMS_PROTOCOLS,

    PROTOCOL_BROADCAST, // send a message on every protocol (Broadcast)
} eCommsProtocol;

typedef enum
{
    CLIENT_ALL, // the destination is for all listening devices
    CLIENT_MUFFIN,
    CLIENT_EDITOR,

    NUM_CLIENT_TYPES,
} eClientType;

typedef enum
{
    PRIORITY_NORMAL,
    PRIORITY_REALTIME,

    NUM_MESSAGE_PRIORITIES,
} eMessagePriority;

typedef struct
{
    eClientType ClientType;
    NetAddress  ClientAddress;
    eModuleID   ModuleID;
} sNetworkAddress;

#define NUM_LABELS_NETWORKADDRESS (3) // the number of CBOR labels/keys required to encode this message into a map

typedef struct
{
    eCommsProtocol  Protocol;
    eMessagePriority Priority;
    uint8_t         ModuleParameter;
    sNetworkAddress Source;
    sNetworkAddress Destination;
} sMessageHeader;

#define NUM_LABELS_MESSAGEHEADER (5 + NUM_LABELS_NETWORKADDRESS * 2)

typedef struct
{
    union
    {
        sMessageHeader Header;
        uint8_t        Bytes[sizeof(sMessageHeader)];
    };
    uint32_t   Value;
    UsefulBufC Data;
} sMessage;

#define NUM_LABELS_MESSAGE (3 + NUM_LABELS_MESSAGEHEADER)

// Function pointers - protocols must register all of these:
typedef bool (*fpProtocol_SendMessageHandler)(u8* pBytes, uint16_t NumBytes);
// typedef bool (*fpProtocol_UpdateHandler)();

typedef struct
{
    fpProtocol_SendMessageHandler fpSendHandler;
    // fpProtocol_UpdateHandler      fpUpdateHandler;
} sProtocolRegister;

// Function prototypes - modules must implement these:
typedef bool (*fpMessageHandler)(sMessage* pMessage);