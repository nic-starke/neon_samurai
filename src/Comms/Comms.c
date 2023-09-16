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
#include "Network.h"
#include "SoftTimer.h"
#include "Display.h"

#define ENABLE_SERIAL
#include "VirtualSerial.h"

#define MAX_INCOMING_MSGS       (5)
#define MAX_MSG_DATA_SIZE       (128) // bytes

const sMessage MSG_BroadcastTemplate = {
    .Header = {
        .Source = {
            .ClientAddress = BROADCAST_ADDRESS,
            .ClientType = CLIENT_MUFFIN,
            .ModuleID = MODULE_NETWORK,
        },
        .Destination = {
            .ClientAddress = BROADCAST_ADDRESS,
            .ClientType = CLIENT_BROADCAST,
            .ModuleID = MODULE_NETWORK,
        },
        .Type = MSG_DEFAULT,
        .ModuleParameter = 0,
        .Protocol = PROTOCOL_BROADCAST,
    },
    .DataSize = 0,
    .pData = NULL,
};

static u8 mLocalAddress = UNASSIGNED_ADDRESS;

static bool ProcessMessage(sMessage* pMessage);

struct _protocols {
    bool Registered;
    fpProtocol_SendMessageHandler fpSendHandler;
    fpProtocol_UpdateHandler fpUpdateHandler;
} static mProtocols[NUM_COMMS_PROTOCOLS] = {0};

struct _modules {
    fpMessageHandler fpMessageHandler;
} static mModules[NUM_MODULE_IDS] = {0};

void Comms_Init(void)
{
    Comms_RegisterModule(MODULE_NETWORK, MessageHandler_Network);


    // switch(mComms.State)
    // {
    //     case STATE_UNITIALISED:{

    //      break;   
    //     }

    //     case STATE_DISCONNECTED:{

    //      break;   
    //     }

    //     case STATE_DISCOVERY:{

    //      break;   
    //     }

    //     case STATE_READY:{

    //      break;   
    //     }

    //     default:
    //     break;

    // }
}

void Comms_Update(void)
{
    if (mLocalAddress == UNASSIGNED_ADDRESS)
    {
        Network_Discovery();
    }
    
    for(eCommsProtocol protocol = 0; protocol < NUM_COMMS_PROTOCOLS; protocol++)
    {

    }
}

static bool SendMessage(sMessage* pMessage, eCommsProtocol Protocol)
{
    // if(!mProtocols[Protocol].Registered || mProtocols[Protocol].fpSendHandler == NULL || pMessage == NULL)
    // {
    //     Serial_Print("SendMessage error\r\n");
    //     return false;
    // }

    if(!mProtocols[Protocol].Registered)
    {
        Serial_Print("Protocol not registered\r\n");
        return false;
    } else if (mProtocols[Protocol].fpSendHandler == NULL) {
        Serial_Print("NoSendHandler\r\n");
        return false;
    } else if (pMessage == NULL) {
        Serial_Print("pMessage null\r\n");
    }

    char buf[128] = "";
    sprintf(buf, "SendMSG SourceMod[%d] DestClient[%d] DestMod[%d] Param[%d] Val[%ld]\r\n", 
        pMessage->Header.Source.ModuleID,
        pMessage->Header.Destination.ClientAddress,
        pMessage->Header.Destination.ModuleID,
        pMessage->Header.ModuleParameter,
        pMessage->Value);

    Serial_Print(buf);

    pMessage->Header.Source.ClientAddress = mLocalAddress;
    pMessage->Header.Source.ClientType = CLIENT_MUFFIN;

    return mProtocols[Protocol].fpSendHandler(pMessage);
}

void Comms_RegisterLocalAddress(u8 Address)
{
    mLocalAddress = Address;
}

bool Comms_SendMessage(sMessage* pMessage, eCommsProtocol Protocol)
{
    if (pMessage == NULL)
    {
        return false;
    }

    if (Protocol == PROTOCOL_BROADCAST)
    {
        Serial_Print("Broadcast\r\n");
        bool success = true;
        for(eCommsProtocol protocol = 0; protocol < NUM_COMMS_PROTOCOLS; protocol++)
        {
            success &= SendMessage(pMessage, protocol);
        }
        return success;
    }
    else
    {
        return SendMessage(pMessage, Protocol);
    }

    return false;
}

bool Comms_RegisterProtocol(eCommsProtocol Protocol, fpProtocol_UpdateHandler UpdateHandler, fpProtocol_SendMessageHandler SendHandler)
{
    if(Protocol >= NUM_COMMS_PROTOCOLS || mProtocols[Protocol].Registered) {
        return false;
    } 

    mProtocols[Protocol].Registered = true;
    
    if(UpdateHandler)
        mProtocols[Protocol].fpUpdateHandler = UpdateHandler;

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

static bool ProcessMessage(sMessage* pMessage)
{
    if(pMessage->Header.Destination.ClientAddress != mLocalAddress)
    {
        return true; // no need to handle this message - ignore.
    }

    if(pMessage->Header.Type == MSG_REALTIME)
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

