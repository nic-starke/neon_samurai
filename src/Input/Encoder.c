/*
 * File: Encoder.c ( 27th November 2021 )
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

#include <avr/pgmspace.h>
#include <math.h>

#include "Encoder.h"
#include "HardwareDescription.h"
#include "Data.h"
#include "Colour.h"
#include "Types.h"
#include "Input.h"

static const sVirtualEncoder DEFAULT_PRIMARY_VENCODER = {
    .CurrentValue            = ENCODER_MIN_VAL,
    .PreviousValue           = ENCODER_MIN_VAL,
    .DisplayStyle            = STYLE_BLENDED_BAR,
    .HasDetent               = false,
    .FineAdjust              = false,
    .MidiConfig.Channel      = 0,
    .MidiConfig.Mode         = MIDIMODE_CC,
    .MidiConfig.MidiValue.CC = 0,
    .RGBColour               = RGB_RED,
    .DetentColour            = RGB_FUSCHIA,
    .DisplayInvalid          = true,
};

static const sVirtualEncoder DEFAULT_SECONDARY_VENCODER = {
    .CurrentValue            = ENCODER_MID_VAL,
    .PreviousValue           = ENCODER_MID_VAL,
    .DisplayStyle            = STYLE_DOT,
    .HasDetent               = true,
    .FineAdjust              = true,
    .MidiConfig.Channel      = 1,
    .MidiConfig.Mode         = MIDIMODE_CC,
    .MidiConfig.MidiValue.CC = 0,
    .RGBColour               = RGB_BLUE,
    .DetentColour            = RGB_FUSCHIA,
    .DisplayInvalid          = true,
};

static const sVirtualUber DEFAULT_VUBER = {
    .StartValue              = ENCODER_MAX_VAL / 4,
    .StopValue               = (ENCODER_MAX_VAL / 4) * 3,
    .MidiConfig.Channel      = 0,
    .MidiConfig.MidiValue.CC = 0,
    .MidiConfig.Mode         = MIDIMODE_CC,
};

// clang-format off
static const sVirtualSwitch DEFAULT_ENCODER_VSWITCH = {
    .State               = 0,
    .Mode                = SWITCH_SECONDARY_TOGGLE,
    .MidiConfig.Channel  = 0,
    .MidiConfig.Mode     = MIDIMODE_DISABLED,
    .MidiConfig.OffValue = 0,
    .MidiConfig.OnValue  = 0
};
// clang-format on

static const sEncoderState DEFAULT_ENCODERSTATE = {
    .Primary        = DEFAULT_PRIMARY_VENCODER,
    .Secondary      = DEFAULT_SECONDARY_VENCODER,
    .PrimaryUber    = DEFAULT_VUBER,
    .SecondaryUber  = DEFAULT_VUBER,
    .Switch         = DEFAULT_ENCODER_VSWITCH,
    .PrimaryEnabled = true,
};

static s16 Acceleration(sHardwareEncoder* pHWEncoder, s8 Movement, bool FineAdjust);

void Encoder_Init(void)
{
}

void Encoder_SetDefaultConfig(sEncoderState* pEncoder)
{
    // Data_PGMReadBlock(pEncoder, &DEFAULT_ENCODERSTATE, sizeof(sEncoderState));
    *pEncoder = DEFAULT_ENCODERSTATE;
}

// Sets the encoder states in SRAM to a default configuration (does not do anything with EEPROM)
void Encoder_FactoryReset(void)
{
    for (int bank = 0; bank < NUM_VIRTUAL_BANKS; bank++)
    {
        for (int encoder = 0; encoder < NUM_ENCODERS; encoder++)
        {
            sEncoderState* pEncoder = &gData.EncoderStates[bank][encoder];
            Encoder_SetDefaultConfig(pEncoder);

            pEncoder->Primary.MidiConfig.MidiValue.CC = encoder;
            pEncoder->Primary.MidiConfig.Channel      = 0;

            pEncoder->PrimaryUber.MidiConfig.MidiValue.CC = encoder;
            pEncoder->PrimaryUber.MidiConfig.Channel      = 1;

            pEncoder->Secondary.MidiConfig.MidiValue.CC = encoder;
            pEncoder->Secondary.MidiConfig.Channel      = 2;

            pEncoder->SecondaryUber.MidiConfig.MidiValue.CC = encoder;
            pEncoder->SecondaryUber.MidiConfig.Channel      = 3;

            pEncoder->Primary.DisplayInvalid   = true;
            pEncoder->Secondary.DisplayInvalid = true;
            // pEncoder->Switch.MidiConfig.Channel = 0;
            // pEncoder->Switch.MidiConfig.OnValue = 1;
            // pEncoder->Switch.MidiConfig.OffValue = 0;
            // pEncoder->Switch.MidiConfig.Mode = MIDIMODE_CC;
        }
    }
}

void Encoder_Update(void)
{
    for (int encoder = 0; encoder < NUM_ENCODERS; encoder++)
    {
        // Encoder indexing is reversed due to hardware design
        sEncoderState* pEncoderState = &gData.EncoderStates[gData.CurrentBank][(NUM_ENCODERS - 1) - encoder];

        bool displayInvalid = false;
        bool transmitValue  = false;

        // Process Encoder Switch
        if (pEncoderState->Switch.Mode != SWITCH_DISABLED)
        {
            pEncoderState->Switch.State = (bool)EncoderSwitchCurrentState(SWITCH_MASK(encoder));

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
                    if (EncoderSwitchWasPressed(SWITCH_MASK(encoder)))
                    {
                        if (pEncoderState->PrimaryEnabled)
                        {
                            pEncoderState->Primary.CurrentValue = ENCODER_MIN_VAL;
                        }
                        else
                        {
                            pEncoderState->Secondary.CurrentValue = ENCODER_MIN_VAL;
                        }

                        displayInvalid = true;
                        transmitValue  = true;
                    }
                    break;

                case SWITCH_RESET_VALUE_ON_RELEASE:
                    if (EncoderSwitchWasReleased(SWITCH_MASK(encoder)))
                    {
                        if (pEncoderState->PrimaryEnabled)
                        {
                            pEncoderState->Primary.CurrentValue = ENCODER_MIN_VAL;
                        }
                        else
                        {
                            pEncoderState->Secondary.CurrentValue = ENCODER_MIN_VAL;
                        }

                        displayInvalid = true;
                        transmitValue  = true;
                    }
                    break;

                case SWITCH_FINE_ADJUST_HOLD:
                    if (pEncoderState->PrimaryEnabled)
                    {
                        pEncoderState->Primary.FineAdjust   = pEncoderState->Switch.State;
                        pEncoderState->Secondary.FineAdjust = false;
                    }
                    else
                    {
                        pEncoderState->Secondary.FineAdjust = pEncoderState->Switch.State;
                        pEncoderState->Primary.FineAdjust   = false;
                    }
                    break;

                case SWITCH_FINE_ADJUST_TOGGLE:
                    if (EncoderSwitchWasPressed(SWITCH_MASK(encoder)))
                    {
                        if (pEncoderState->PrimaryEnabled)
                        {
                            pEncoderState->Primary.FineAdjust = !pEncoderState->Primary.FineAdjust;
                        }
                        else
                        {
                            pEncoderState->Secondary.FineAdjust = !pEncoderState->Primary.FineAdjust;
                        }
                    }
                    break;

                case SWITCH_SECONDARY_HOLD:
                {
                    bool oldState                 = pEncoderState->PrimaryEnabled;
                    pEncoderState->PrimaryEnabled = !pEncoderState->Switch.State;
                    if (oldState != pEncoderState->PrimaryEnabled)
                    {
                        displayInvalid = true;
                    }

                    break;
                }

                case SWITCH_SECONDARY_TOGGLE:
                    if (EncoderSwitchWasPressed(SWITCH_MASK(encoder)))
                    {
                        pEncoderState->PrimaryEnabled = !pEncoderState->PrimaryEnabled;
                        displayInvalid                = true;
                    }
                    break;

                default:
                case SWITCH_DISABLED: break;
            }
        }

        sVirtualEncoder* pVE = NULL;

        if (pEncoderState->PrimaryEnabled)
        {
            pVE = &pEncoderState->Primary;
        }
        else
        {
            pVE = &pEncoderState->Secondary;
        }

        if (pVE->MidiConfig.Mode == MIDIMODE_DISABLED)
        {
            continue; // skip to next encoder
        }

        // Process Encoder Rotation
        sHardwareEncoder* pHardwareEncoder = &gData.HardwareEncoders[encoder];
        pVE->PreviousValue                 = pVE->CurrentValue;
        u8  dir                            = Encoder_GetDirection(encoder);
        s16 move                           = 0;
        if (dir == DIR_STATIONARY)
        {
            move = 0;
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
#define ACC_ENABLED 0
#if (ACC_ENABLED == 1)
            pHardwareEncoder->CurrentVelocity = move * Acceleration(pHardwareEncoder, move, pVE->FineAdjust);
#else
            pHardwareEncoder->CurrentVelocity = move * 1000;
#endif
            s32 newValue = (s32)pVE->CurrentValue + pHardwareEncoder->CurrentVelocity;

            if (newValue >= ENCODER_MAX_VAL)
            {
                pVE->CurrentValue = ENCODER_MAX_VAL;
            }
            else if (newValue < ENCODER_MIN_VAL)
            {
                pVE->CurrentValue = ENCODER_MIN_VAL;
            }
            else
            {
                pVE->CurrentValue += pHardwareEncoder->CurrentVelocity;
            }

            if (pVE->CurrentValue != pVE->PreviousValue)
            {
                transmitValue  = true;
                displayInvalid = true;
            }
        }

        pVE->DisplayInvalid = displayInvalid;

        if (transmitValue)
        {
            //USBMidi_ProcessKnob(pVE);
        }
    }
}