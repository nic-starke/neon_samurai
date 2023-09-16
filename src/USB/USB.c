// /*
//  * File: USB.c ( 8th November 2021 )
//  * Project: Muffin
//  * Copyright 2021 - 2021 Nic Starke (mail@bxzn.one)
//  * -----
//  * This program is free software: you can redistribute it and/or modify
//  * it under the terms of the GNU General Public License as published by
//  * the Free Software Foundation, either version 3 of the License, or
//  * (at your option) any later version.
//  *
//  * This program is distributed in the hope that it will be useful,
//  * but WITHOUT ANY WARRANTY; without even the implied warranty of
//  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  * GNU General Public License for more details.
//  *
//  * You should have received a copy of the GNU General Public License
//  * along with this program. If not, see http://www.gnu.org/licenses/.
//  */

// #include "LUFA/Common/Common.h"
// #include "Descriptors.h"

// #include "USB.h"
// #include "Encoder.h"
// #include "Types.h"

// static const USB_ClassInfo_MIDI_Device_t mMIDI_Interface =
// {
// 	.Config =
// 	{
// 		.DataINEndpoint =
// 		{
// 			.Address	= MIDI_STREAM_IN_EPADDR,
// 			.Size		= MIDI_STREAM_EPSIZE,
// 			.Banks		= 1,
// 		},

// 		.DataOUTEndpoint =
// 		{
// 			.Address	= MIDI_STREAM_OUT_EPADDR,
// 			.Size		= MIDI_STREAM_EPSIZE,
// 			.Banks		= 1,
// 		}
// 	}
// 	};

// void USBMidi_Init(void)
// {

// }

// void USBMidi_Receive(void)
// {
// 	MIDI_EventPacket_t rx;

// 	if( MIDI_Device_ReceiveEventPacket(&mMIDI_Interface, &rx) ) {
// 		// switch on the event type - handle.
// 	}

// }

// inline static void TransmitMidiCC(u8 Channel, u8 CC, u8 Value)
// {
// 	MIDI_EventPacket_t packet = {0};
// 	packet.Event = MIDI_EVENT(0, MIDI_COMMAND_CONTROL_CHANGE);
// 	packet.Data1 = (Channel & 0x0F) | MIDI_COMMAND_CONTROL_CHANGE;
// 	packet.Data2 = CC & 0x7F;
// 	packet.Data3 = Value & 0x7F;
// 	MIDI_Device_SendEventPacket(&mMIDI_Interface, &packet);
// }

// inline static void TransmitMidiNote(u8 Channel, u8 Note, u8 Velocity, bool
// NoteOn)
// {
// 	MIDI_EventPacket_t packet = {0};
// 	u8 cmd = NoteOn ? MIDI_COMMAND_NOTE_ON : MIDI_COMMAND_NOTE_OFF;
// 	packet.Event = MIDI_EVENT(0, cmd);
// 	packet.Data1 = (Channel & 0x0F) | cmd;
// 	packet.Data2 = Note & 0x7F;
// 	packet.Data3 = Velocity & 0x7F;
// 	MIDI_Device_SendEventPacket(&mMIDI_Interface, &packet);
// }

// inline static void ProcessKnobMIDI(eKnobMidiMode Mode, u8 Channel, u8 CC, u16
// Value, u16 PreviousValue)
// {
// 	switch((eKnobMidiMode)Mode)
// 	{
// 		case KNOB_MIDI_CC:
// 		{
// 			TransmitMidiCC(Channel, CC, Value >> 9);	// convert to 7 bit
// value (probably not a good way to do this) 			break;
// 		}

// 		case KNOB_MIDI_REL_CC:
// 		{
// 			if(PreviousValue > Value)
// 			{
// 				TransmitMidiCC(Channel, CC, 0x3F);
// 			}
// 			else if (PreviousValue < Value)
// 			{
// 				TransmitMidiCC(Channel, CC, 0x41);
// 			}
// 			break;
// 		}

// 		case KNOB_MIDI_NOTE:
// 		{
// 			TransmitMidiNote(Channel, CC, Value >> 9, true);
// 			break;
// 		}

// 		case KNOB_DISABLED:
// 		default:
// 		break;
// 	}
// }

// void USBMidi_ProcessKnob(sKnob* pKnob)
// {
// 	if (pKnob->Mode != KNOB_DISABLED)
// 	{
// 		ProcessKnobMIDI(pKnob->Mode, pKnob->MidiChannel, pKnob->MidiCC,
// pKnob->CurrentValue, pKnob->PreviousValue);
// 	}

// 	if (pKnob->SuperMode != KNOB_DISABLED && (pKnob->CurrentValue >=
// pKnob->SuperMinValue) && (pKnob->CurrentValue <= pKnob->SuperMaxValue))
// 	{
// 		ProcessKnobMIDI(pKnob->SuperMode, pKnob->SuperMidiChannel,
// pKnob->SuperCC, pKnob->CurrentValue, pKnob->PreviousValue);
// 	}
// }

// void USBMidi_ProcessEncoderSwitch(sSwitch* pSwitch)
// {

// }

// // Must be called prior to LUFAs master usb task - USB_USBTask()
// void USBMidi_Update(void)
// {
// 	if(USB_IsInitialized)
// 	{
// 		//USBMidi_Receive();
// 		MIDI_Device_USBTask(&mMIDI_Interface); // this calls MIDI_Device_Flush
// 	}
// }

// /** Event handler for the library USB Connection event. */
// void EVENT_USB_Device_Connect(void)
// {

// }

// /** Event handler for the library USB Disconnection event. */
// void EVENT_USB_Device_Disconnect(void)
// {

// }

// /** Event handler for the library USB Configuration Changed event. */
// void EVENT_USB_Device_ConfigurationChanged(void)
// {
// 	bool ConfigSuccess = true;
// 	ConfigSuccess &= MIDI_Device_ConfigureEndpoints(&mMIDI_Interface);
// }

// /** Event handler for the library USB Control Request reception event. */
// void EVENT_USB_Device_ControlRequest(void)
// {
// 	MIDI_Device_ProcessControlRequest(&mMIDI_Interface);
// }