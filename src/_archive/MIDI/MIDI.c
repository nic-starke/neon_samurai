/*
 * File: MIDI.c ( 17th March 2022 )
 * Project: Muffin
 * Copyright 2022 Nicolaus Starke  
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

#include "Comms/Comms.h"
#include "Config.h"
#include "system/types.h"
#include "Display/Display.h"
#include "MIDI/MIDI.h"
#include "USB/USB.h"

#define ENABLE_SERIAL
#include "USB/VirtualSerial.h"

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
static uint8_t           mOutBuffer[OUTBUFFER_LEN] = {0};
static bool         mMirrorInput              = false;
static uint8_t           mSysexChannel             = SYSEX_CH;

static const uint8_t MANF_ID[3] PROGMEM = {0x00, 0x48, 0x01}; //TODO is it worth moving this to progmem?
static const uint8_t FMLY_ID[2] PROGMEM = {0x00, 0x00};
static const uint8_t PROD_ID[2] PROGMEM = {0x00, 0x01};
static const uint8_t VRSN_ID[4] PROGMEM = {0x00, 0x00, 0x00, VERSION};

static bool Msg_SendHandler(uint8_t* pBytes, uint16_t NumBytes);

static void EncodeAndTransmit(uint8_t* pData, uint16_t NumDataBytes);
static void DecodeFromSysex(uint8_t* pSysexBytes, uint8_t* pDestBuffer, uint16_t NumBlocks);
static uint32_t  EncodedLength_Bytes(uint16_t NumDataBytes);
static uint32_t  EncodedLength_Blocks(uint16_t NumDataBytes);
static uint16_t  DecodedLength_Bytes(uint32_t NumEncodedBytes);

static inline void TransmitMidiCC(uint8_t Channel, uint8_t CC, uint8_t Value);
static inline void TransmitMidiNote(uint8_t Channel, uint8_t Note, uint8_t Velocity, bool NoteOn);
static inline void TransmitSysexByte(uint8_t Byte);

static void SysExProcess_NonRT(eMidiSysExNonRealtime SubId);

/**
 * @brief The midi module init function.
 */
void MIDI_Init(void)
{
    Comms_RegisterProtocol(PROTOCOL_USBMIDI, Msg_SendHandler);
}

// Must be called prior to LUFAs master usb task - USB_USBTask()
/**
 * @brief The midi module update function.
 * To be called within the main loop.
 * WARNING - this should probably be called before the LUFA's USB_USBTask(),
 * otherwise all midi events will be one loop behind. 
 */
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

/**
 * @brief Enable/disable MIDI mirroring.
 * All received midi messages will be retransmitted.
 * 
 * @param Enable - True to enable, false to disable.
 */
void MIDI_MirrorInput(bool Enable)
{
    mMirrorInput = Enable;
}

/**
 * @brief The main midi process function for a virtual encoder layer.
 * This function should only be called once per main loop.
 * It handles transmitting midi values based on a virtual encoder layers current configuration.
 * 
 * @param pEncoderState A pointer to the encode state.
 * @param pLayer A pointer to the layer to be processed.
 * @param ValueToTransmit The actual output value to be sent (if appropriate)
 */
