/*
 * File: MIDI.h ( 16th March 2022 )
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

#pragma once

#include <avr/pgmspace.h>

#include "CommDefines.h"
#include "DataTypes.h"
#include "Drivers/USB/USB.h"
#include "Encoder.h"
#include "MIDICommandDefines.h"

#define SIZEOF_SYSEXBLOCK    (8)

#define ENCODE_LEN(x)        ((u16)(x + ceil(x / 7.0))) // number of bytes after encoding to sysex - CAUTION, x must be greater than 7!!
#define ENCODE_LEN_BLOCKS(x) (ENCODE_LEN(x) / SIZEOF_SYSEXBLOCK)
#define DECODE_LEN(x)        (x - (x / SIZEOF_SYSEXBLOCK)) // number of bytes after decoding from sysex - CAUTION, x must be greater than 8!!

void MIDI_Init(void);
void MIDI_Update(void);
void MIDI_MirrorInput(bool Enable);
void MIDI_ProcessMessage(MIDI_EventPacket_t* pMsg);
void MIDI_ProcessLayer(sEncoderState* pEncoderState, sVirtualEncoderLayer* pLayer, u8 ValueToTransmit);
void MIDI_SendCommsMessage(sMessage* pMessage);