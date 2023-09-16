/*
 * File: Comms.c ( 10th April 2022 )
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
#include "CBOR.h"
#include "Network.h"
#include "SoftTimer.h"
#include "Display.h"

#define ENABLE_SERIAL
#include "VirtualSerial.h"

#define MAX_INCOMING_MSGS       (5)
#define MAX_MSG_DATA_SIZE       (128) // bytes

// clang-format off

// Ensure all fields are set locally, otherwise this message will be broadcasted
const sMessage MSG_Default = {
    .Header = {
        .Protocol = PROTOCOL_BROADCAST,
        .Source = {
            .ClientAddress = BROADCAST_ADDRESS,
            .ClientType = CLIENT_MUFFIN,
            .ModuleID = MODULE_NETWORK,
        },
        .Destination = {
            .ClientAddress = BROADCAST_ADDRESS,
            .ClientType = CLIENT_ALL,
            .ModuleID = MODULE_NETWORK,
        },
        .Priority = PRIORITY_NORMAL,
        .ModuleParameter = 0,
    },
    .Data = {
        .len = 0,
        .ptr = NULL,
    },
};

// clang-format on

static bool ProcessMessage(sMessage* pMessage);

struct _protocols {
    bool Registered;
    fpProtocol_SendMessageHandler fpSendHandler;
    // fpProtocol_UpdateHandler fpUpdateHandler;
} static mProtocols[NUM_COMMS_PROTOCOLS] = {0};

struct _modules {
    fpMessageHandler fpMessageHandler;
} static mModules[NUM_MODULE_IDS] = {0};

void Comms_Init(void)
{

}

void Comms_Update(void)
{

}

static bool SendMessage(u8* pBytes, uint16_t NumBytes, eCommsProtocol Protocol)
{
    if(!mProtocols[Protocol].Registered)
    {
        Serial_Print("Protocol not registered\r\n");
        return false;
    } 
    else if (mProtocols[Protocol].fpSendHandler == NULL)
    {
        Serial_Print("No SendHandler\r\n");
        return false;
    } 
    else if (!pBytes || NumBytes == 0)
    {
        Serial_Print("SendMessage: Invalid parameters\r\n");
    }

    return mProtocols[Protocol].fpSendHandler(pBytes, NumBytes);
}

bool Comms_SendMessage(sMessage* pMessage)
{
    if (pMessage == NULL)
    {
        return false;
    }

    pMessage->Header.Source.ClientAddress = Network_GetLocalAddress();
    pMessage->Header.Source.ClientType = CLIENT_MUFFIN;

// #ifdef ENABLE_SERIAL
//     char buf[192] = "";
//     sprintf(buf, "SendMessage\r\nProtocol[%d] Priority[%d] Param[%d]\r\nSource | Type[%d] Address[%d] Module[%d]\r\nDestin | Type[%d] Address[%d] Module[%d]\r\nVal[%lu]\r\n",
//         pMessage->Header.Protocol, pMessage->Header.Priority, pMessage->Header.ModuleParameter,
//         pMessage->Header.Source.ClientType, pMessage->Header.Source.ClientAddress, pMessage->Header.Source.ModuleID,
//         pMessage->Header.Destination.ClientType, pMessage->Header.Destination.ClientAddress,pMessage->Header.Destination.ModuleID,
//         pMessage->Value);

//     Serial_Print(buf);
// #endif

    UsefulBuf_MAKE_STACK_UB(cborBuffer, (sizeof(sMessage) + SIZEOF_CBORLABEL * NUM_LABELS_MESSAGE));
    UsefulBufC encodedMessage = CBOR_Encode_Message(pMessage, cborBuffer);

    if(UsefulBuf_IsNULLC(encodedMessage))
    {
        Serial_Print("Failed to encode\r\n");
        return false;
    }

    if (pMessage->Header.Protocol == PROTOCOL_BROADCAST)
    {
        Serial_Print("Broadcast\r\n");
        bool success = true;
        for(eCommsProtocol protocol = 0; protocol < NUM_COMMS_PROTOCOLS; protocol++)
        {
            success &= SendMessage((u8*) encodedMessage.ptr, encodedMessage.len, protocol);
        }
        return success;
    }
    else
    {
        return SendMessage(encodedMessage.ptr, encodedMessage.len, pMessage->Header.Protocol);
    }

    return false;
}

bool Comms_RegisterProtocol(eCommsProtocol Protocol, fpProtocol_SendMessageHandler SendHandler)
{
    if(Protocol >= NUM_COMMS_PROTOCOLS || mProtocols[Protocol].Registered) {
        return false;
    } 

    mProtocols[Protocol].Registered = true;
    
    // if(UpdateHandler)
    //     mProtocols[Protocol].fpUpdateHandler = UpdateHandler;

    if(SendHandler)
        mProtocols[Protocol].fpSendHandler = SendHandler;

    return true;
}

bool Comms_RegisterModule(eModuleID ID, fpMessageHandler MessageHandler)
{
    if (MessageHandler == NULL)
    {
        return false;
    }

    mModules[ID].fpMessageHandler = MessageHandler;
    return true;
}

/**
 * @brief Returns the total size of a message (excluding protocol size)
 * 
 * @param pMessage a pointer to a message structure.
 * @return u16 the total size of the message, including the header, and data.
 */
 u16 Comms_MessageSize(sMessage* pMessage)
{
    return sizeof(sMessageHeader) + pMessage->Data.len;
}


static bool ProcessMessage(sMessage* pMessage)
{
    if(pMessage->Header.Destination.ClientAddress != BROADCAST_ADDRESS || pMessage->Header.Destination.ClientAddress != Network_GetLocalAddress())
    {
        return true; // no need to handle this message - ignore.
    }

    if(pMessage->Header.Priority == PRIORITY_REALTIME)
    {
        // realtime handling
    }

    if(IsValidModuleID(pMessage->Header.Destination.ModuleID))
    {
        if (mModules[pMessage->Header.Destination.ModuleID].fpMessageHandler != NULL)
        return mModules[pMessage->Header.Destination.ModuleID].fpMessageHandler(pMessage);
    }

    return false;
}

