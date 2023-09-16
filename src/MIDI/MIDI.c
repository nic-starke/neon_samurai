/*
 * File: MIDI.c ( 17th March 2022 )
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"

#include <avr/pgmspace.h>

#include "CommDefines.h"
#include "Comms.h"
#include "Config.h"
#include "DataTypes.h"
#include "Display.h"
#include "MIDI.h"
#include "USB.h"

#define ENABLE_SERIAL
#include "VirtualSerial.h"

#define SYSEX_NONRT        0x7E
#define SYSEX_RT           0x7F
#define SYSEX_CH           0x00
#define SYSEX_BROADCAST_CH 0x7F
#define OUTBUFFER_LEN      32 // must be divisible by 8!

typedef enum
{
    PARSER_INIT,

    PARSER_SYSEX_NONRT,
    PARSER_SYSEX_RT,

    PARSER_DONE,

    PARSER_ERROR,

    NUM_PARSER_STATES,
} eParserState;

static eParserState mParserState              = PARSER_INIT;
static u8           mOutBuffer[OUTBUFFER_LEN] = {0};

static const u8 MANF_ID[3] PROGMEM = {0x00, 0x48, 0x01};
static const u8 FMLY_ID[2] PROGMEM = {0x00, 0x00};
static const u8 PROD_ID[2] PROGMEM = {0x00, 0x01};
static const u8 VRSN_ID[4] PROGMEM = {0x00, 0x00, 0x00, VERSION};

// static const u8 SYSEX_HEADER[] = {
//     0xF0,             // start of sysex
//     0x00, 0x48, 0x01, // manufacturer id
//     0x00, 0x01        // product id
// };

// typedef struct
// {
//     u8 MSBs;
//     u8 Data[7];
// } sSysExPacket;

// typedef struct
// {
//     u8 header[sizeof(SYSEX_HEADER)];
//     u8 dataLen;
// } sMuffinSysex;

static inline void TransmitMidiCC(u8 Channel, u8 CC, u8 Value)
{
    MIDI_EventPacket_t packet = {0};
    packet.Event              = MIDI_EVENT(0, MIDI_COMMAND_CONTROL_CHANGE);
    packet.Data1              = (Channel & 0x0F) | MIDI_COMMAND_CONTROL_CHANGE;
    packet.Data2              = CC & 0x7F;
    packet.Data3              = Value & 0x7F;
    MIDI_Device_SendEventPacket(&gMIDI_Interface, &packet);
}

static inline void TransmitMidiNote(u8 Channel, u8 Note, u8 Velocity, bool NoteOn)
{
    MIDI_EventPacket_t packet = {0};
    u8                 cmd    = NoteOn ? MIDI_COMMAND_NOTE_ON : MIDI_COMMAND_NOTE_OFF;
    packet.Event              = MIDI_EVENT(0, cmd);
    packet.Data1              = (Channel & 0x0F) | cmd;
    packet.Data2              = Note & 0x7F;
    packet.Data3              = Velocity & 0x7F;
    MIDI_Device_SendEventPacket(&gMIDI_Interface, &packet);
}

/*
    To use sysex messages as an arbitrary data stream it is necessary to encode/decode data.
    Data bytes cannot have the midi status bit set (bit 8), therefore the maximum value 
    for any byte 0x7F. 

    The encoding scheme is simple - always transmit 8 byte blocks.
    A block contains 7 data bytes, and an MSB byte.
    The MSB byte contains the MSB bits for the 7 data bytes.
    
    The encoder does the following:
    for the 7 bytes {
        1. Set bit 7 in the MSB byte equal to the MSB of data byte.
        2. Mask off the MSB of data byte N and add it to the encode buffer
    }
    
    WARNING - pDestBuffer size should be atleast ENCODE_LEN(NumDataBytes) in size.
    If the last block contains partial data, it is padded upto the 8 bytes with zeros.
*/
static u16 EncodeToSysex(u8* pDataBytes, u8* pDestBuffer, u16 NumDataBytes)
{
    u16 numBlocks = 0;

    while (NumDataBytes)
    {
        u8 msb = 0x00;
        for (u8 i = 0; i < 7; i++)
        {
            if (NumDataBytes-- > 0) // encode if any data left
            {
                msb |= ((*pDataBytes & 0x80) << i);
                *pDestBuffer++ = (*pDataBytes++ & 0x7F);
            }
            else // otherwise pad with zeros
            {
                *pDestBuffer++ = 0x00;
            }
        }
        *pDestBuffer++ = (msb & 0x7F);
        numBlocks++;
    }

    return numBlocks;
}

