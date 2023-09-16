/*
 * File: Data.c ( 28th November 2021 )
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

#include <avr/eeprom.h>

#include "DataTypes.h"
#include "Config.h"
#include "Data.h"
#include "Display.h"
#include "Encoder.h"
#include "Colour.h"
#include "Input.h"
#include "Interrupt.h"
#include "EncoderDisplay.h"

#define EE_DATA_VERSION (0xAF03)

sData gData = {
    .DataVersion          = 0,
    .FirmwareVersion      = VERSION,
    .FactoryReset         = false,
    .OperatingMode        = DEFAULT_MODE,
    .RGBBrightness        = (u8)BRIGHTNESS_MAX,
    .DetentBrightness     = BRIGHTNESS_MAX,
    .IndicatorBrightness  = BRIGHTNESS_MAX,
    .FineAdjustConstant   = FINE_ADJUST_CONST,
    .AccelerationConstant = ACCEL_CONST,
    .CurrentBank          = 0,
};

typedef struct
{
    u8  Reserved; // Reserved byte ensures that original MFT firmware will reset eeprom on first bootup.
    u16 DataVersion;

    u8 OperatingMode;

    u8 RGBBrightness;
    u8 DetentBrightness;
    u8 IndicatorBrightness;

    sEE_Encoder Encoders[NUM_VIRTUAL_BANKS][NUM_ENCODERS];
} sEEData; // Data stored in EEPROM

// "_EEPROM_DATA_" is stored in eeprom - access only via eeprom read/write functions - THIS IS NOT IN RAM
// so dont try (_EEPROM_DATA_.SOMETHING = xxx)
// The initialised values will be present within the .eep file - which has to manually written to
// cpu via AVRdude, standard users cannot do this, therefore the default values can also be written
// by calling the Data_WriteDefaultsToEEPROM function in this file.
sEEData _EEPROM_DATA_ EEMEM; // = {
//     .Reserved            = 0xFF, // do not change this value!
//     .DataVersion         = EE_DATA_VERSION,
//     .OperatingMode       = DEFAULT_MODE,
//     .RGBBrightness       = BRIGHTNESS_MAX,
//     .DetentBrightness    = BRIGHTNESS_MAX,
//     .IndicatorBrightness = BRIGHTNESS_MAX,
//     .Encoders            = {0},
// };

// static inline void EE_ReadVirtualEncoder(sVirtualEncoder* pDest, sEEVirtualEncoder* pSrc)
// {
//     sEEVirtualEncoder shadow = {0};
//     eeprom_read_block(&shadow, pSrc, sizeof(sEEVirtualEncoder));

//     pDest->FineAdjust   = shadow.FineAdjust;
//     pDest->DisplayStyle = shadow.DisplayStyle;
//     pDest->HasDetent    = shadow.HasDetent;
//     pDest->MidiConfig   = shadow.MidiConfig;

//     Hue2RGB(shadow.DetentHue, &pDest->DetentColour);
//     Hue2RGB(shadow.RGBHue, &pDest->RGBColour);

//     pDest->DisplayInvalid = true;
// }

// static inline void EE_WriteVirtualEncoder(sVirtualEncoder* pSrc, sEEVirtualEncoder* pDest)
// {
//     sEEVirtualEncoder data = {0};

//     data.FineAdjust   = pSrc->FineAdjust;
//     data.DisplayStyle = pSrc->DisplayStyle;
//     data.HasDetent    = pSrc->HasDetent;
//     data.MidiConfig   = pSrc->MidiConfig;

//     eeprom_update_block(&data, pDest, sizeof(sEEVirtualEncoder));
// }

// /**
//  * When a user needs to modify the colours there is no conversion from RGB into HSV colour space.
//  * Instead, the HSV value is stored directly into EEPROM, and then this is converted during
//  * runtime. Animations require this conversion process as well.
//  */
// static inline void EE_WriteEncoderColours(u16 RGBHue, u16 DetentHue, sEEVirtualEncoder* pDest)
// {
//     eeprom_update_word(pDest->RGBHue, RGBHue);
//     eeprom_update_word(pDest->DetentHue, DetentHue);
// }

// static inline void EE_ReadVirtualUber(sVirtualUber* pDest, sEEVirtualUber* pSrc)
// {
//     sEEVirtualUber shadow = {0};
//     eeprom_read_block(&shadow, pSrc, sizeof(sEEVirtualUber));

//     *pDest = shadow;
// }

// static inline void EE_WriteVirtualUber(sVirtualUber* pSrc, sEEVirtualUber* pDest)
// {
//     eeprom_update_block(pSrc, pDest, sizeof(sEEVirtualUber));
// }

// static inline void EE_ReadVirtualSwitch(sVirtualSwitch* pDest, sEEVirtualSwitch* pSrc)
// {
//     sEEVirtualSwitch shadow = {0};
//     eeprom_read_block(&shadow, pSrc, sizeof(sEEVirtualSwitch));

//     pDest->Mode       = shadow.Mode;
//     pDest->MidiConfig = shadow.MidiConfig;
// }

// static inline void EE_WriteVirtualSwitch(sVirtualSwitch* pSrc, sEEVirtualSwitch* pDest)
// {
//     sEEVirtualSwitch data = {0};

//     data.Mode       = pSrc->Mode;
//     data.MidiConfig = pSrc->MidiConfig;

//     eeprom_update_block(&data, pDest, sizeof(sEEVirtualSwitch));
// }

