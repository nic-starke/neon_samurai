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

#include "Comms.h"
#include "Config.h"
#include "DataTypes.h"
#include "Display.h"
#include "MIDI.h"
#include "USB.h"

#define ENABLE_SERIAL
#include "VirtualSerial.h"

#define SYSEX_NONRT        (0x7E)
#define SYSEX_RT           (0x7F)
#define SYSEX_CH           (0x00) // default sysex channel to start with
#define SYSEX_BROADCAST_CH (0x7F)

#define SIZEOF_SYSEXBLOCK (8)
#define OUTBUFFER_LEN     (SIZEOF_SYSEXBLOCK) // a single 8-byte buffer is used and encoded data is effectively "streamed" out

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
static bool         mMirrorInput              = false;
static u8           mSysexChannel             = SYSEX_CH;

static const u8 MANF_ID[3] PROGMEM = {0x00, 0x48, 0x01};
static const u8 FMLY_ID[2] PROGMEM = {0x00, 0x00};
static const u8 PROD_ID[2] PROGMEM = {0x00, 0x01};
static const u8 VRSN_ID[4] PROGMEM = {0x00, 0x00, 0x00, VERSION};

static bool Msg_SendHandler(u8* pBytes, u16 NumBytes);

static void EncodeAndTransmit(u8* pData, u16 NumDataBytes);
static void DecodeFromSysex(u8* pSysexBytes, u8* pDestBuffer, u16 NumBlocks);
static u32  EncodedLength_Bytes(u16 NumDataBytes);
static u32  EncodedLength_Blocks(u16 NumDataBytes);
static u16  DecodedLength_Bytes(u32 NumEncodedBytes);

static inline void TransmitMidiCC(u8 Channel, u8 CC, u8 Value);
static inline void TransmitMidiNote(u8 Channel, u8 Note, u8 Velocity, bool NoteOn);
static inline void TransmitSysexByte(u8 Byte);

static void SysExProcess_NonRT(eMidiSysExNonRealtime SubId);

