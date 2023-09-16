/*
 * File: Encoder.c ( 27th November 2021 )
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

#include <avr/pgmspace.h>
#include <math.h>

#include "Colour.h"
#include "Data.h"
#include "DataTypes.h"
#include "Encoder.h"
#include "EncoderDisplay.h"
#include "HardwareDescription.h"
#include "Input.h"
#include "MIDI.h"
#include "USB.h"
#include "Utility.h"

// A default configuration for LAYER A of an encoder.
static const sVirtualEncoderLayer DEFAULT_LAYER_A = {     // TODO move to progmem.
    .StartPosition           = ENCODER_MIN_VAL,
    .StopPosition            = ENCODER_MID_VAL - 1,
    .MinValue                = ENCODER_MIN_VAL,
    .MaxValue                = ENCODER_MAX_VAL,
    .MidiConfig.Channel      = 0,
    .MidiConfig.Mode         = MIDIMODE_CC,
    .MidiConfig.MidiValue.CC = 0,
    .RGBHue                  = HUE_RED,
    .Enabled                 = true,
};

// A default configuration for LAYER B of an encoder.
static const sVirtualEncoderLayer DEFAULT_LAYER_B = {     // TODO move to progmem.
    .StartPosition           = ENCODER_MID_VAL,
    .StopPosition            = ENCODER_MAX_VAL,
    .MinValue                = ENCODER_MAX_VAL,
    .MaxValue                = ENCODER_MIN_VAL,
    .MidiConfig.Channel      = 1,
    .MidiConfig.Mode         = MIDIMODE_CC,
    .MidiConfig.MidiValue.CC = 1,
    .RGBHue                  = HUE_GREEN,
    .Enabled                 = true,
};

// clang-format off
// A default configuration for an encoder switch
static const sVirtualSwitch DEFAULT_ENCODER_SWITCH = { // TODO move to progmem.
    .State               = 0,
    .Mode                = SWITCH_LAYER_CYCLE,
    .MidiConfig.Channel  = 0,
    .MidiConfig.Mode     = MIDIMODE_DISABLED,
    .MidiConfig.OffValue = 0,
    .MidiConfig.OnValue  = 0,
    .ModeParameter.LayerTransition.CurrentLayer = VIRTUAL_LAYER_A,
    .ModeParameter.LayerTransition.NextLayer    = VIRTUAL_LAYER_B,
};
// clang-format on

// A default configuration for an encoder.
static const sEncoderState DEFAULT_ENCODERSTATE = {
    // TODO move to progmem.
    .CurrentValue            = 0,
    .PreviousValue           = 0,
    .DetentColour            = {0},
    .RGBColour               = {0},
    .DetentHue               = HSV_BLUE,
    .DisplayInvalid          = true,
    .DisplayStyle            = STYLE_BLENDED_BAR,
    .FineAdjust              = false,
    .HasDetent               = false,
    .Layers[VIRTUAL_LAYER_A] = DEFAULT_LAYER_A,
    .Layers[VIRTUAL_LAYER_B] = DEFAULT_LAYER_B,
    .LayerA_Enabled          = true,
    .LayerB_Enabled          = true,
    .Switch                  = DEFAULT_ENCODER_SWITCH,
};

/**
 * @brief Check if an encoder layer is currently active.
 * Will return false if midi is disabled for this layer.
 * 
 * @param pLayer A pointer to the virtual layer.
 * @return True if active, false if not active.
 */
static inline bool IsLayerActive(sVirtualEncoderLayer* pLayer)
{
    return (pLayer->Enabled && (pLayer->MidiConfig.Mode != MIDIMODE_DISABLED));
}

/**
 * @brief Check if any encoder layer is active.
 * 
 * @param pEncoderState A pointer to the encoder state.
 * @return True if any layer is active, false otherwise.
 */