void Data_Init(void)
{
    while (!eeprom_is_ready()) {} // wait for eeprom ready

    const vu8 flags = IRQ_DisableInterrupts();
    // Read the version stored in eeprom, if this doesnt match then factory reset the unit.
    gData.DataVersion  = eeprom_read_word(&_EEPROM_DATA_.DataVersion);
    gData.FactoryReset = (gData.DataVersion != EE_DATA_VERSION) || Input_IsResetPressed();

    u8 displayFlashCount = 0;

    if (gData.FactoryReset)
    {
        Data_WriteDefaultsToEEPROM();
        Data_RecallEEPROMSettings();
        displayFlashCount = 5;
        gData.FactoryReset = false;
    }
    else
    {
        Data_RecallEEPROMSettings();
        displayFlashCount = 2;        
    }

    IRQ_EnableInterrupts(flags);
    Display_Flash(200, displayFlashCount);
}

// Disable interrupts before calling
void Data_WriteDefaultsToEEPROM(void)
{
    eeprom_update_word(&_EEPROM_DATA_.DataVersion, (u16)EE_DATA_VERSION);

    // At this point in execution gData should not have been modified and should be initialised
    // with the default values (see top of this file)
    eeprom_update_byte(&_EEPROM_DATA_.RGBBrightness, (u8)gData.RGBBrightness);
    eeprom_update_byte(&_EEPROM_DATA_.DetentBrightness, (u8)gData.DetentBrightness);
    eeprom_update_byte(&_EEPROM_DATA_.IndicatorBrightness, (u8)gData.IndicatorBrightness);
    eeprom_update_byte(&_EEPROM_DATA_.OperatingMode, (u8)gData.OperatingMode);

    // Encoder_FactoryReset(); // firstly reset the encoder states in SRAM, then write this to EEPROM
    // for (int bank = 0; bank < NUM_VIRTUAL_BANKS; bank++)
    // {
    //     for (int encoder = 0; encoder < NUM_ENCODERS; encoder++)
    //     {
    //         sEncoderState* pEncoder = &gData.EncoderStates[bank][encoder];

    //         EE_WriteVirtualEncoder(&pEncoder->Primary, &_EEPROM_DATA_.VirtualEncoders[bank][encoder][VIRTUALENCODER_PRIMARY]);
    //         EE_WriteVirtualEncoder(&pEncoder->Secondary, &_EEPROM_DATA_.VirtualEncoders[bank][encoder][VIRTUALENCODER_SECONDARY]);
    //         EE_WriteVirtualUber(&pEncoder->PrimaryUber, &_EEPROM_DATA_.VirtualUbers[bank][encoder][VIRTUALUBER_PRIMARY]);
    //         EE_WriteVirtualUber(&pEncoder->SecondaryUber, &_EEPROM_DATA_.VirtualUbers[bank][encoder][VIRTUALUBER_SECONDARY]);
    //         EE_WriteVirtualSwitch(&pEncoder->Switch, &_EEPROM_DATA_.VirtualSwitches[bank][encoder]);
    //     }
    // }

    gData.DataVersion = EE_DATA_VERSION;
}

static inline void DisplayDataOnLEDS(void)
{
    EncoderDisplay_SetValueU8(0, eeprom_read_byte((const u8*)&_EEPROM_DATA_.RGBBrightness));
    EncoderDisplay_SetValueU8(1, gData.RGBBrightness);
    EncoderDisplay_SetValueU8(4, eeprom_read_byte((const u8*)&_EEPROM_DATA_.DetentBrightness));
    EncoderDisplay_SetValueU8(5, gData.DetentBrightness);
    EncoderDisplay_SetValueU8(9, gData.IndicatorBrightness);
    EncoderDisplay_SetValueU8(8, eeprom_read_byte((const u8*)&_EEPROM_DATA_.IndicatorBrightness));
    EncoderDisplay_SetValueU8(12, eeprom_read_byte((const u8*)&_EEPROM_DATA_.OperatingMode));
    EncoderDisplay_SetValueU8(13, gData.OperatingMode);
}

// Disable interrupts before calling
void Data_RecallEEPROMSettings(void)
{
    Display_SetRGBBrightness(eeprom_read_byte((const u8*)&_EEPROM_DATA_.RGBBrightness));
    Display_SetDetentBrightness(eeprom_read_byte((const u8*)&_EEPROM_DATA_.DetentBrightness));
    Display_SetIndicatorBrightness(eeprom_read_byte((const u8*)&_EEPROM_DATA_.IndicatorBrightness));
    gData.OperatingMode = eeprom_read_byte((const u8*)&_EEPROM_DATA_.OperatingMode);

    // for (int bank = 0; bank < NUM_VIRTUAL_BANKS; bank++)
    // {
    //     for (int encoder = 0; encoder < NUM_ENCODERS; encoder++)
    //     {
    //         sEncoderState* pEncoder = &gData.EncoderStates[bank][encoder];

    //         EE_ReadVirtualEncoder(&pEncoder->Primary, &_EEPROM_DATA_.VirtualEncoders[bank][encoder][VIRTUALENCODER_PRIMARY]);
    //         EE_ReadVirtualEncoder(&pEncoder->Secondary, &_EEPROM_DATA_.VirtualEncoders[bank][encoder][VIRTUALENCODER_SECONDARY]);
    //         EE_ReadVirtualUber(&pEncoder->PrimaryUber, &_EEPROM_DATA_.VirtualUbers[bank][encoder][VIRTUALUBER_PRIMARY]);
    //         EE_ReadVirtualUber(&pEncoder->SecondaryUber, &_EEPROM_DATA_.VirtualUbers[bank][encoder][VIRTUALUBER_SECONDARY]);
    //         EE_ReadVirtualSwitch(&pEncoder->Switch, &_EEPROM_DATA_.VirtualSwitches[bank][encoder]);
    //     }
    // }
}