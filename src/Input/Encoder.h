/*
 * File: Encoder.h ( 27th November 2021 )
 * Project: Muffin
 * Copyright 2021 Nic Starke (mail@bxzn.one)
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

#include "DataTypes.h"
#include "RGB.h"

#define ENCODER_MIN_VAL (0)
#define ENCODER_MAX_VAL (UINT16_MAX)
#define ENCODER_MID_VAL (0x7FFF)

#define ACCEL_CONST       (225)
#define FINE_ADJUST_CONST (50)

#define NUM_VIRTUAL_ENCODERS (NUM_VIRTUAL_BANKS * NUM_ENCODERS * NUM_VIRTUAL_ENCODER_TYPES)
#define NUM_VIRTUAL_UBERS    (NUM_VIRTUAL_BANKS * NUM_ENCODERS * NUM_VIRTUAL_UBER_TYPES)
#define NUM_VIRTUAL_SWITCHES (NUM_VIRTUAL_BANKS * NUM_ENCODERS)

typedef enum
{
    VIRTUALBANK_A,
    VIRTUALBANK_B,
    VIRTUALBANK_C,

    NUM_VIRTUAL_BANKS,
} eVirtualBank;

typedef enum
{
    VIRTUALENCODER_PRIMARY,
    VIRTUALENCODER_SECONDARY,

    NUM_VIRTUAL_ENCODER_TYPES,
} eVirtualEncoderTypes;

typedef enum
{
    VIRTUALUBER_PRIMARY,
    VIRTUALUBER_SECONDARY,

    NUM_VIRTUAL_UBER_TYPES,
} eVirtualUberTypes;

typedef enum
{
    STYLE_DOT,
    STYLE_BAR,
    STYLE_BLENDED_BAR,

    NUM_DISPLAY_STYLES,
} eEncoderDisplayStyle;

// Mode is stored in 4 bits, max num modes is therefore 16
typedef enum
{
    MIDIMODE_DISABLED,
    MIDIMODE_CC,
    MIDIMODE_REL_CC,
    MIDIMODE_NOTE,
    // Switch only modes
    MIDIMODE_SW_CC_TOGGLE,
    MIDIMODE_SW_NOTE_HOLD,
    MIDIMODE_SW_NOTE_TOGGLE,

    NUM_MIDI_MODES,
} eMidiMode;

// Mode is stored in 4 bits, max num modes is therefore 16
typedef enum
{
    SWITCH_DISABLED,
    SWITCH_MIDI,
    SWITCH_RESET_VALUE_ON_PRESS,
    SWITCH_RESET_VALUE_ON_RELEASE,
    SWITCH_FINE_ADJUST_HOLD,
    SWITCH_FINE_ADJUST_TOGGLE,
    SWITCH_SECONDARY_HOLD,
    SWITCH_SECONDARY_TOGGLE,

    NUM_SWITCH_MODES,
} eSwitchMode;

typedef union
{
    u8 CC;
    u8 Note;
} uMidiValue;

typedef struct
{
    u8 Mode    : 4;
    u8 Channel : 4;

    uMidiValue MidiValue;
} sRotaryMidiConfig;

typedef struct
{
    u8 Mode    : 4;
    u8 Channel : 4;

    uMidiValue OnValue;
    uMidiValue OffValue;
} sSwitchMidiConfig;

typedef struct
{
    u16 CurrentValue;
    u16 PreviousValue;

    u8 DisplayStyle   : 2;
    u8 DisplayInvalid : 1;
    u8 FineAdjust     : 1;
    u8 HasDetent      : 1;
    u8 Reserved       : 3;

    sRotaryMidiConfig MidiConfig;
    sRGB              RGBColour;
    sRGB              DetentColour;
} sVirtualEncoder; // Runtime state of a virtual encoder

typedef struct
{
    u8 DisplayStyle   : 2;
    u8 DisplayInvalid : 1;
    u8 FineAdjust     : 1;
    u8 HasDetent      : 1;
    u8 Reserved       : 3;

    sRotaryMidiConfig MidiConfig;

    u16 RGBHue;
    u16 DetentHue;
} sEEVirtualEncoder; // Stored (EEPROM) state of a virtual encoder

typedef struct
{
    u8 State    : 1;
    u8 Mode     : 4;
    u8 Reserved : 3;

    sSwitchMidiConfig MidiConfig;
} sVirtualSwitch; // Runtime state of a virtual switch

typedef struct
{
    u8 Mode     : 4;
    u8 Reserved : 4;

    sSwitchMidiConfig MidiConfig;
} sEEVirtualSwitch; // Stored (EEPROM) state of a virtual switch

typedef struct
{
    u16 StartValue;
    u16 StopValue;

    sRotaryMidiConfig MidiConfig;
} sVirtualUber; // Runtime state of an virtual uber encoder

typedef sVirtualUber sEEVirtualUber; // Stored (EEPROM) state of an uber encoder.

typedef struct
{
    sVirtualEncoder Primary;
    sVirtualEncoder Secondary;
    sVirtualUber    PrimaryUber;
    sVirtualUber    SecondaryUber;
    sVirtualSwitch  Switch;
    bool            PrimaryEnabled;
} sEncoderState;

typedef struct
{
    s16 PreviousMovement;
    s16 CurrentVelocity;
    u8  Speed;
    u8  Sample;
    u32 Bits;
} sHardwareEncoder;

void Encoder_Init(void);
void Encoder_SetDefaultConfig(sEncoderState* pEncoder);
void Encoder_FactoryReset(void);
void Encoder_Update(void);