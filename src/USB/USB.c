/*
 * File: USB.c ( 8th November 2021 )
 * Project: Muffin
 * Copyright 2021 bxzn (mail@bxzn.one)
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

#include "Common.h"
#include "Descriptors.h"

#include "USB.h"
#include "Encoder.h"
#include "DataTypes.h"
#include "Input.h"
#include "Display.h"

bool mMirrorInput = false;

// clang-format off
static const USB_ClassInfo_MIDI_Device_t mMIDI_Interface = {
    .Config = 
    {
        .DataINEndpoint = 
        {
            .Address = MIDI_STREAM_IN_EPADDR,
            .Size    = MIDI_STREAM_EPSIZE,
            .Banks   = 1,
        },
        .DataOUTEndpoint = 
        {
            .Address = MIDI_STREAM_OUT_EPADDR,
            .Size    = MIDI_STREAM_EPSIZE,
            .Banks   = 1,
        }
    }
};
// clang-format on

void USBMidi_Init(void)
{
}

void USBMidi_Receive(void)
{
    MIDI_EventPacket_t rx;

    if (MIDI_Device_ReceiveEventPacket(&mMIDI_Interface, &rx))
    {
        if (mMirrorInput)
        {
            MIDI_Device_SendEventPacket(&mMIDI_Interface, &rx);
        }

        switch(rx.Event)
        {
            case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_2BYTE):
            case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_3BYTE):
            case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_START_3BYTE):
            case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_1BYTE):
            case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_2BYTE):
            case MIDI_EVENT(0, MIDI_COMMAND_SYSEX_END_3BYTE):
            {
                Display_Flash(500, 2);
                break;
            }
        }
        // switch on the event type - handle.
        // if (rx.Event == MIDI_EVENT(0, MIDI_COMMAND_SYSEX_START_3BYTE))
        // {

        // }
    }
}

// Replies to every midi packet by sending a copy back
void USBMidi_MirrorInput(bool Enable)
{
    mMirrorInput = Enable;
}

static inline void TransmitMidiCC(u8 Channel, u8 CC, u8 Value)
{
    MIDI_EventPacket_t packet = {0};
    packet.Event              = MIDI_EVENT(0, MIDI_COMMAND_CONTROL_CHANGE);
    packet.Data1              = (Channel & 0x0F) | MIDI_COMMAND_CONTROL_CHANGE;
    packet.Data2              = CC & 0x7F;
    packet.Data3              = Value & 0x7F;
    MIDI_Device_SendEventPacket(&mMIDI_Interface, &packet);
}

static inline void TransmitMidiNote(u8 Channel, u8 Note, u8 Velocity, bool NoteOn)
{
    MIDI_EventPacket_t packet = {0};
    u8                 cmd    = NoteOn ? MIDI_COMMAND_NOTE_ON : MIDI_COMMAND_NOTE_OFF;
    packet.Event              = MIDI_EVENT(0, cmd);
    packet.Data1              = (Channel & 0x0F) | cmd;
    packet.Data2              = Note & 0x7F;
    packet.Data3              = Velocity & 0x7F;
    MIDI_Device_SendEventPacket(&mMIDI_Interface, &packet);
}

void USBMidi_ProcessLayer(sEncoderState* pEncoderState, sVirtualEncoderLayer* pLayer, u8 ValueToTransmit)
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

// Must be called prior to LUFAs master usb task - USB_USBTask()
void USBMidi_Update(void)
{
    if (USB_IsInitialized)
    {
        USBMidi_Receive();
        MIDI_Device_USBTask(&mMIDI_Interface); // this calls MIDI_Device_Flush
    }
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
    bool ConfigSuccess = true;
    ConfigSuccess &= MIDI_Device_ConfigureEndpoints(&mMIDI_Interface);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
    MIDI_Device_ProcessControlRequest(&mMIDI_Interface);
}