// Decode from sysex
static void DecodeFromSysex(u8* pSysexBytes, u8* pDestBuffer, u16 NumBlocks)
{
    for (u16 block = 0; block < NumBlocks; block++)
    {
        u8* pSysexBlock = &pSysexBytes[(block * 8)];
        for (u8 i = 0; i < 7; i++)
        {
            // Extract the msb for the data byte from the MSB byte
            u8 msb         = (pSysexBlock[7] & (0x01 << i));
            // insert MSB into the data byte, then push into dest buffer
            *pDestBuffer++ = (pSysexBlock[i] | msb);
        }
    }
}

void MIDI_ProcessLayer(sEncoderState* pEncoderState, sVirtualEncoderLayer* pLayer, u8 ValueToTransmit)
{
    switch ((eMidiMode)pLayer->MidiConfig.Mode)
    {
        case MIDIMODE_CC:
        {
            TransmitMidiCC(pLayer->MidiConfig.Channel, pLayer->MidiConfig.MidiValue.CC, ValueToTransmit);
            break;
        }

        case MIDIMODE_REL_CC:
        {
            if (pEncoderState->CurrentValue < pEncoderState->PreviousValue)
            {
                TransmitMidiCC(pLayer->MidiConfig.Channel, pLayer->MidiConfig.MidiValue.CC, 0x3F);
            }
            else if (pEncoderState->CurrentValue > pEncoderState->PreviousValue)
            {
                TransmitMidiCC(pLayer->MidiConfig.Channel, pLayer->MidiConfig.MidiValue.CC, 0x41);
            }

            break;
        }

        case MIDIMODE_NOTE:
        {
            TransmitMidiNote(pLayer->MidiConfig.Channel, pLayer->MidiConfig.MidiValue.Note, ValueToTransmit, true);
            break;
        }

        case MIDIMODE_DISABLED:
        default: return;
    }
}

static bool mMirrorInput = false;

// Replies to every midi packet by sending a copy back
void MIDI_MirrorInput(bool Enable)
{
    mMirrorInput = Enable;
}

void MIDI_Init(void)
{
    // nothing :)
}

// Must be called prior to LUFAs master usb task - USB_USBTask()
void MIDI_Update(void)
{
    if (USB_IsInitialized)
    {
        MIDI_EventPacket_t rx;

        if (MIDI_Device_ReceiveEventPacket(&gMIDI_Interface, &rx))
        {
            MIDI_ProcessMessage(&rx);

            if (mMirrorInput)
            {
                MIDI_Device_SendEventPacket(&gMIDI_Interface, &rx);
            }
        }
        MIDI_Device_USBTask(&gMIDI_Interface); // this calls MIDI_Device_Flush
    }
}

// Transmit sysex data via the lufa midi event packet
// This does not add sysex headers - ensure pData is a valid midi sysex message!
// This does not encode data to ensure status bit is valid - ensure data has been encoded.
static void TransmitSysexData(u8* pData, u16 DataLength)
{
    // char str[32]         = "";
    // str[sizeof(str) - 1] = '\0';
    // sprintf(str, "[MIDI TX] tx %u bytes\r\n", DataLength);
    // Serial_Print(str);

    int                i = 0;
    MIDI_EventPacket_t msg;

    while (i != DataLength)
    {
        if (DataLength - i > 3)
        {
            msg.Event = MIDI_EVENT(0, MIDI_COMMAND_SYSEX_START_3BYTE);
            msg.Data1 = pData[i++];
            msg.Data2 = pData[i++];
            msg.Data3 = pData[i++];
        }
        else
        {
            switch (DataLength - i)
            {
                case 1:
                    msg.Event = MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_1BYTE);
                    msg.Data1 = pData[i++];
                    break;
                case 2:
                    msg.Event = MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_2BYTE);
                    msg.Data1 = pData[i++];
                    msg.Data2 = pData[i++];
                    break;
                case 3:
                    msg.Event = MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_3BYTE);
                    msg.Data1 = pData[i++];
                    msg.Data2 = pData[i++];
                    msg.Data3 = pData[i++];
                    break;
            }
        }
        MIDI_Device_SendEventPacket(&gMIDI_Interface, &msg);
        MIDI_Device_Flush(&gMIDI_Interface);
    }
}

