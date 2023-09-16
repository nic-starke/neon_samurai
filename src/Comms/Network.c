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

#include "CommsTypes.h"
#include "Data.h"
#include "DataTypes.h"
#include "Network.h"
#include "NetworkMessages.h"
#include "SoftTimer.h"

#define ENABLE_SERIAL
#include "VirtualSerial.h"

struct _connections
{
    // Peer connections
    NetAddress       LocalAddress;
    
    struct {
        eCommsProtocol Protocol;
        NetAddress Address;
        eConnectionState State;
    } MuffinEditor;

    union
    {
        u16 Peer0  : 1;
        u16 Peer1  : 1;
        u16 Peer2  : 1;
        u16 Peer3  : 1;
        u16 Peer4  : 1;
        u16 Peer5  : 1;
        u16 Peer6  : 1;
        u16 Peer7  : 1;
        u16 Peer8  : 1;
        u16 Peer9  : 1;
        u16 Peer10 : 1;
        u16 Peer11 : 1;
        u16 Peer12 : 1;
        u16 Peer13 : 1;
        u16 Peer14 : 1;
        u16 Peer15 : 1;
        u16 Peers;
    } MuffinConnections;
} static mConnections = {
    .MuffinEditor.State = STATE_UNINITIALISED,
    .LocalAddress = UNASSIGNED_ADDRESS,

};

static eAddressState   mAddressState   = ADDRESS_UNINITIALISED;
static eDiscoveryState mDiscoveryState = DISCOVERY_STOPPED;

static void ConnectToEditor(void);
static void NetworkDiscovery(void);
static void AddressResolution(void);
static void SendDiscoveryRequest(void);
static void SendConnectionRequest(sNetworkAddress* pRemoteDevice, eCommsProtocol Protocol);
static void SendDeviceRegistrationMessage(void);
static void SetLocalAddress(NetAddress NewAddress);
static bool MessageHandler(sMessage* pMessage);

void Network_Init(void)
{
    mConnections.LocalAddress = Data_GetNetworkAddress();
    mAddressState             = ADDRESS_ASSIGNED;

    Serial_Print("Network address init: ");
    Serial_PrintValue(mConnections.LocalAddress);
    Serial_Print("\r\n");

    Comms_RegisterModule(MODULE_NETWORK, MessageHandler);
}

void Network_Update(void)
{
    NetworkDiscovery();
    ConnectToEditor();
}

void Network_ConnectToEditor(void)
{
    if(mConnections.MuffinEditor.State != STATE_CONNECTED)
    {
        mConnections.MuffinEditor.State = STATE_DISCOVERY;
    }
}

void Network_StartDiscovery(void)
{
    mDiscoveryState = DISCOVERY_START;
}

NetAddress Network_GetLocalAddress(void)
{
    return mConnections.LocalAddress;
}

static void ConnectToEditor(void)
{
    static sSoftTimer timer = {0};

    switch (mConnections.MuffinEditor.State)
    {
        case STATE_DISCOVERY:
        {
            Serial_Print("Discovering MuffinEditor\r\n");
            SendDiscoveryRequest();
            mConnections.MuffinEditor.State = STATE_LISTEN;
            break;
        }

        case STATE_HANDSHAKE:
        {
            Serial_Print("Handshake to MuffinEditor\r\n");
            sNetworkAddress editor = {
                .ClientAddress = mConnections.MuffinEditor.Address,
                .ClientType = CLIENT_EDITOR,
                .ModuleID = MODULE_NETWORK,
            };
            SendConnectionRequest(&editor, mConnections.MuffinEditor.Protocol);
            mConnections.MuffinEditor.State = STATE_LISTEN;
            break;
        }

        case STATE_CONNECTED:
        {
            break;
        }

        case STATE_LISTEN:
        {
            if (timer.State == TIMER_STOPPED)
            {
                SoftTimer_Start(&timer);
            } 
            else if (timer.State == TIMER_RUNNING)
            {
                if(SoftTimer_Elapsed(&timer) > 5000)
                {
                    SoftTimer_Stop(&timer);
                    Serial_Print("Editor connection timeout\r\n");
                    mConnections.MuffinEditor.State = STATE_ERROR;
                }
            }
            break;
        }

        case STATE_CLOSING:
        {
            Serial_Print("Editor connection closed\r\n");
            mConnections.MuffinEditor.State = STATE_CLOSED;
            SoftTimer_Stop(&timer);
            break;
        }

        case STATE_CLOSED:
        case STATE_UNINITIALISED:
        case STATE_ERROR:
        default: 
            break;
    }
}

static void NetworkDiscovery(void)
{
    static sSoftTimer discoveryTimer = {0};
    static u8 attempts = MAX_DISCOVERY_ATTEMPTS;

    switch (mDiscoveryState)
    {
        case DISCOVERY_START:
        {
            Serial_Print("Starting discovery\r\n");
            SoftTimer_Start(&discoveryTimer);
            SendDiscoveryRequest();
            mDiscoveryState = DISCOVERY_AWAITING_REPLIES;
            break;
        }

        case DISCOVERY_AWAITING_REPLIES:
        {
            
            if (SoftTimer_Elapsed(&discoveryTimer) > DISCOVERY_WAIT_TIME)
            {
                Serial_Print("Discovery timer finished\r\n");
                SoftTimer_Stop(&discoveryTimer);
                mDiscoveryState = DISCOVERY_COMPLETE;
            }
            break;
        }

        case DISCOVERY_COMPLETE:
        {
            Serial_Print("Discovery complete\r\n");
            attempts--;

            if (mConnections.MuffinConnections.Peers > 0)
            {
                AddressResolution();
                mDiscoveryState = DISCOVERY_STOPPED;
            }
            else
            {
                
                if (attempts > 0)
                {
                    mDiscoveryState = DISCOVERY_START;
                }
                else
                {
                    mDiscoveryState = DISCOVERY_STOPPED;
                    AddressResolution();
                }
            }

            break;
        }

        case DISCOVERY_STOPPED:
        default:
            attempts = MAX_DISCOVERY_ATTEMPTS;
             break;
    }
}

