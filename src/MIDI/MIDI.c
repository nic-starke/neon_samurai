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

#include "MIDI.h"
#include "Display.h"
#include "USB.h"
#include "DataTypes.h"

typedef enum
{
    PARSER_INIT,
    PARSER_READ_COMMAND,
    PARSER_READ_SYSEX_STREAM,
    PARSER_GOT_COMMAND,
    PARSER_GOT_SYSEX,
    PARSER_DONE,

    PARSER_ERROR,

    NUM_PARSER_STATES,
} eParserState;

static eParserState mParserState = PARSER_INIT;

static const u8 SYSEX_HEADER[] = {
    0xF0,             // start of sysex
    0x00, 0x48, 0x01, // manufacturer id
    0x00, 0x01        // product id
};

typedef struct
{
    u8 MSBs;
    u8 Data[7];
} sSysExPacket;

typedef struct
{
    u8 header[sizeof(SYSEX_HEADER)];
    u8 dataLen;
} sMuffinSysex;

// Transmit sysex data via the lufa midi event packet
// This does not add sysex headers - ensure pData is a valid midi sysex message!
static void TransmitSysexData(u8* pData, u16 DataLength)
{
    for (int i = 0; i < DataLength; i++)
    {
        u8                 numBytes = 0;
        MIDI_EventPacket_t msg;
        if ((DataLength - i) > 3)
        {
            numBytes  = 3;
            msg.Event = MIDI_EVENT(0, 0x40);
        }
        else
        {
            numBytes  = (DataLength - 1);
            msg.Event = MIDI_EVENT(0, 0x50 + ((numBytes - 1) * 16));
        }

        for (int byte = 0; byte < numBytes; byte++)
        {
            msg.Data[byte] = *pData++;
        }
        MIDI_Device_SendEventPacket(&gMIDI_Interface, &msg);
        MIDI_Device_Flush(&gMIDI_Interface);
        DataLength -= numBytes;
        i += numBytes;
    }
}

static inline void TransmitSysexByte(u8 Byte)
{
    MIDI_EventPacket_t msg;
    msg.Event = MIDI_EVENT(0, 0x50);
    msg.Data1 = Byte;
    MIDI_Device_SendEventPacket(&gMIDI_Interface, &msg);
}

void MIDI_SendMMCMD(u8* pData, eMuffinMidiCommand Command, u16 DataLength)
{
    TransmitSysexData(SYSEX_HEADER, sizeof(SYSEX_HEADER));
    TransmitSysexByte(Command);
    TransmitSysexData(pData, DataLength);
    TransmitSysexByte(MIDI_CMD_SYSEX_END);
}

// void MIDI_ProcessMessage(MIDI_EventPacket_t* pMsg)
// {
//     static u8 bytesRead = 0;

//     if (pMsg->Event == MIDI_EVENT(0, MIDI_COMMAND_SYSEX_1BYTE) 0x80)
//     switch (pMsg->Event)
//     {
//         case
//     }
// }

// static inline uint8_t CmdDataSize(uint8_t MidiCommand)
// {
//     switch (MidiCommand)
//     {
//         case MIDI_CMD_NOTE_OFF:
//         case MIDI_CMD_NOTE_ON:
//         case MIDI_CMD_AFTERTOUCH:
//         case MIDI_CMD_CC:
//         case MIDI_CMD_PITCH_BEND:
//         case MIDI_CMD_MTC_QUARTER:
//         case MIDI_CMD_SONG_POS:
//         case MIDI_CMD_SONG_SELECT: return 2;

//         case MIDI_CMD_PC:
//         case MIDI_CMD_CHANNEL_PRESSURE: return 1;

//         case MIDI_CMD_SYSEX_START:
//         case MIDI_CMD_SYSEX_END:
//         case MIDI_CMD_TUNE_REQUEST:
//         case MIDI_CMD_CLOCK:
//         case MIDI_CMD_START:
//         case MIDI_CMD_CONTINUE:
//         case MIDI_CMD_STOP:
//         case MIDI_CMD_SENSING:
//         case MIDI_CMD_RESET: return 0;
//     }

//     return 0;
// }

// void MidiParser_ParseMessage(MIDI_EventPacket_t* pMsg)
// {
//     while (mParserState != PARSER_DONE)
//     {
//         switch (mParserState)
//         {
//             case PARSER_INIT:
//             {
//                 // Split upper and lower status byte into command and channel
//                 eMidiCommand cmd = pMsg->Event & 0xF0;
//                 u8 channel = pMsg->Event & 0x0F;

//                 if (cmd == MIDI_CMD_SYSEX_START)
//                 {
//                     mParserState  = PARSER_READ_SYSEX_STREAM;
//                     mSysExMsg.DataLen = 0;
//                 }
//                 else
//                 {
//                     mParserState = PARSER_READ_COMMAND;
//                 }
//             }
//             break;

//             case PARSER_READ_COMMAND:
//             {
//                 uint8_t expectedSize = CmdDataSize(cmd);
//                 if ((MessageSize - 1) != expectedSize)
//                 {
//                     mParserState = PARSER_ERROR;
//                     break;
//                 }

