/*
 * File: Data.c ( 28th November 2021 )
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

#include <avr/eeprom.h>

#include "Colour.h"
#include "CommsTypes.h"
#include "Config.h"
#include "Data.h"
#include "DataTypes.h"
#include "Display.h"
#include "Encoder.h"
#include "EncoderDisplay.h"
#include "Input.h"
#include "Interrupt.h"

#define EE_DATA_VERSION (0xAFF2)

sData gData = {
    .DataVersion          = 0,
    .FirmwareVersion      = VERSION,
    .FactoryReset         = false,
    .OperatingMode        = DEFAULT_MODE,
    .NetworkAddress       = UNASSIGNED_ADDRESS,
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

    u8  OperatingMode;
    u16 NetworkAddress;

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

static inline void DisplayDataOnLEDS(void)
{
    EncoderDisplay_SetIndicatorValueU8(0, eeprom_read_byte((const u8*)&_EEPROM_DATA_.RGBBrightness));
    EncoderDisplay_SetIndicatorValueU8(1, gData.RGBBrightness);
    EncoderDisplay_SetIndicatorValueU8(4, eeprom_read_byte((const u8*)&_EEPROM_DATA_.DetentBrightness));
    EncoderDisplay_SetIndicatorValueU8(5, gData.DetentBrightness);
    EncoderDisplay_SetIndicatorValueU8(9, gData.IndicatorBrightness);
    EncoderDisplay_SetIndicatorValueU8(8, eeprom_read_byte((const u8*)&_EEPROM_DATA_.IndicatorBrightness));
    EncoderDisplay_SetIndicatorValueU8(12, eeprom_read_byte((const u8*)&_EEPROM_DATA_.OperatingMode));
    EncoderDisplay_SetIndicatorValueU8(13, gData.OperatingMode);
}

// Reads the user stored settings in the EEPROM for all encoders - data goes into gData.EncoderStates
static inline void ReadEEPROMEncoderSettings(void)
{
    for (int bank = 0; bank < NUM_VIRTUAL_BANKS; bank++)
    {
        for (int encoder = 0; encoder < NUM_ENCODERS; encoder++)
        {
            sEE_Encoder shadow = {0};
            eeprom_read_block((void*)&shadow, (const void*)&_EEPROM_DATA_.Encoders[bank][encoder], sizeof(shadow));

            sEncoderState* pEncoder = &gData.EncoderStates[bank][encoder];

            pEncoder->DisplayStyle   = shadow.BitFieldSettings.BitField.DisplayStyle;
            pEncoder->FineAdjust     = shadow.BitFieldSettings.BitField.FineAdjust;
            pEncoder->HasDetent      = shadow.BitFieldSettings.BitField.HasDetent;
            pEncoder->LayerA_Enabled = shadow.BitFieldSettings.BitField.LayerA_Enabled;
            pEncoder->LayerB_Enabled = shadow.BitFieldSettings.BitField.LayerB_Enabled;
            pEncoder->DetentHue      = shadow.DetentHue;

            for (int layer = 0; layer < NUM_VIRTUAL_ENCODER_LAYERS; layer++)
            {
                sVirtualEncoderLayer*    pLayer       = &pEncoder->Layers[layer];
                sEE_VirtualEncoderLayer* pShadowLayer = &shadow.Layers[layer];

                pLayer->MaxValue                = pShadowLayer->MaxValue;
                pLayer->MinValue                = pShadowLayer->MinValue;
                pLayer->StartPosition           = pShadowLayer->StartPosition;
                pLayer->StopPosition            = pShadowLayer->StopPosition;
                pLayer->RGBHue                  = pShadowLayer->RGBHue;
                pLayer->MidiConfig.Channel      = pShadowLayer->MidiConfig.MidiData.BitField.Channel;
                pLayer->MidiConfig.Mode         = pShadowLayer->MidiConfig.MidiData.BitField.Mode;
                pLayer->MidiConfig.MidiValue.CC = pShadowLayer->MidiConfig.MidiValue.CC;
            }
        }
    }
}

// Writes the current gData.EncoderStates to eeprom
static inline void WriteEEPROMEncoderSettings(void)
{
    for (int bank = 0; bank < NUM_VIRTUAL_BANKS; bank++)
    {
        for (int encoder = 0; encoder < NUM_ENCODERS; encoder++)
        {
            sEE_Encoder    shadow   = {0};
            sEncoderState* pEncoder = &gData.EncoderStates[bank][encoder];

            shadow.BitFieldSettings.BitField.DisplayStyle   = pEncoder->DisplayStyle;
            shadow.BitFieldSettings.BitField.FineAdjust     = pEncoder->FineAdjust;
            shadow.BitFieldSettings.BitField.HasDetent      = pEncoder->HasDetent;
            shadow.BitFieldSettings.BitField.LayerA_Enabled = pEncoder->LayerA_Enabled;
            shadow.BitFieldSettings.BitField.LayerB_Enabled = pEncoder->LayerB_Enabled;
            shadow.DetentHue                                = pEncoder->DetentHue;

            for (int layer = 0; layer < NUM_VIRTUAL_ENCODER_LAYERS; layer++)
            {
                sEE_VirtualEncoderLayer* pShadowLayer = &shadow.Layers[layer];
                sVirtualEncoderLayer*    pLayer       = &pEncoder->Layers[layer];

                pShadowLayer->MaxValue                             = pLayer->MaxValue;
                pShadowLayer->MinValue                             = pLayer->MinValue;
                pShadowLayer->StartPosition                        = pLayer->StartPosition;
                pShadowLayer->StopPosition                         = pLayer->StopPosition;
                pShadowLayer->RGBHue                               = pLayer->RGBHue;
                pShadowLayer->MidiConfig.MidiData.BitField.Channel = pLayer->MidiConfig.Channel;
                pShadowLayer->MidiConfig.MidiData.BitField.Mode    = pLayer->MidiConfig.Mode;
                pShadowLayer->MidiConfig.MidiValue.CC              = pLayer->MidiConfig.MidiValue.CC;
            }

            eeprom_update_block((const void*)&shadow, (void*)&_EEPROM_DATA_.Encoders[bank][encoder], sizeof(shadow));
        }
    }
}

void Data_Init(void)
{
    while (!eeprom_is_ready()) {} // wait for eeprom ready
    Data_CheckAndPerformFactoryReset(true);
}

bool Data_CheckAndPerformFactoryReset(bool CheckUserResetRequest)
{
    bool userReset = false;
    if (CheckUserResetRequest)
    {
        userReset = Input_IsResetPressed();
    }

    const vu8 flags   = IRQ_DisableInterrupts();
    // Read the version stored in eeprom, if this doesnt match then factory reset the unit.
    gData.DataVersion = eeprom_read_word(&_EEPROM_DATA_.DataVersion);

    gData.FactoryReset  = ((gData.DataVersion != EE_DATA_VERSION) || userReset);
    bool resetPerformed = false;

    if (gData.FactoryReset)
    {
        Data_WriteDefaultsToEEPROM();
        gData.FactoryReset = false;
        resetPerformed     = true;
    }

    IRQ_EnableInterrupts(flags);

    return resetPerformed;
}

// Disable interrupts before calling
void Data_WriteDefaultsToEEPROM(void)
{
    eeprom_update_byte(&_EEPROM_DATA_.Reserved, 0xFA);
    eeprom_update_word(&_EEPROM_DATA_.DataVersion, (u16)EE_DATA_VERSION);

    // At this point in execution gData should not have been modified and should be initialised
    // with the default values (see top of this file)
    eeprom_update_byte(&_EEPROM_DATA_.RGBBrightness, (u8)gData.RGBBrightness);
    eeprom_update_byte(&_EEPROM_DATA_.DetentBrightness, (u8)gData.DetentBrightness);
    eeprom_update_byte(&_EEPROM_DATA_.IndicatorBrightness, (u8)gData.IndicatorBrightness);
    eeprom_update_byte(&_EEPROM_DATA_.OperatingMode, (u8)gData.OperatingMode);
    eeprom_update_word(&_EEPROM_DATA_.NetworkAddress, (u16)gData.NetworkAddress);

    WriteEEPROMEncoderSettings();

    gData.DataVersion = EE_DATA_VERSION;
}

// Disable interrupts before calling
void Data_RecallEEPROMSettings(void)
{
    Display_SetRGBBrightness(eeprom_read_byte((const u8*)&_EEPROM_DATA_.RGBBrightness));
    Display_SetDetentBrightness(eeprom_read_byte((const u8*)&_EEPROM_DATA_.DetentBrightness));
    Display_SetIndicatorBrightness(eeprom_read_byte((const u8*)&_EEPROM_DATA_.IndicatorBrightness));
    gData.OperatingMode  = eeprom_read_byte((const u8*)&_EEPROM_DATA_.OperatingMode);
    gData.NetworkAddress = eeprom_read_word((const u16*)&_EEPROM_DATA_.NetworkAddress);
    ReadEEPROMEncoderSettings();
}

NetAddress Data_GetNetworkAddress(void)
{
    return (NetAddress)gData.NetworkAddress;
}

void Data_SetNetworkAddress(NetAddress NewAddress)
{
    if ((u16)NewAddress == gData.NetworkAddress)
    {
        return;
    }

    gData.NetworkAddress = (u16)NewAddress;

    const vu8 flags = IRQ_DisableInterrupts();
    eeprom_update_word(&_EEPROM_DATA_.NetworkAddress, (u16)gData.NetworkAddress);
    IRQ_EnableInterrupts(flags);
}