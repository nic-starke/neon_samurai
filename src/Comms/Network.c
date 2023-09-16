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

// Stores the local address and all active connections.
struct _connections
{
    // Peer connections
    NetAddress LocalAddress;

    struct
    {
        eCommsProtocol   Protocol;
        NetAddress       Address;
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
    .LocalAddress       = UNASSIGNED_ADDRESS,
};

static eAddressState   mAddressState   = ADDRESS_UNINITIALISED;
static eDiscoveryState mDiscoveryState = DISCOVERY_STOPPED;

static void            ConnectToEditor(void);
static void            NetworkDiscovery(void);
static void            AddressResolution(void);
static void            SendDiscoveryRequest(void);
static void            SendConnectionRequest(sNetworkAddress* pRemoteDevice, eCommsProtocol Protocol);
static void            SendDeviceRegistrationMessage(void);
static void            PrintLocalAddress(void);
static void            SetLocalAddress(NetAddress NewAddress);
static sNetworkAddress GetMuffinEditorAddress(void);
static u16             GetNumberOfConnectedPeers(void);
static bool            MessageHandler(sMessage* pMessage);

/**
 * @brief Initialise the Network module.
 * Recalls the last used network address from EEPROM and assign it to the local address.
 * Registers the Network module with comms.
 */
void Network_Init(void)
{
    SetLocalAddress(Data_GetNetworkAddress());
    PrintLocalAddress();
    Comms_RegisterModule(MODULE_NETWORK, MessageHandler);
}

/**
 * @brief To be called in the main loop - performs network updates periodically if required.
 */
void Network_Update(void)
{
    NetworkDiscovery();
    ConnectToEditor();
}

/**
 * @brief Attempt to connect with Muffin Editor desktop application.
 * ConnectToEditor() performs connection/session management.
 */
void Network_ConnectToEditor(void)
{
    if (mConnections.MuffinEditor.State != STATE_CONNECTED)
    {
        mConnections.MuffinEditor.State = STATE_DISCOVERY;
    }
}

/**
 * @brief Attempt discovery of all network clients (Muffin and MuffinEditor)
 * 
 * @param AssignNewAddress Assign a new network address to this unit
 */
void Network_StartDiscovery(bool AssignNewAddress)
{
    mDiscoveryState = DISCOVERY_START;
    if (AssignNewAddress)
    {
        mAddressState = ADDRESS_DISCOVERY;
    }
}

/**
 * @brief Returns the currently assigned network address.
 * 
 * @return NetAddress - The currently assigned network address.
 */
NetAddress Network_GetLocalAddress(void)
{
    return mConnections.LocalAddress;
}

/**
 * @brief The MuffinEditor connection/sessions state machine handler.
 * Will attempt to discover and then connect to the MuffinEditor.
 */
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
            sNetworkAddress editor = GetMuffinEditorAddress();
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
                if (SoftTimer_Elapsed(&timer) > 5000)
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
        default: break;
    }
}

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

/**
 * @brief A network device discovery state machine.
 * Must be polled in main loop.
 * See comment above for discovery process details.
 */
static void NetworkDiscovery(void)
{
    static sSoftTimer discoveryTimer = {0};
    static u8         attempts       = MAX_DISCOVERY_ATTEMPTS;

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
        default: attempts = MAX_DISCOVERY_ATTEMPTS; break;
    }
}

/**
 * @brief Resolve a valid network address for the local muffin device.
 * Attempt network discovery before calling this function.
 * If there are more than 16 muffin devices already in the network this function will not set a local address.
 */