static inline void TransmitSysexByte(u8 Byte)
{
    MIDI_EventPacket_t msg;
    msg.Event = MIDI_EVENT(0, 0x50);
    msg.Data1 = Byte;
    MIDI_Device_SendEventPacket(&gMIDI_Interface, &msg);
}

// Encode and transmit arbitrary data stream
static inline void EncodeAndTransmitSysexBlock(u8* pData, u16 DataLen)
{
    u16 totalBlocks       = ENCODE_LEN_BLOCKS(DataLen);
    u16 blocksPerTransfer = sizeof(mOutBuffer) / SIZEOF_SYSEXBLOCK;

    u16 blocksDone = 0;


    for(int block = 0; (block < blocksPerTransfer) && (blocksDone < totalBlocks); i++)
    {
        EncodeToSysex()
    }


    for(int i = 0; i < totalBlocks; i++)
    {
        if(blocksDone < blocksPerTransfer)
        {
            // encode
        }
    }

    // below code is wrong

    for (int i = 0; i < blocksPerTransfer; i++)
    {
        EncodeToSysex(pData, &mOutBuffer, sizeof(mOutBuffer));
        TransmitSysexData(mOutBuffer, sizeof(mOutBuffer));
        pData += SIZEOF_SYSEXBLOCK;
    }
}

void MIDI_SendMMCMD(u8* pData, eMuffinMidiCommand Command, u16 DataLength)
{
    // TransmitSysexData(SYSEX_HEADER, sizeof(SYSEX_HEADER));
    // TransmitSysexByte(Command);
    // TransmitSysexData(pData, DataLength);
    // TransmitSysexByte(MIDI_CMD_SYSEX_END);
}

void MIDI_SendCommsMessage(sMessage* pMessage)
{

    if (!pMessage)
    {
        return;
    }

    sEncodedMessage toSend = {0};
    toSend.pMessage        = pMessage;

    uint32_t numBytes                           = Comms_MessageSize(pMessage);
    toSend.ProtocolHeader.MIDI.NumEncodedBlocks = ENCODE_LEN_BLOCKS(numBytes);

    TransmitSysexByte(MIDI_CMD_SYSEX_START);

    for (int i = 0; i < sizeof(MANF_ID); i++)
        TransmitSysexByte(pgm_read_byte(&MANF_ID[i]));

    TransmitSysexData((u8*)&pMessage->Header, sizeof(sMessageHeader));

    if (pMessage->pData)
        TransmitSysexData(pMessage->pData, pMessage->DataSize);

    TransmitSysexByte(MIDI_CMD_SYSEX_END);
}

static inline void PrintMsg(MIDI_EventPacket_t* pMsg, int DataCount)
{
    if (DataCount > 3)
    {
        DataCount = 3;
    }
    else if (DataCount == 0)
    {
        return;
    }

    for (int i = 0; i < DataCount; i++)
    {
        char val[2] = "";
        sprintf(val, "0x%02x", pMsg->Data[i]);
        Serial_Print(val);
        Serial_Print(" ");
    }
    Serial_Print("\r\n");
}

