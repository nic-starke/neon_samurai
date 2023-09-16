/*
 * File: MIDICommandDefines.h ( 17th March 2022 )
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

#define MIDI_CHANNELS (16)

typedef enum
{
#define MIDI_CMD_DEF(type, str, val, dSize) MIDI_CMD_##type = val,
#include "MIDICommandDefineList.h"
} eMidiCommand;

typedef enum
{
#define MIDI_SYSEX_NON_REALTIME_DEF(type, str, subid) MIDI_SYSEX_NONRT_##type = subid,
#include "MIDICommandDefineList.h"
} eMidiSysExNonRealtime;

typedef enum
{
#define GENERAL_SYS_DEF(type, str, subid) GENERAL_SYS_##type = subid,
#include "MIDICommandDefineList.h"
} eGeneralSystemInfo;

typedef enum
{
#define MIDI_CC_DEF(type, str, val) MIDI_CC_##type = val,
#include "MIDICommandDefineList.h"
} eMidiControlChange;

typedef enum
{
#define MUFFIN_MIDICMD_DEF(type, str, val, inSize, outSize) MMCMD_##type = val,
#include "MIDIMuffinCommands.h"
} eMuffinMidiCommand;
