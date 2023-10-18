/*
 * File: MIDI.h ( 16th March 2022 )
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

#pragma once

#include <avr/pgmspace.h>

#include "Comms/CommsTypes.h"
#include "core/types.h"
#include "subprojects/lufa/LUFA/Drivers/USB/USB.h"
#include "Input/Encoder.h"
#include "MIDICommandDefines.h"

void MIDI_Init(void);
void MIDI_Update(void);
void MIDI_MirrorInput(bool Enable);
void MIDI_ProcessMessage(MIDI_EventPacket_t* pMsg);
void MIDI_ProcessLayer(sEncoderState*        pEncoderState,
                       sVirtualEncoderLayer* pLayer, uint8_t ValueToTransmit);