void MIDI_ProcessLayer(sEncoderState* pEncoderState, sVirtualEncoderLayer* pLayer, uint8_t ValueToTransmit)
{
    //TODO the ValueToTransmit argument seems very odd here - instead perform a lookup on the hardware encoder and calculate the necessary value?
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

/**
 * @brief Transmit an arbitrary byte stream as MIDI sysex.
 * 
 * @param pData A pointer to the start of the byte stream.
 * @param DataLength The number of bytes to be transmitted.
 */
static void TransmitDataAsSysex(uint8_t* pData, uint16_t DataLength)
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

/**
 * @brief Handle an incoming midi message.
 * 
 * @param pMsg A pointer to the Lufa midi msg event.
 */
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

/**
 * @brief Encodes 7 bytes of input data into an 8 byte sysex block.
 * The destination buffer must be at least 8 bytes in length.
 * 
 * @param pData A pointer to the byte stream to encode.
 * @param NumDataBytes The number of bytes in the byte stream.
 * @param pDest A destination buffer - must be at least 8 bytes regardless of the NumDataBytes
 * @param DestBufferLen The output buffer length
 * @return uint16_t The number of bytes that were encoded.
 */
static uint16_t EncodeSysexBlock(uint8_t* pData, uint16_t NumDataBytes, uint8_t* pDest, uint8_t DestBufferLen)
{
    if (DestBufferLen < SIZEOF_SYSEXBLOCK || NumDataBytes == 0)
    {
        return 0;
    }

    uint8_t msb        = 0;
    uint8_t numEncoded = 0;

    for (uint8_t i = 0; i < 7; i++) // encode 7 bytes
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

/**
 * @brief Encode and transmit an arbitrary length byte stream.
 * The data will be encoded into 8-byte sysex blocks (see EncodeSysexBlock).
 * It will then be transmitted via sysex immediately.
 * 
 * @param pData A pointer to a byte array/stream.
 * @param NumDataBytes The number of bytes in the array/stream.
 */
static void EncodeAndTransmit(uint8_t* pData, uint16_t NumDataBytes)
{
    if (!pData || NumDataBytes == 0)
    {
        return;
    }

    uint16_t totalBlocks = EncodedLength_Blocks(NumDataBytes);
    // #ifdef ENABLE_SERIAL
    //     char buf[64] = {0};
    //     sprintf(buf, "InCnt: %d - NumEncBlocks: %d\r\n", NumDataBytes, totalBlocks);
    //     Serial_Print(buf);
    // #endif
    for (uint16_t currentBlock = 0; currentBlock < totalBlocks; currentBlock++)
    {
        memset(mOutBuffer, 0x00, OUTBUFFER_LEN);
        uint8_t numEncoded = EncodeSysexBlock(pData, NumDataBytes, mOutBuffer, OUTBUFFER_LEN);
        if (numEncoded)
        {
            NumDataBytes -= numEncoded;
            pData += numEncoded;
            TransmitDataAsSysex(mOutBuffer, OUTBUFFER_LEN);
        }
    }
}

/**
 * @brief Decodes an incoming sysex-block encoded stream into a byte stream.
 * 
 * @param pSysexBytes A pointer to the encoded byte array/stream.
 * @param pDestBuffer A pointer to a destination buffer.
 * @param NumBlocks The number of 8-byte sysex blocks to be decoded.
 */
static void DecodeFromSysex(uint8_t* pSysexBytes, uint8_t* pDestBuffer, uint16_t NumBlocks)
{
    for (uint16_t block = 0; block < NumBlocks; block++)
    {
        uint8_t* pSysexBlock = &pSysexBytes[(block * 8)];
        for (uint8_t i = 0; i < 7; i++)
        {
            // Extract the msb for the data byte from the MSB byte
            uint8_t msb         = (pSysexBlock[7] & (0x01 << i));
            // insert MSB into the data byte, then push into dest buffer
            *pDestBuffer++ = (pSysexBlock[i] | msb);
        }
    }
}

/**
 * @brief Calculates the number of bytes required to encode NumDataBytes bytes;
 * 
 * @param NumDataBytes Number of bytes in input data stream.
 * @return uint32_t Number of output bytes required to encode to SysEx;
 */
static uint32_t EncodedLength_Bytes(uint16_t NumDataBytes)
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
 * @return uint32_t Number of 8-byte blocks required to encode to SysEx
 */
static uint32_t EncodedLength_Blocks(uint16_t NumDataBytes)
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
 * @return uint16_t Number of bytes required to decode from SysEx
 */
static uint16_t DecodedLength_Bytes(uint32_t NumEncodedBytes)
{
    if (NumEncodedBytes < 8)
    {
        return 0;
    }

    return (NumEncodedBytes - (NumEncodedBytes / SIZEOF_SYSEXBLOCK));
}

/**
 * @brief The comms message handler for the MIDI module.
 * 
 * @param pBytes A pointer to a byte array to transmit.
 * @param NumBytes The number of bytes in the array to be transmitted.
 * @return True on success, false on failure.
 */
static bool Msg_SendHandler(uint8_t* pBytes, uint16_t NumBytes)
{
    if (!pBytes || NumBytes == 0)
    {
        return false;
    }

    TransmitSysexByte(MIDI_CMD_SYSEX_START);
    // TransmitSysexByte(SYSEX_NONRT);
    // TransmitSysexByte(mSysexChannel);

    for (uint8_t i = 0; i < sizeof(MANF_ID); i++)
    {
        TransmitSysexByte(pgm_read_byte(&MANF_ID[i]));
    }

    uint16_t numBlocks = (uint16_t)EncodedLength_Blocks(NumBytes);
    EncodeAndTransmit(&numBlocks, sizeof(numBlocks));
    EncodeAndTransmit(pBytes, NumBytes);
    TransmitSysexByte(MIDI_CMD_SYSEX_END);

    return true; //TODO - no failure states?
}

/**
 * @brief Transmit a MIDI CC message
 * 
 * @param Channel The midi channel
 * @param CC The CC type
 * @param Value The CC value
 */
static inline void TransmitMidiCC(uint8_t Channel, uint8_t CC, uint8_t Value)
{
    MIDI_EventPacket_t packet = {0};
    packet.Event              = MIDI_EVENT(0, MIDI_COMMAND_CONTROL_CHANGE);
    packet.Data1              = (Channel & 0x0F) | MIDI_COMMAND_CONTROL_CHANGE;
    packet.Data2              = CC & 0x7F;
    packet.Data3              = Value & 0x7F;
    MIDI_Device_SendEventPacket(&gMIDI_Interface, &packet);
}

/**
 * @brief Transmit a midi note message
 * 
 * @param _Channel The midi channel
 * @param _Note The note value
 * @param _Velocity The velocity value
 * @param _NoteOn True if note on, false if note off.
 */
static inline void TransmitMidiNote(uint8_t _Channel, uint8_t _Note, uint8_t _Velocity, bool _NoteOn)
{
    MIDI_EventPacket_t _packet = {0};
    uint8_t                 _cmd    = _NoteOn ? MIDI_COMMAND_NOTE_ON : MIDI_COMMAND_NOTE_OFF;
    _packet.Event              = MIDI_EVENT(0, _cmd);
    _packet.Data1              = (_Channel & 0x0F) | _cmd;
    _packet.Data2              = _Note & 0x7F;
    _packet.Data3              = _Velocity & 0x7F;
    MIDI_Device_SendEventPacket(&gMIDI_Interface, &_packet);
}

/**
 * @brief Transmit a single sysex byte.
 * 
 * @param _Byte The byte to transmit.
 */
static inline void TransmitSysexByte(uint8_t _Byte)
{
    MIDI_EventPacket_t _msg;
    _msg.Event = MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_1BYTE);
    _msg.Data1 = _Byte;
    MIDI_Device_SendEventPacket(&gMIDI_Interface, &_msg);
}

/**
 * @brief A stub function to handle non-realtime sysex messages.
 * 
 * @param SubId The non-realtime midi message type/parameter.
 */
static void SysExProcess_NonRT(eMidiSysExNonRealtime SubId)
{
    // switch (SubId)
    // {
    //     case GENERAL_SYS_ID_REQ:
    //     {
    //         // uint8_t* b = &mOutBuffer[0];

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
    //         // TransmitDataAsSysex(mOutBuffer, b - &mOutBuffer[0]);
    //         break;
    //     }

    //     default: break;
    // }
}
