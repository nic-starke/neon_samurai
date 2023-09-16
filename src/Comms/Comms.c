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
#include "Display.h"
#include "Network.h"
#include "SoftTimer.h"

#define ENABLE_SERIAL
#include "VirtualSerial.h"

#define MAX_INCOMING_MSGS (5)   // The number of messages in the incoming message queue
#define MAX_MSG_DATA_SIZE (128) // Maximum number of bytes per message (including CBOR encoding requirements)

// clang-format off

// This is a default struct that can be used to conveniently create a new message.
// WARNING - the message address is BROADCAST by default, change locally if required.
// TODO - move this to progmem and write a small helper function "CreateNewMessage(pMsg)..."
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

// All protocols must be added to this register during initial system bootup sequence.
struct _protocols
{
    bool                          Registered;
    fpProtocol_SendMessageHandler fpSendHandler;
    // fpProtocol_UpdateHandler fpUpdateHandler;
} static mProtocols[NUM_COMMS_PROTOCOLS] = {0};

// All modules must be added to this register during initial system bootup sequence.
struct _modules
{
    fpMessageHandler fpMessageHandler;
} static mModules[NUM_MODULE_IDS] = {0};

/**
 * @brief Initialise the comms module.
 * 
 */
void Comms_Init(void)
{
    // Not yet implemented
}

/**
 * @brief The main-loop update function for the comms module
 * 
 */
void Comms_Update(void)
{
    // Not yet implemented
}

/**
 * @brief Calls the appropriate SendMessageHandler based on the "Protocol" argument
 * 
 * @param pBytes A pointer to a byte stream.
 * @param NumBytes The number of bytes in the byte stream.
 * @param Protocol The protocol to use to transmit the byte stream.
 * @return True on successful transmission of byte stream. False on failed transmission of byte stream.
 */
static bool SendMessage(u8* pBytes, uint16_t NumBytes, eCommsProtocol Protocol)
{
    if (!mProtocols[Protocol].Registered)
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

/**
 * @brief Transmits a message.
 * Note - only valid CBOR encoded data will be appended - it will not be encoded, do this yourself.
 * @param pMessage A pointer to a message that needs to be transmitted.
 * @return True if message transmission successful, False otherwise.
 */
bool Comms_SendMessage(sMessage* pMessage)
{
    if (pMessage == NULL)
    {
        return false;
    }

    // Get the current network address for this device.
    pMessage->Header.Source.ClientAddress = Network_GetLocalAddress();
    pMessage->Header.Source.ClientType    = CLIENT_MUFFIN;

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

    if (UsefulBuf_IsNULLC(encodedMessage))
    {
        Serial_Print("Failed to encode\r\n");
        return false;
    }

    if (pMessage->Header.Protocol == PROTOCOL_BROADCAST)
    {
        Serial_Print("Broadcast\r\n");
        bool success = true; // FIXME - this provides no information on which protocol failed, what should be done to recover from failure?
        for (eCommsProtocol protocol = 0; protocol < NUM_COMMS_PROTOCOLS; protocol++)
        {
            if (SendMessage((u8*)encodedMessage.ptr, encodedMessage.len, protocol) == false)
            {
                success = false;
            }
        }
        return success;
    }
    else
    {
        return SendMessage(encodedMessage.ptr, encodedMessage.len, pMessage->Header.Protocol);
    }

    return false;
}

/**
 * @brief Register a new transmission protocol.
 * 
 * @param Protocol The unique valid enum for the Protocol.
 * @param SendHandler A pointer to the send handler for this protocol.
 * @return True if registration successful, False otherwise.
 */
bool Comms_RegisterProtocol(eCommsProtocol Protocol, fpProtocol_SendMessageHandler SendHandler)
{
    if (Protocol >= NUM_COMMS_PROTOCOLS || mProtocols[Protocol].Registered || SendHandler == NULL)
    {
        return false;
    }

    mProtocols[Protocol].Registered = true;

    // if(UpdateHandler) // TODO - may use this another time, commented out for now.
    //     mProtocols[Protocol].fpUpdateHandler = UpdateHandler;

    if (SendHandler)
        mProtocols[Protocol].fpSendHandler = SendHandler;

    return true;
}

/**
 * @brief Register a module to be comms enabled.
 * 
 * @param ID The unique valid enum for the module.
 * @param MessageHandler A pointer to the message handler that will be used when messages are received for this module.
 * @return True if registration successful, False otherwise.
 */
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

/**
 * @brief Processes an incoming message and passes to the appropriate module message handler.
 * 
 * @param pMessage A pointer to the message to process.
 * @return True if module message handler processed the message, false otherwise.
 */
static bool ProcessMessage(sMessage* pMessage)
{
    if (pMessage->Header.Destination.ClientAddress != BROADCAST_ADDRESS ||
        pMessage->Header.Destination.ClientAddress != Network_GetLocalAddress())
    {
        return true; // no need to handle this message because it wasnt for this unit.
    }

    if (pMessage->Header.Priority == PRIORITY_REALTIME)
    {
        // TODO - no realtime messages are implemented yet.
    }

    if (IsValidModuleID(pMessage->Header.Destination.ModuleID))
    {
        if (mModules[pMessage->Header.Destination.ModuleID].fpMessageHandler != NULL)
            return mModules[pMessage->Header.Destination.ModuleID].fpMessageHandler(pMessage);
    }

    return false;
}