static void AddressResolution(void)
{
    if (mAddressState != ADDRESS_DISCOVERY)
    {
        return;
    }

    mAddressState = ADDRESS_RESOLUTION;
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

/**
 * @brief Sends a network discovery message onto the network.
 * This message is broadcast on all protocols. 
 */
static void SendDiscoveryRequest(void)
{
    sMessage msg               = MSG_Default;
    msg.Header.ModuleParameter = DISCOVERY_REQUEST;
    Comms_SendMessage(&msg);
}

/**
 * @brief Send a connection request to a device on the network.
 * 
 * @param pRemoteDevice A pointer to the devices' network address.
 * @param Protocol The desired transmission protocol to send the request on.
 */
static void SendConnectionRequest(sNetworkAddress* pRemoteDevice, eCommsProtocol Protocol)
{
    sMessage msg               = MSG_Default;
    msg.Header.Protocol        = Protocol;
    msg.Header.Destination     = *pRemoteDevice;
    msg.Header.Source.ModuleID = MODULE_NETWORK;
    msg.Header.ModuleParameter = CONNECTION_REQUEST;

    Comms_SendMessage(&msg);
}

/**
 * @brief Send a device registration message into the network.
 * This is a broadcast message that should be sent after a valid network address has been resolved.
 * This will notify other network devices of the local network address.
 */
static void SendDeviceRegistrationMessage(void)
{
    sMessage msg                    = MSG_Default;
    msg.Header.Source.ClientAddress = Network_GetLocalAddress();
    msg.Header.Source.ModuleID      = MODULE_NETWORK;
    msg.Header.ModuleParameter      = DEVICE_REGISTRATION;

    Serial_Print("Sending DEVICE_REGISTRATION\r\n");
    Comms_SendMessage(&msg);
}

/**
 * @brief Prints the local network address to serial. 
 */
static void PrintLocalAddress(void)
{
    Serial_Print("Local address is: ");
    Serial_PrintValue(mConnections.LocalAddress);
    Serial_Print("\r\n");
}

/**
 * @brief Sets the local network address.
 * If NewAddress == UNASSIGNED_ADDRESS then the system will set address state to unitialised,
 * but will not perform address discovery/resolution.
 * 
 * @param NewAddress The value of the new address.
 */
static void SetLocalAddress(NetAddress NewAddress)
{
    mConnections.LocalAddress = NewAddress;

    if (NewAddress == UNASSIGNED_ADDRESS)
    {
        mAddressState = ADDRESS_UNINITIALISED;
    }
    else
    {
        mAddressState = ADDRESS_ASSIGNED;
    }

    Data_SetNetworkAddress(mConnections.LocalAddress);
    PrintLocalAddress();
}

/**
 * @brief Get the address for the muffin editor.
 * 
 * @return sNetworkAddress The address for the muffin editor. 
 */
static sNetworkAddress GetMuffinEditorAddress(void)
{
    sNetworkAddress address = {
        .ClientAddress = mConnections.MuffinEditor.Address,
        .ClientType    = CLIENT_EDITOR,
        .ModuleID      = MODULE_NETWORK,
    };

    // FIXME - this will return an invalid address, needs to be state aware (i.e has the editor been discovered?)
    return address;
}

/**
 * @brief Gets the currently connected number of peers.
 * 
 * @return u16 The number of peers, 0 to 16;
 */
static u16 GetNumberOfConnectedPeers(void)
{
    // Kernighan bit count algorithm
    u16 numPeers    = 0;
    u16 connections = mConnections.MuffinConnections.Peers;

    while (connections)
    {
        connections &= (connections - 1);
        numPeers++;
    }
    return numPeers;
}

/**
 * @brief The comms message handler for the network module.
 */
static bool MessageHandler(sMessage* pMessage)
{
    switch ((eModuleParameter_Network)pMessage->Header.ModuleParameter)
    {
        case DISCOVERY_REQUEST:
        {
            // If a discovery request is received, send a reply with the local address.
            sMessage reply = {
                .Header.Protocol        = pMessage->Header.Protocol,
                .Header.Destination     = pMessage->Header.Source,
                .Header.Source.ModuleID = MODULE_NETWORK,
                .Value                  = mConnections.LocalAddress,
            };

            return Comms_SendMessage(&reply);
        }

        // If a discovery reply is received - register the device.
        // If it is MuffinEditor - update the address and attempt connection.
        case DISCOVERY_REPLY:
        {
            switch (pMessage->Header.Source.ClientType)
            {
                case CLIENT_EDITOR:
                {
                    //FIXME - what happens if there are two editors attempting to send discovery replies?
                    mConnections.MuffinEditor.Address  = pMessage->Header.Source.ClientAddress;
                    mConnections.MuffinEditor.Protocol = pMessage->Header.Protocol;

                    switch (mConnections.MuffinEditor.State)
                    {
                        case STATE_UNINITIALISED:
                        case STATE_DISCOVERY:
                        {
                            mConnections.MuffinEditor.State = STATE_HANDSHAKE;
                            break;
                        }

                        default: break;
                    }

                    break;
                }

                case CLIENT_MUFFIN:
                {
                    // FIXME - should this check a UUID, or possibly a connection sate for this particular peer, before updating the connection state?
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
                    // A muffin is registering itself on the network, reset the local address and attempt network discovery.
                    mConnections.MuffinConnections.Peers |= pMessage->Header.Source.ClientAddress;
                    if (pMessage->Header.Source.ClientAddress == mConnections.LocalAddress)
                    {
                        SetLocalAddress(UNASSIGNED_ADDRESS);

                        // If there are no connection slots available - do not attempt connection!
                        if (GetNumberOfConnectedPeers() < MAX_CONNECTIONS)
                        {
                            Network_StartDiscovery(true);
                        }
                    }
                    break;
                }

                case CLIENT_EDITOR:
                {
                    if (mConnections.MuffinEditor.State == STATE_UNINITIALISED)
                    {
                        Network_ConnectToEditor();
                    }
                }

                default: break;
            }
        }

        case CONNECTION_REQUEST:
        {
            // TODO - handle a connection request.
            break;
        }

        case CONNECTION_REPLY:
        {
            switch (pMessage->Header.Source.ClientType)
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
                        if (mConnections.MuffinEditor.State == STATE_HANDSHAKE)
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