void MIDI_Init(void)
{
    Comms_RegisterProtocol(PROTOCOL_USBMIDI, Msg_SendHandler);
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

// Replies to every midi packet by sending a copy back
void MIDI_MirrorInput(bool Enable)
{
    mMirrorInput = Enable;
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

// Transmit sysex data via the lufa midi event packet
// This does not add sysex headers - ensure pData is a valid midi sysex message!
// This does not encode data to ensure status bit is valid - ensure data has been encoded.
static void TransmitSysexData(u8* pData, u16 DataLength)
{
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

void MIDI_ProcessMessage(MIDI_EventPacket_t* pMsg)
{
    int count = 0;

    switch (pMsg->Event)
    {
        case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_1BYTE):
        {
            count = 1;
            break;
        }
        case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_2BYTE):
        {
            count = 2;
            break;
        }
        case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_3BYTE):
        {
            count = 3;
            break;
        }
        break;

        case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_START_3BYTE):
        {
            Serial_Print("SysEx stream\r\n");
            count = 3;
            switch (mParserState)
            {
                case PARSER_INIT:
                {
                    // redundant as lufa passed sysex message already
                    // if (pMsg->Data[0] == MIDI_CMD_SYSEX_START)
                    // {

                    // }
                    if (pMsg->Data[2] == mSysexChannel || pMsg->Data[2] == SYSEX_BROADCAST_CH)
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
                    // if (pMsg->Data[0] == MSG_START)
                    // {
                    //     // static sMessage msg = {0};
                    //     // Comms_ReceiveMessage(&msg);
                    // }
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
            Serial_Print("SysEx end stream 2 byte\r\n");
            break;
        }
        case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_3BYTE):
        {
            count = 3;
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

// pDest must be atleast 8 bytes!
static u16 EncodeSysexBlock(u8* pData, u16 NumDataBytes, u8* pDest, u8 DestBufferLen)
{
    if (DestBufferLen < SIZEOF_SYSEXBLOCK || NumDataBytes == 0)
    {
        return 0;
    }

    u8 msb        = 0;
    u8 numEncoded = 0;

    for (u8 i = 0; i < 7; i++) // encode 7 bytes
    {
        if (NumDataBytes) // encode the next byte if available
        {
            if (!!(*pData & 0x80)) // check if msb is set in the byte, !! converts to boolean
            {
                msb |= (1U << i); // set the msb bit for this data byte
            }
            *pDest = (*pData & 0x7F); // remove the msb from the data byte then store it into output buffer
            pDest++;                  // increment the output buffer pointer
            pData++;                  // increment the pointer to the next data byte
            NumDataBytes--;           // decrement the remaining number of bytes to encode
            numEncoded++;
        }
        else // otherwise pad with zeros
        {
            *pDest = 0x00;
            pDest++;
        }
    }
    *pDest = (msb & 0x7F); // store the MSB byte in the final position
    return numEncoded;
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
        1. Set bit N in the MSB byte equal to the MSB of data byte.
        2. Mask off the MSB of data byte N and add it to the encode buffer
    }
    
    If a block contains partial data, it is padded upto the 8 bytes with zeros.
*/
static void EncodeAndTransmit(u8* pData, u16 NumDataBytes)
{
    if (!pData || NumDataBytes == 0)
    {
        return;
    }

    u16 totalBlocks = EncodedLength_Blocks(NumDataBytes);
// #ifdef ENABLE_SERIAL
//     char buf[64] = {0};
//     sprintf(buf, "InCnt: %d - NumEncBlocks: %d\r\n", NumDataBytes, totalBlocks);
//     Serial_Print(buf);
// #endif
    for (u16 currentBlock = 0; currentBlock < totalBlocks; currentBlock++)
    {
        memset(mOutBuffer, 0x00, OUTBUFFER_LEN);
        u8 numEncoded = EncodeSysexBlock(pData, NumDataBytes, mOutBuffer, OUTBUFFER_LEN);
        if (numEncoded)
        {
            NumDataBytes -= numEncoded;
            pData += numEncoded;
            TransmitSysexData(mOutBuffer, OUTBUFFER_LEN);
        }
    }
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

/**
 * @brief Calculates the number of bytes required to encode NumDataBytes bytes;
 * 
 * @param NumDataBytes Number of bytes in input data stream.
 * @return u32 Number of output bytes required to encode to SysEx;
 */
static u32 EncodedLength_Bytes(u16 NumDataBytes)
{
    if (NumDataBytes == 0)
    {
        return 0;
    }
    else if (NumDataBytes < SIZEOF_SYSEXBLOCK)
    {
        return SIZEOF_SYSEXBLOCK; // atleast 8 bytes required
    }

    return (NumDataBytes + ceil(NumDataBytes / 7.0));
}

/**
 * @brief Calculates the number of blocks required to encode NumDataBytes bytes
 * 
 * @param NumDataBytes Number of bytes in input data stream.
 * @return u32 Number of 8-byte blocks required to encode to SysEx
 */
static u32 EncodedLength_Blocks(u16 NumDataBytes)
{
    if (NumDataBytes == 0)
    {
        return 0;
    }
    else if (NumDataBytes < SIZEOF_SYSEXBLOCK)
    {
        return 1; // atleast 1 block required
    }

    return ceil((double)EncodedLength_Bytes(NumDataBytes) / SIZEOF_SYSEXBLOCK);
}

/**
 * @brief Calculates the number of bytes required to decode from a SysEx encoded stream;
 * 
 * @param NumEncodedBytes Number of bytes in SysEx encoded data stream
 * @return u16 Number of bytes required to decode from SysEx
 */
static u16 DecodedLength_Bytes(u32 NumEncodedBytes)
{
    if (NumEncodedBytes < 8)
    {
        return 0;
    }

    return (NumEncodedBytes - (NumEncodedBytes / SIZEOF_SYSEXBLOCK));
}

static bool Msg_SendHandler(u8* pBytes, u16 NumBytes)
{
    if (!pBytes || NumBytes == 0)
    {
        return false;
    }

    TransmitSysexByte(MIDI_CMD_SYSEX_START);
    // TransmitSysexByte(SYSEX_NONRT);
    // TransmitSysexByte(mSysexChannel);

    for (u8 i = 0; i < sizeof(MANF_ID); i++)
    {
        TransmitSysexByte(pgm_read_byte(&MANF_ID[i]));
    }

    u16 numBlocks = (u16)EncodedLength_Blocks(NumBytes);
    EncodeAndTransmit(&numBlocks, sizeof(numBlocks));
    EncodeAndTransmit(pBytes, NumBytes);
    TransmitSysexByte(MIDI_CMD_SYSEX_END);

    return true;
}

static inline void TransmitMidiCC(u8 Channel, u8 CC, u8 Value)
{
    MIDI_EventPacket_t packet = {0};
    packet.Event              = MIDI_EVENT(0, MIDI_COMMAND_CONTROL_CHANGE);
    packet.Data1              = (Channel & 0x0F) | MIDI_COMMAND_CONTROL_CHANGE;
    packet.Data2              = CC & 0x7F;
    packet.Data3              = Value & 0x7F;
    MIDI_Device_SendEventPacket(&gMIDI_Interface, &packet);
}

static inline void TransmitMidiNote(u8 _Channel, u8 _Note, u8 _Velocity, bool _NoteOn)
{
    MIDI_EventPacket_t _packet = {0};
    u8                 _cmd    = _NoteOn ? MIDI_COMMAND_NOTE_ON : MIDI_COMMAND_NOTE_OFF;
    _packet.Event              = MIDI_EVENT(0, _cmd);
    _packet.Data1              = (_Channel & 0x0F) | _cmd;
    _packet.Data2              = _Note & 0x7F;
    _packet.Data3              = _Velocity & 0x7F;
    MIDI_Device_SendEventPacket(&gMIDI_Interface, &_packet);
}

static inline void TransmitSysexByte(u8 _Byte)
{
    MIDI_EventPacket_t _msg;
    _msg.Event = MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_1BYTE);
    _msg.Data1 = _Byte;
    MIDI_Device_SendEventPacket(&gMIDI_Interface, &_msg);
}

static void SysExProcess_NonRT(eMidiSysExNonRealtime SubId)
{
    // switch (SubId)
    // {
    //     case GENERAL_SYS_ID_REQ:
    //     {
    //         // u8* b = &mOutBuffer[0];

    //         // *b++ = MIDI_CMD_SYSEX_START;

    //         // *b++ = SYSEX_NONRT;
    //         // *b++ = SYSEX_BROADCAST_CH;
    //         // *b++ = MIDI_SYSEX_NONRT_GENERAL_SYS;
    //         // *b++ = GENERAL_SYS_ID_REP;

    //         // for (int i = 0; i < sizeof(MANF_ID); i++)
    //         //     *b++ = pgm_read_byte(&MANF_ID[i]);

    //         // for (int i = 0; i < sizeof(FMLY_ID); i++)
    //         //     *b++ = pgm_read_byte(&FMLY_ID[i]);

    //         // for (int i = 0; i < sizeof(PROD_ID); i++)
    //         //     *b++ = pgm_read_byte(&PROD_ID[i]);

    //         // for (int i = 0; i < sizeof(VRSN_ID); i++)
    //         //     *b++ = pgm_read_byte(&VRSN_ID[i]);

    //         // *b++ = MIDI_CMD_SYSEX_END;
    //         // Serial_Print("[MIDI TX] ID reply\r\n");
    //         // TransmitSysexData(mOutBuffer, b - &mOutBuffer[0]);
    //         break;
    //     }

    //     default: break;
    // }
}