static inline bool IsAnyLayerActive(sEncoderState* pEncoderState)
{
    for (int layer = 0; layer < NUM_VIRTUAL_ENCODER_LAYERS; layer++)
    {
        if (IsLayerActive(&pEncoderState->Layers[layer]))
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Initialise the encoder module.
 * Resets all encoders to a default state.
 * WARNING - Must be called before Data_Init() to avoid overwriting user data.
 */
void Encoder_Init(void)
{
    Encoders_ResetToDefaultConfig();
}

/**
 * @brief Sets an encoder state to the default encoder configuration.
 * 
 * @param pEncoder A pointer to the encoder to reset to default configuration.
 */
void Encoder_SetDefaultConfig(sEncoderState* pEncoder)
{
    *pEncoder = DEFAULT_ENCODERSTATE;
}

/**
 * @brief Sets all encoder states in RAM to the default configuration.
 * 
 */
void Encoders_ResetToDefaultConfig(void)
{
    for (int bank = 0; bank < NUM_VIRTUAL_BANKS; bank++)
    {
        for (int encoder = 0; encoder < NUM_ENCODERS; encoder++)
        {
            sEncoderState* pEncoder = &gData.EncoderStates[bank][encoder];
            Encoder_SetDefaultConfig(pEncoder);
        }
    }
}

/**
 * @brief Checks if a hardware encoder has been rotated - updates an encoder state based on this.
 * 
 * @param EncoderIndex The hardware encoder index.
 * @param pHardwareEncoder A pointer to the hardware encoder state.
 * @param pEncoderState A pointer to the encoder state (from one of the virtual banks)
 * @return True if a hardware encoder has been rotated - false otherwise.
 */
static inline bool UpdateEncoderRotary(int EncoderIndex, sHardwareEncoder* pHardwareEncoder, sEncoderState* pEncoderState)
{
    pEncoderState->PreviousValue = pEncoderState->CurrentValue;

    u8  dir  = Encoder_GetDirection(EncoderIndex);
    s16 move = 0;

    if (dir == DIR_STATIONARY)
    {
        return false; // nothing to do, return
    }
    else
    {
        if (dir == DIR_CW)
        {
            move = 1;
        }
        else if (dir == DIR_CCW)
        {
            move = -1;
        }

// TODO - Encoder acceleration algorithm
#define ACC_ENABLED 0
#if (ACC_ENABLED == 1)
        pHardwareEncoder->CurrentVelocity = move * Acceleration(pHardwareEncoder, move, pVE->FineAdjust);
#else
        pHardwareEncoder->CurrentVelocity = move * 1000;
#endif
        s32 newValue = (s32)pEncoderState->CurrentValue + pHardwareEncoder->CurrentVelocity;

        if (newValue >= ENCODER_MAX_VAL)
        {
            pEncoderState->CurrentValue = ENCODER_MAX_VAL;
        }
        else if (newValue < ENCODER_MIN_VAL)
        {
            pEncoderState->CurrentValue = ENCODER_MIN_VAL;
        }
        else
        {
            pEncoderState->CurrentValue += pHardwareEncoder->CurrentVelocity;
        }

        if (pEncoderState->CurrentValue != pEncoderState->PreviousValue)
        {
            pEncoderState->DisplayInvalid = true;
            return true;
        }
    }

    return false;
}

/**
 * @brief Updates the switch state for an encoder based on the current switch hardware state
 * 
 * @param EncoderIndex The hardware encoder index to check.
 * @param pEncoderState A pointer to the encoder state to be updated.
 */
static inline void UpdateEncoderSwitch(int EncoderIndex, sEncoderState* pEncoderState)
{
    // Process Encoder Switch
    if (pEncoderState->Switch.Mode != SWITCH_DISABLED)
    {
        pEncoderState->Switch.State = (bool)EncoderSwitchCurrentState(SWITCH_MASK(EncoderIndex));

        switch (pEncoderState->Switch.Mode)
        {
            case SWITCH_MIDI:
            {
                switch (pEncoderState->Switch.MidiConfig.Mode)
                {
                    case MIDIMODE_DISABLED: break;

                    case MIDIMODE_SW_CC_TOGGLE:
                    case MIDIMODE_SW_NOTE_HOLD:
                    case MIDIMODE_SW_NOTE_TOGGLE:
                        // USBMidi_ProcessEncoderSwitch(&pEncoderState->Switch);
                        break;
                }
                break;
            }

            case SWITCH_RESET_VALUE_ON_PRESS:
            {
                if (EncoderSwitchWasPressed(SWITCH_MASK(EncoderIndex)))
                {
                    pEncoderState->CurrentValue = pEncoderState->Switch.ModeParameter.Value;
                }
                break;
            }

            case SWITCH_RESET_VALUE_ON_RELEASE:
            {
                if (EncoderSwitchWasReleased(SWITCH_MASK(EncoderIndex)))
                {
                    pEncoderState->CurrentValue = pEncoderState->Switch.ModeParameter.Value;
                }
                break;
            }

            case SWITCH_FINE_ADJUST_HOLD:
            {
                pEncoderState->FineAdjust = pEncoderState->Switch.State;
                break;
            }

            case SWITCH_FINE_ADJUST_TOGGLE:
            {
                if (EncoderSwitchWasPressed(SWITCH_MASK(EncoderIndex)))
                {
                    pEncoderState->FineAdjust = !pEncoderState->FineAdjust;
                }
                break;
            }

            case SWITCH_LAYER_TOGGLE:
            {
                if (EncoderSwitchWasPressed(SWITCH_MASK(EncoderIndex)))
                {
                    pEncoderState->Layers[pEncoderState->Switch.ModeParameter.Value].Enabled =
                        !pEncoderState->Layers[pEncoderState->Switch.ModeParameter.Value].Enabled;
                }
                break;
            }
            case SWITCH_LAYER_HOLD:
            {
                pEncoderState->Layers[pEncoderState->Switch.ModeParameter.Value].Enabled = pEncoderState->Switch.State;
                break;
            }
            case SWITCH_LAYER_CYCLE:
            {
                if (EncoderSwitchWasPressed(SWITCH_MASK(EncoderIndex)))
                {
                    u8 currentLayer                             = pEncoderState->Switch.ModeParameter.LayerTransition.CurrentLayer;
                    pEncoderState->Layers[currentLayer].Enabled = false;
                    pEncoderState->Layers[pEncoderState->Switch.ModeParameter.LayerTransition.NextLayer].Enabled = true;
                    pEncoderState->Switch.ModeParameter.LayerTransition.CurrentLayer =
                        pEncoderState->Switch.ModeParameter.LayerTransition.NextLayer;
                    pEncoderState->Switch.ModeParameter.LayerTransition.NextLayer = currentLayer;
                }
                break;
            }
                // case SWITCH_LAYER_CYCLE_NEXT:
                // {
                //     if (EncoderSwitchWasPressed(SWITCH_MASK(EncoderIndex)))
                //     {
                //         u8 nextLayer = (pEncoderState->Switch.ModeParameter.LayerTransition.CurrentLayer + 1) % NUM_VIRTUAL_ENCODER_LAYERS;
                //         pEncoderState->Layers[pEncoderState->Switch.ModeParameter.LayerTransition.CurrentLayer].Enabled = false;
                //         pEncoderState->Layers[nextLayer].Enabled                                                        = true;
                //         pEncoderState->Switch.ModeParameter.LayerTransition.CurrentLayer                                = nextLayer;
                //         pEncoderState->Switch.ModeParameter.LayerTransition.NextLayer = (nextLayer + 1) % NUM_VIRTUAL_ENCODER_LAYERS;
                //     }
                //     break;
                // }

            default:
            case SWITCH_DISABLED: break;
        }
    }
}

/**
 * @brief Update state for all virtual layers for a particular encoder.
 * 
 * @param pEncoderState The encoder state to update.
 */
static inline void ProcessEncoderLayers(sEncoderState* pEncoderState)
{
    for (int layer = 0; layer < NUM_VIRTUAL_ENCODER_LAYERS; layer++)
    {
        sVirtualEncoderLayer* pLayer = &pEncoderState->Layers[layer];

        if (IsLayerActive(pLayer))
        {
            if (IN_RANGE(pEncoderState->CurrentValue, pLayer->StartPosition, pLayer->StopPosition))
            {
                // Conversion from encoder value to virtual layer value.
                float percent =
                    (float)(pEncoderState->CurrentValue - pLayer->StartPosition) / (pLayer->StopPosition - pLayer->StartPosition);

                u16 virtualValue = 0;

                // Check if min/max are reversed (i.e the range is counting down)
                if (pLayer->MinValue < pLayer->MaxValue)
                {
                    virtualValue = (u16)(percent * pLayer->MaxValue);
                }
                else
                {
                    virtualValue = (u16)((1.0f - percent) * pLayer->MinValue);
                }

                MIDI_ProcessLayer(pEncoderState, pLayer,
                                  virtualValue >> 9); // TODO - 7-bit lazy conversion is bad, what about NRPN/14-bit?
            }
        }
    }
}

/**
 * @brief The encoder update function - to be called in the main poll.
 * 
 */
void Encoder_Update(void)
{
    for (int encoder = 0; encoder < NUM_ENCODERS; encoder++)
    {
        // Encoder indexing is reversed due to hardware design
        sEncoderState* pEncoderState = &gData.EncoderStates[gData.CurrentBank][(NUM_ENCODERS - 1) - encoder];

        UpdateEncoderSwitch(encoder, pEncoderState);

        // Check if one of layers is active (otherwise skip processing)
        // Check if the encoder has rotated at all (otherwise skip processing)
        // Process/transmit midi for each active layer.
        if (IsAnyLayerActive(pEncoderState))
        {
            if (UpdateEncoderRotary(encoder, &gData.HardwareEncoders[encoder], pEncoderState))
            {
                ProcessEncoderLayers(pEncoderState);
            }
        }
    }
}