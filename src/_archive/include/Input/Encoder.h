/*
 * File: Encoder.h ( 27th November 2021 )
 * Project: Muffin
 * Copyright 2021 Nicolaus Starke
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

#include "system/types.h"

#include "Display/Colour.h"

#define ENCODER_MIN_VAL (0)
#define ENCODER_MAX_VAL (UINT16_MAX)
#define ENCODER_MID_VAL (0x7FFF)

#define ACCEL_CONST       (225)
#define FINE_ADJUST_CONST (50)

typedef enum {
  VIRTUAL_BANK_A,
  VIRTUAL_BANK_B,
  VIRTUAL_BANK_C,

  NUM_VIRTUAL_BANKS,
} eVirtualBank;

typedef enum {
  VIRTUAL_LAYER_A,
  VIRTUAL_LAYER_B,

  NUM_VIRTUAL_ENCODER_LAYERS,
} eVirtualEncoderLayers;

typedef enum {
  STYLE_DOT,
  STYLE_BAR,
  STYLE_BLENDED_BAR,

  NUM_DISPLAY_STYLES,
} eEncoderDisplayStyle;

typedef enum {
  LAYERMODE_AB_
} eLayerMode;

// Mode is stored in 4 bits, max num modes is therefore 16
typedef enum {
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
typedef enum {
  SWITCH_DISABLED,
  SWITCH_MIDI,
  SWITCH_RESET_VALUE_ON_PRESS, // Sets the encoder value to
                               // switch->ModeParameter (layer values will also
                               // be set accordingly)
  SWITCH_RESET_VALUE_ON_RELEASE,
  SWITCH_FINE_ADJUST_HOLD,
  SWITCH_FINE_ADJUST_TOGGLE,
  SWITCH_LAYER_TOGGLE, // Enable/disable a specified layer
  SWITCH_LAYER_HOLD,   // Enable a specified layer while switch is active
  SWITCH_LAYER_CYCLE, // Cycle between two layers (enables one, and disables the
                      // other)

  NUM_SWITCH_MODES,
} eSwitchMode;

typedef union {
  struct {
    uint16_t CurrentLayer : 8;
    uint16_t NextLayer    : 8;
  } LayerTransition;
  uint16_t Value;
} uSwitchModeParameter;

typedef union {
  uint8_t CC;
  uint8_t Note;
} uMidiValue;

typedef struct {
  uint8_t Mode    : 4;
  uint8_t Channel : 4;

  uMidiValue MidiValue;
} sRotaryMidiConfig;

typedef struct {
  union {
    struct {
      uint8_t Mode    : 4;
      uint8_t Channel : 4;
    } BitField;
    uint8_t Byte;
  } MidiData;
  uMidiValue MidiValue;
} sEE_RotaryMidiConfig;

typedef struct {
  uint8_t Mode    : 4;
  uint8_t Channel : 4;

  uMidiValue OnValue;
  uMidiValue OffValue;
} sSwitchMidiConfig;

typedef struct {
  uint16_t          StartPosition;
  uint16_t          StopPosition;
  uint16_t          MinValue;
  uint16_t          MaxValue;
  sRotaryMidiConfig MidiConfig;
  uint16_t          RGBHue;
  bool Enabled; // Runtime enabled state - can be modified by encoder/side
                // switch.
} sVirtualEncoderLayer; // Runtime state of an encoder layer

typedef struct {
  uint16_t             StartPosition;
  uint16_t             StopPosition;
  uint16_t             MinValue;
  uint16_t             MaxValue;
  sEE_RotaryMidiConfig MidiConfig;
  uint16_t             RGBHue;
} sEE_VirtualEncoderLayer;

typedef struct {
  uint8_t State    : 1;
  uint8_t Mode     : 4;
  uint8_t Reserved : 3;

  uSwitchModeParameter ModeParameter;
  sSwitchMidiConfig    MidiConfig;
} sVirtualSwitch; // Runtime state of a virtual switch

typedef struct {
  uint16_t CurrentValue;
  uint16_t PreviousValue;

  sRGB DetentColour;
  sRGB RGBColour;

  uint8_t DisplayStyle   : 2;
  uint8_t DisplayInvalid : 1;
  uint8_t FineAdjust     : 1;
  uint8_t HasDetent      : 1;
  uint8_t LayerA_Enabled : 1;
  uint8_t LayerB_Enabled : 1;
  uint8_t Reserved       : 1;

  uint16_t DetentHue;

  sVirtualEncoderLayer Layers[NUM_VIRTUAL_ENCODER_LAYERS];
  sVirtualSwitch       Switch;
} sEncoderState;

typedef struct {
  union {
    struct {
      uint8_t DisplayStyle   : 2;
      uint8_t FineAdjust     : 1;
      uint8_t HasDetent      : 1;
      uint8_t LayerA_Enabled : 1;
      uint8_t LayerB_Enabled : 1;
      uint8_t Reserved       : 2;
    } BitField;
    uint8_t Byte;
  } BitFieldSettings;

  uint16_t DetentHue;

  sEE_VirtualEncoderLayer Layers[NUM_VIRTUAL_ENCODER_LAYERS];

} sEE_Encoder;

typedef struct {
  int16_t CurrentVelocity;
} sHardwareEncoder;

void Encoder_Init(void);
void Encoder_SetDefaultConfig(sEncoderState* pEncoder);
void Encoders_ResetToDefaultConfig(void);
void Encoder_Update(void);