//                 for (int i = 0; i < MessageSize - 1; i++)
//                 {
//                     pMsg->Data[i] = Message[i + 1];
//                 }

//                 pMsg->DataLen = MessageSize - 1;

//                 mParserState = PARSER_GOT_COMMAND;
//                 break;
//             }

//             case PARSER_READ_SYSEX_STREAM:
//             {
//                 for (int i = 1; i < MessageSize - 1; i++)
//                 {
//                     if (Message[i] == MIDI_CMD_SYSEX_END)
//                     {
//                         mParserState = PARSER_GOT_SYSEX;
//                         break;
//                     }
//                     else if (mSysExMsg.DataLen >= MIDI_MSG_MAX_LEN)
//                     {
//                         mParserState = PARSER_ERROR;
//                         break;
//                     }
//                     else
//                     {
//                         mSysExMsg.Data[mSysExMsg.DataLen] = Message[i];

//                         mSysExMsg.DataLen++;
//                     }
//                 }
//                 break; // probably need a timeout here if no sysex end is received this locks up...
//             }

//             case PARSER_GOT_COMMAND:
//             {
//                 // add message to the midi.c update() queue - let it deal with it
//                 // dont have a queue here...

//                 mMsgParser.Queue.Index = (mMsgParser.Queue.Index + 1) % MIDI_MSG_Q_LEN;

//                 mParserState = PARSER_DONE;
//                 break;
//             }

//             case PARSER_GOT_SYSEX:
//             {
//                 // as above - add message to external queue, let it deal with it.
//                 mParserState = PARSER_DONE;
//                 break;
//             }

//             case PARSER_DONE:
//             {
//                 break;
//             }

//             case PARSER_ERROR:
//             default:
//             {
//                 // clear the current message
//                 pMsg->Channel = 0;
//                 cmd = 0;
//                 pMsg->DataLen = 0;
//                 pMsg->Data[0] = 0;
//                 pMsg->Data[1] = 0;

//                 mParserState = PARSER_DONE;
//                 LogError(MIDI_MSG_PARSER_ERROR);
//                 break;
//             }
//         }
//     }

//     mParserState = PARSER_INIT;
// }

void MIDI_ProcessMessage(MIDI_EventPacket_t* pMsg)
{
    // switch (pMsg->Event)
    // {
    //     case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_1BYTE): break;
    //     case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_2BYTE): break;
    //     case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_3BYTE): break;

    //     case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_START_3BYTE):
    //     {
    //         switch(mParserState)
    //         {
    //             case PARSER_INIT:
    //             {
    //                 // sysex non-realtime on channel 0
    //                 if (pMsg->Data1 == 0xF0 && pMsg->Data2 == 0x7E && pMsg->Data3 == 0x00)
    //                 {
    //                     mParserState = PARSER_READ_SYSEX_STREAM;
    //                     Display_Flash(200, 1);
    //                 }
    //                 break;
    //             }

    //             case PARSER_READ_SYSEX_STREAM:
    //             {
    //                 if(pMsg->Data1 == MMCMD_HEADER[0] && pMsg->Data2 == MMCMD_HEADER[1] && pMsg->Data3 == MMCMD_HEADER[2])
    //                 {
    //                     mParserState = PARSER_READ_COMMAND;
    //                     Display_Flash(200, 1);
    //                 }
    //                 break;
    //             }

    //             case PARSER_READ_COMMAND:
    //             {
    //                 switch((eMuffinMidiCommand) pMsg->Data1)
    //                 {
    //                     case MMCMD_MED_CONNECT:
    //                     {
    //                         Display_Flash(200, 1);
    //                         mParserState = PARSER_INIT;
    //                         break;
    //                     }

    //                     default:
    //                     break;
    //                 }
    //             }

    //             default:
    //             break;
    //         }

    //         break;
    //     }

    //     default:
    //     //case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_1BYTE):
    //     case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_2BYTE):
    //     case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_3BYTE):
    //     {
    //         break;
    //     }
    // }

    // switch((eMidiCommand)pMsg->Event)
    // {
    //     case MIDI_CMD_SYSEX_START:
    //     case MIDI_CMD_SYSEX_END:

    //     default:
    //     case MIDI_CMD_NOTE_OFF:
    //     case MIDI_CMD_NOTE_ON:
    //     case MIDI_CMD_AFTERTOUCH:
    //     case MIDI_CMD_CC:
    //     case MIDI_CMD_PC:
    //     case MIDI_CMD_CHANNEL_PRESSURE:
    //     case MIDI_CMD_PITCH_BEND:
    //     case MIDI_CMD_MTC_QUARTER:
    //     case MIDI_CMD_SONG_POS:
    //     case MIDI_CMD_SONG_SELECT:
    //     case MIDI_CMD_TUNE_REQUEST:
    //     case MIDI_CMD_CLOCK:
    //     case MIDI_CMD_START:
    //     case MIDI_CMD_CONTINUE:
    //     case MIDI_CMD_STOP:
    //     case MIDI_CMD_SENSING:
    //     case MIDI_CMD_RESET:
    //         break;
    // }
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