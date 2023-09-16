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

#include "Colour.h"

#define ENCODER_MIN_VAL (0)
#define ENCODER_MAX_VAL (UINT16_MAX)
#define ENCODER_MID_VAL (0x7FFF)

#define ACCEL_CONST       (225)
#define FINE_ADJUST_CONST (50)

typedef enum
{
    VIRTUAL_BANK_A,
    VIRTUAL_BANK_B,
    VIRTUAL_BANK_C,

    NUM_VIRTUAL_BANKS,
} eVirtualBank;

typedef enum
{
    VIRTUAL_LAYER_A,
    VIRTUAL_LAYER_B,

    NUM_VIRTUAL_ENCODER_LAYERS,
} eVirtualEncoderLayers;

typedef enum
{
    STYLE_DOT,
    STYLE_BAR,
    STYLE_BLENDED_BAR,

    NUM_DISPLAY_STYLES,
} eEncoderDisplayStyle;

typedef enum
{
    LAYERMODE_AB_
} eLayerMode;

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
// Most of these are activated on switch press (not release)
typedef enum
{
    SWITCH_DISABLED,
    SWITCH_MIDI,
    SWITCH_RESET_VALUE_ON_PRESS, // Sets the encoder value to switch->ModeParameter (layer values will also be set accordingly)
    SWITCH_RESET_VALUE_ON_RELEASE,
    SWITCH_FINE_ADJUST_HOLD,
    SWITCH_FINE_ADJUST_TOGGLE,
    SWITCH_LAYER_TOGGLE, // Enable/disable a specified layer
    SWITCH_LAYER_HOLD,   // Enable a specified layer while switch is active
    SWITCH_LAYER_CYCLE,  // Cycle between two layers (enables one, and disables the other)

    NUM_SWITCH_MODES,
} eSwitchMode;

typedef union
{
    struct
    {
        u16 CurrentLayer : 8;
        u16 NextLayer    : 8;
    } LayerTransition;
    u16 Value;
} uSwitchModeParameter;

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
    union
    {
        struct
        {
            u8 Mode    : 4;
            u8 Channel : 4;
        } BitField;
        u8 Byte;
    } MidiData;
    uMidiValue MidiValue;
} sEE_RotaryMidiConfig;

typedef struct
{
    u8 Mode    : 4;
    u8 Channel : 4;

    uMidiValue OnValue;
    uMidiValue OffValue;
} sSwitchMidiConfig;

typedef struct
{
    u16               StartPosition;
    u16               StopPosition;
    u16               MinValue;
    u16               MaxValue;
    sRotaryMidiConfig MidiConfig;
    u16               RGBHue;
    bool              Enabled; // Runtime enabled state - can be modified by encoder/side switch.
} sVirtualEncoderLayer;        // Runtime state of an encoder layer

typedef struct
{
    u16                  StartPosition;
    u16                  StopPosition;
    u16                  MinValue;
    u16                  MaxValue;
    sEE_RotaryMidiConfig MidiConfig;
    u16                  RGBHue;
} sEE_VirtualEncoderLayer;

typedef struct
{
    u8 State    : 1;
    u8 Mode     : 4;
    u8 Reserved : 3;

    uSwitchModeParameter ModeParameter;
    sSwitchMidiConfig    MidiConfig;
} sVirtualSwitch; // Runtime state of a virtual switch

typedef struct
{
    u16 CurrentValue;
    u16 PreviousValue;

    sRGB DetentColour;
    sRGB RGBColour;

    u8 DisplayStyle   : 2;
    u8 DisplayInvalid : 1;
    u8 FineAdjust     : 1;
    u8 HasDetent      : 1;
    u8 LayerA_Enabled : 1;
    u8 LayerB_Enabled : 1;
    u8 Reserved       : 1;

    sVirtualEncoderLayer Layers[NUM_VIRTUAL_ENCODER_LAYERS];
    sVirtualSwitch       Switch;
} sEncoderState;

typedef struct
{
    union
    {
        struct
        {
            u8 DisplayStyle   : 2;
            u8 FineAdjust     : 1;
            u8 HasDetent      : 1;
            u8 LayerA_Enabled : 1;
            u8 LayerB_Enabled : 1;
            u8 Reserved       : 2;
        } BitField;
        u8 Byte;
    } BitFieldSettings;

    u16 DetentHue;

    sEE_VirtualEncoderLayer Layers[NUM_VIRTUAL_ENCODER_LAYERS];

} sEE_Encoder;

typedef struct
{
    s16 CurrentVelocity;
} sHardwareEncoder;

void Encoder_Init(void);
void Encoder_SetDefaultConfig(sEncoderState* pEncoder);
void Encoder_FactoryReset(void);
void Encoder_Update(void);