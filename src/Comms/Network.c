/*
 * File: Network.c ( 10th April 2022 )
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

#include <avr/pgmspace.h>

#include "Network.h"
#include "CommsTypes.h"
#include "DataTypes.h"
#include "NetworkMessages.h"
#include "SoftTimer.h"

#define DISCOVERY_TIMEOUT_MS (3000)

struct _connections
{    
    // Peer connections
    u8 LocalAddress;
    eConnectionState MuffinEditorConnection;
    union
    {
        uint8_t Peer0 : 1;
        uint8_t Peer1 : 1;
        uint8_t Peer2 : 1;
        uint8_t Peer3 : 1;
        uint8_t Peer4 : 1;
        uint8_t Peer5 : 1;
        uint8_t Peer6 : 1;
        uint8_t Peer7 : 1;
        uint8_t Peers;
    } MuffinConnections;
} static mConnections = {
    .LocalAddress = UNASSIGNED_ADDRESS,
};

void Network_Discovery(void)
{
    static sSoftTimer timer = {0};

    if(timer.State == TIMER_STOPPED)
    {
        SoftTimer_Start(&timer);
        sMessage msg = MSG_BroadcastTemplate;
        msg.Header.ModuleParameter = DISCOVERY_REQUEST;
        Comms_SendMessage(&msg, PROTOCOL_BROADCAST);
    }
    else if (timer.State == TIMER_RUNNING)
    {
        if(SoftTimer_Elapsed(&timer) > DISCOVERY_TIMEOUT_MS)
        {
            SoftTimer_Stop(&timer);
        }
    }
}

bool Network_ConnectToEditor(void)
{
    switch(mConnections.MuffinEditorConnection)
    {
        case STATE_CLOSED:
        case STATE_UNINITIALISED: {
            break;
        }

        case STATE_DISCOVERY: {
            break;
        }

        case STATE_CONNECTED: {
            return true;
            break;
        }

        // Invalid states
        case STATE_ERROR:
        case STATE_LISTEN:
        default:
            return false;
            break;
    }
    return false;
}

bool MessageHandler_Network(sMessage* pMessage)
{
    switch((eNetworkMessages)pMessage->Header.ModuleParameter)
    {
        case DISCOVERY_REQUEST:
        {
            sMessage reply = {
                .Header.Destination = pMessage->Header.Source,
                .Header.Source.ModuleID = MODULE_NETWORK,
                .Value = mConnections.LocalAddress,
            };
            
            return Comms_SendMessage(&reply, pMessage->Header.Protocol);
        }
        
        case DISCOVERY_REPLY: {
            switch(pMessage->Header.Source.ClientType)
            {
                case CLIENT_BROADCAST:
                {
                    break;
                }

                case CLIENT_EDITOR:
                {
                    Network_ConnectToEditor(); // Connect to the editor if not already done so
                    break;
                }

                case CLIENT_MUFFIN:
                {
                    mConnections.MuffinConnections.Peers |= (1u << pMessage->Header.Source.ClientAddress);
                    break;
                }

                default:
                break;
            }
            break;
        }

        default:
            break;
    }

    return true;
}