static inline void SysExProcess_NonRT(eMidiSysExNonRealtime SubId)
{
    switch (SubId)
    {
        case GENERAL_SYS_ID_REQ:
        {
            u8* b = &mOutBuffer[0];

            *b++ = MIDI_CMD_SYSEX_START;

            *b++ = SYSEX_NONRT;
            *b++ = SYSEX_BROADCAST_CH;
            *b++ = MIDI_SYSEX_NONRT_GENERAL_SYS;
            *b++ = GENERAL_SYS_ID_REP;

            for (int i = 0; i < sizeof(MANF_ID); i++)
                *b++ = pgm_read_byte(&MANF_ID[i]);

            for (int i = 0; i < sizeof(FMLY_ID); i++)
                *b++ = pgm_read_byte(&FMLY_ID[i]);

            for (int i = 0; i < sizeof(PROD_ID); i++)
                *b++ = pgm_read_byte(&PROD_ID[i]);

            for (int i = 0; i < sizeof(VRSN_ID); i++)
                *b++ = pgm_read_byte(&VRSN_ID[i]);

            *b++ = MIDI_CMD_SYSEX_END;
            Serial_Print("[MIDI TX] ID reply\r\n");
            TransmitSysexData(mOutBuffer, b - &mOutBuffer[0]);
            break;
        }

        default: break;
    }
}

void MIDI_ProcessMessage(MIDI_EventPacket_t* pMsg)
{
    int count = 0;

    switch (pMsg->Event)
    {
        case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_1BYTE):
        {
            count = 1;
            PrintMsg(pMsg, count);
            Serial_Print("SysEx 1 byte\r\n");
            break;
        }
        case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_2BYTE):
        {
            count = 2;
            PrintMsg(pMsg, count);
            Serial_Print("SysEx 2 byte\r\n");
            break;
        }
        case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_3BYTE):
        {
            count = 3;
            PrintMsg(pMsg, count);
            Serial_Print("SysEx 3 byte\r\n");
            break;
        }
        break;

        case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_START_3BYTE):
        {
            Serial_Print("SysEx stream\r\n");
            count = 3;
            PrintMsg(pMsg, count);
            switch (mParserState)
            {
                case PARSER_INIT:
                {
                    // redundant as lufa passed sysex message already
                    // if (pMsg->Data[0] == MIDI_CMD_SYSEX_START)
                    // {

                    // }
                    if (pMsg->Data[2] == SYSEX_CH || pMsg->Data[2] == SYSEX_BROADCAST_CH)
                    {
                        if (pMsg->Data[1] == SYSEX_NONRT)
                        {
                            Serial_Print("Non Realtime\r\n");
                            mParserState = PARSER_SYSEX_NONRT;
                        }
                        else if (pMsg->Data[1] == SYSEX_RT)
                        {
                            Serial_Print("Realtime\r\n");
                            mParserState = PARSER_SYSEX_RT;
                        }
                    }
                    else
                    {
                        return;
                    }
                    break;
                }

                case PARSER_SYSEX_NONRT:
                {
                    //Check if this is a comms message, then start streaming.
                    if (pMsg->Data[0] == MSG_START)
                    {
                        // static sMessage msg = {0};
                        // Comms_ReceiveMessage(&msg);
                    }
                }

                // parser is in an invalid state!
                default: break;
            }
            break;
        }

        default:
        //case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_1BYTE):
        case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_2BYTE):
        {
            count = 2;
            PrintMsg(pMsg, count);
            Serial_Print("SysEx end stream 2 byte\r\n");
            break;
        }
        case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_3BYTE):
        {
            count = 3;
            PrintMsg(pMsg, count);
            Serial_Print("SysEx end stream 3 byte\r\n");
            switch (mParserState)
            {
                case PARSER_SYSEX_NONRT:
                {
                    switch ((eMidiSysExNonRealtime)pMsg->Data[0])
                    {
                        case MIDI_SYSEX_NONRT_GENERAL_SYS:
                        {
                            SysExProcess_NonRT((eMidiSysExNonRealtime)pMsg->Data[1]);
                            break;
                        }

                        default: break;
                    }

                    mParserState = PARSER_INIT;
                    break;
                }

                default: break;
            }
            break;
        }
    }
}