static void AddressResolution(void)
{
    bool resolved = false;
    u16  bit      = 0;

    do
    {
        if ((mConnections.MuffinConnections.Peers & (1U << bit)) == 0)
        {
            resolved = true;
            SetLocalAddress(mConnections.LocalAddress = (1U << bit));
            SendDeviceRegistrationMessage();
        }
        else if (bit++ >= 16)
        {
            resolved = true;
        }
    } while (!resolved);
}

static void SendDiscoveryRequest(void)
{
    sMessage msg               = MSG_Default;
    msg.Header.ModuleParameter = DISCOVERY_REQUEST;
    Comms_SendMessage(&msg);
}

static void SendConnectionRequest(sNetworkAddress* pRemoteDevice, eCommsProtocol Protocol)
{
    sMessage msg = MSG_Default;
    msg.Header.Protocol = Protocol;
    msg.Header.Destination = *pRemoteDevice;
    msg.Header.Source.ModuleID = MODULE_NETWORK;
    msg.Header.ModuleParameter = CONNECTION_REQUEST;

    Comms_SendMessage(&msg);
}

static void SendDeviceRegistrationMessage(void)
{
    sMessage msg                    = MSG_Default;
    msg.Header.Source.ClientAddress = Network_GetLocalAddress();
    msg.Header.Source.ModuleID      = MODULE_NETWORK;
    msg.Header.ModuleParameter      = DEVICE_REGISTRATION;

    Serial_Print("Sending DEVICE_REGISTRATION\r\n");
    Comms_SendMessage(&msg);

}

static void SetLocalAddress(NetAddress NewAddress)
{
    mConnections.LocalAddress = NewAddress;
    Data_SetNetworkAddress(mConnections.LocalAddress);
    Serial_Print("Setting local address: ");
    Serial_PrintValue(mConnections.LocalAddress);
    Serial_Print("\r\n");
}

static bool MessageHandler(sMessage* pMessage)
{
    switch ((eModuleParameter_Network)pMessage->Header.ModuleParameter)
    {
        case DISCOVERY_REQUEST:
        {
            sMessage reply = {
                .Header.Protocol        = pMessage->Header.Protocol,
                .Header.Destination     = pMessage->Header.Source,
                .Header.Source.ModuleID = MODULE_NETWORK,
                .Value                  = mConnections.LocalAddress,
            };

            return Comms_SendMessage(&reply);
        }

        case DISCOVERY_REPLY:
        {
            switch (pMessage->Header.Source.ClientType)
            {
                case CLIENT_EDITOR:
                {
                    mConnections.MuffinEditor.Address = pMessage->Header.Source.ClientAddress;
                    mConnections.MuffinEditor.Protocol = pMessage->Header.Protocol;
    
                    switch (mConnections.MuffinEditor.State)
                    {
                        case STATE_UNINITIALISED:
                        case STATE_DISCOVERY:
                        {
                            mConnections.MuffinEditor.State = STATE_HANDSHAKE;
                            break;
                        }

                        default:
                        break;
                    }

                    break;
                }

                case CLIENT_MUFFIN:
                {
                    mConnections.MuffinConnections.Peers |= pMessage->Header.Source.ClientAddress;
                    break;
                }

                default: break;
            }
            break;
        }

        case DEVICE_REGISTRATION:
        {
            switch (pMessage->Header.Source.ClientType)
            {
                case CLIENT_MUFFIN:
                {
                    mConnections.MuffinConnections.Peers |= pMessage->Header.Source.ClientAddress;
                    if (pMessage->Header.Source.ClientAddress == mConnections.LocalAddress)
                    {
                        mConnections.LocalAddress = UNASSIGNED_ADDRESS;
                        mDiscoveryState           = DISCOVERY_START;
                    }
                    break;
                }

                case CLIENT_EDITOR:
                {
                    if(mConnections.MuffinEditor.State == STATE_UNINITIALISED)
                    {
                        Network_ConnectToEditor();
                    }
                }

                default: break;
            }
        }
        
        
        case CONNECTION_REQUEST:
        {
            break;
        }

        case CONNECTION_REPLY:
        {
            switch(pMessage->Header.Source.ClientType)
            {
                case CLIENT_MUFFIN:
                {
                    // muffin to muffin connections are not yet implemented
                    break;
                }

                case CLIENT_EDITOR:
                {

                    if (pMessage->Value == 1) // connection accepted
                    {
                        if(mConnections.MuffinEditor.State == STATE_HANDSHAKE)
                        {
                            mConnections.MuffinEditor.State = STATE_CONNECTED;
                            Serial_Print("Connected to editor\r\n");
                        }
                    }
                    else // connection declined
                    {
                        mConnections.MuffinEditor.State = STATE_CLOSING;
                    }
  
                    break;
                }

                default: break;
            }
            break;
        }

        default: break;
    }

    return true;
}