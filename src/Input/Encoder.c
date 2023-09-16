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

#include "Encoder.h"
#include "HardwareDescription.h"
#include "Data.h"
#include "Colour.h"
#include "Types.h"

static const sVirtualEncoder DEFAULT_PRIMARY_VENCODER PROGMEM = {
    .CurrentValue            = ENCODER_MIN_VAL,
    .PreviousValue           = ENCODER_MIN_VAL,
    .DisplayStyle            = STYLE_DOT,
    .HasDetent               = false,
    .FineAdjust              = false,
    .MidiConfig.Channel      = 0,
    .MidiConfig.Mode         = MIDIMODE_CC,
    .MidiConfig.MidiValue.CC = 0,
    .RGBColour               = RGB_RED,
    .DetentColour            = RGB_FUSCHIA,
    .DisplayInvalid          = true,
};

static const sVirtualEncoder DEFAULT_SECONDARY_VENCODER PROGMEM = {
    .CurrentValue            = ENCODER_MID_VAL,
    .PreviousValue           = ENCODER_MID_VAL,
    .DisplayStyle            = STYLE_BLENDED_BAR,
    .HasDetent               = true,
    .FineAdjust              = true,
    .MidiConfig.Channel      = 1,
    .MidiConfig.Mode         = MIDIMODE_CC,
    .MidiConfig.MidiValue.CC = 0,
    .RGBColour               = RGB_BLUE,
    .DetentColour            = RGB_FUSCHIA,
    .DisplayInvalid          = true,
};

static const sVirtualUber DEFAULT_VUBER PROGMEM = {
    .StartValue              = ENCODER_MAX_VAL / 4,
    .StopValue               = (ENCODER_MAX_VAL / 4) * 3,
    .MidiConfig.Channel      = 0,
    .MidiConfig.MidiValue.CC = 0,
    .MidiConfig.Mode         = MIDIMODE_CC,
};

// clang-format off
static const sVirtualSwitch DEFAULT_ENCODER_VSWITCH PROGMEM = {
    .State               = 0,
    .Mode                = SWITCH_SECONDARY_TOGGLE,
    .MidiConfig.Channel  = 0,
    .MidiConfig.Mode     = MIDIMODE_DISABLED,
    .MidiConfig.OffValue = 0,
    .MidiConfig.OnValue  = 0
};
// clang-format on

static const sEncoderState DEFAULT_ENCODERSTATE PROGMEM = {
    .Primary       = DEFAULT_PRIMARY_VENCODER,
    .Secondary     = DEFAULT_SECONDARY_VENCODER,
    .PrimaryUber   = DEFAULT_VUBER,
    .SecondaryUber = DEFAULT_VUBER,
    .Switch        = DEFAULT_ENCODER_VSWITCH,
};

void Encoder_Init(void)
{
}

void Encoder_SetDefaultConfig(sEncoderState* pEncoder)
{
    Data_PGMReadBlock(pEncoder, &DEFAULT_ENCODERSTATE, sizeof(sEncoderState));
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
        }
    }
}