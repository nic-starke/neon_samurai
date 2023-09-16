/*
 * File: Comms.c ( 25th March 2022 )
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

#include "Comms.h"
#include "DataTypes.h"
#include "MIDI.h"
#include "SoftTimer.h"

#define ENABLE_SERIAL
#include "VirtualSerial.h"

#define MAX_INCOMING_MSGS (50)
#define MAX_DATA_SIZE     (100)

typedef struct
{
    eTransportProtocol Protocol;
    sMessage*          pMessage;
} sInternalMessage;

static sCommsState mComms      = {0};
static uint8_t     mAssignedID = UNASSIGNED_ID;
static sSoftTimer  timer       = {0};

static sInternalMessage mOutgoingMessage;

static uint8_t  mNumMessages = 0;
static sMessage mIncomingMessages[MAX_INCOMING_MSGS];
static uint8_t  mIncomingDataBuffers[MAX_INCOMING_MSGS][MAX_DATA_SIZE];

bool mTransmitting = false;

static sMessage MSG_NetworkDiscovery = {
    .Header.Source.ClientID = BROADCAST_ID,
    .Header.Source.ModuleID = INVALID_MODULE_ID,

    .Header.Destination.ClientID = BROADCAST_ID,

    .Header.Type            = MSG_NETWORK,
    .Header.SubType.Network = DISCOVERY_REQUEST,
};

static void AttemptNetworkDiscovery(void);
static void TransmitMessages(void);
static void ProcessMessages(void);

void Comms_Init(void)
{
    if (mComms.State == STATE_INIT)
    {
        // do first-time init stuff here
        mComms.State = STATE_DISCONNECTED;
    }

    if (mComms.State == STATE_DISCONNECTED)
    {
        AttemptNetworkDiscovery();
    }
}

void Comms_Update(void)
{
    switch (mComms.State)
    {
        case STATE_INIT:
        {
            break;
        }

        case STATE_DISCONNECTED:
        {
            break;
        }

        case STATE_DISCOVERY:
        {
            if (SoftTimer_Elapsed(&timer) >= DISCOVERY_TIMEOUT_MS)
            {
                SoftTimer_Stop(&timer);
                mComms.State = STATE_DISCONNECTED;
            }

            break;
        }

        case STATE_READY:
        {
            break;
        }

        default: break;
    }
}

bool Comms_SendMessage(sMessage* pMessage)
{
    if (!pMessage)
    {
        return false;
    }

    switch (pMessage->Header.Type)
    {
        case MSG_NETWORK:
        {
            break;
        }

        case MSG_DEFAULT:
        {
            if (mComms.State != STATE_READY)
            {
                return false;
            }

            // Ensure source id and type is valid.
            pMessage->Header.Source.ClientID   = mAssignedID;
            pMessage->Header.Source.ClientType = CLIENT_MUFFIN;
            break;
        }

        default: return false;
    }

    mOutgoingMessage.pMessage = pMessage;
    mOutgoingMessage.Protocol = TRANSPORT_USBMIDI;

    TransmitMessages();
    return true;
}

bool Comms_ReceiveMessage(sMessage* pMessage)
{
    if (!pMessage || mNumMessages >= MAX_INCOMING_MSGS)
    {
        return false;
    }

    memcpy(&mIncomingMessages[mNumMessages], pMessage, sizeof(sMessage));

    if (pMessage->pData && pMessage->DataSize <= MAX_DATA_SIZE)
    {
        memcpy(&mIncomingDataBuffers[mNumMessages++][0], pMessage->pData, pMessage->DataSize);
    }

    return true;
}

static void AttemptNetworkDiscovery(void)
{
    mOutgoingMessage.pMessage = &MSG_NetworkDiscovery;

    if (Comms_SendMessage(&MSG_NetworkDiscovery))
    {
        mComms.State = STATE_DISCOVERY;
        SoftTimer_Start(&timer);
    }
}

static void TransmitMessages(void)
{
    // block here?
    while (mTransmitting) {}

    mTransmitting = true;

    // At this stage, if there were multiple protocols, send on the correct protocol
    // to the destination address.
    switch (mOutgoingMessage.Protocol)
    {
        case TRANSPORT_USBMIDI:
        {
            MIDI_SendCommsMessage(mOutgoingMessage.pMessage);
        }

        default:
        {
            Serial_Print("Invalid message protocol\r\n");
            break;
        }
    }

    mTransmitting = false;
}

static void ProcessMessages(void)
{
}
