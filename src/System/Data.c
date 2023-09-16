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

#include "Types.h"
#include "Config.h"
#include "Data.h"
#include "Display.h"
#include "Encoder.h"
#include "Colour.h"

#define EE_DATA_VERSION (3)

sData gData = {
	.DataVersion		  = 0,
	.FirmwareVersion	  = VERSION,
	.FactoryReset		  = false,
	.OperatingMode		  = DEFAULT_MODE,
	.RGBBrightness		  = BRIGHTNESS_MAX,
	.DetentBrightness	  = BRIGHTNESS_MAX,
	.IndicatorBrightness  = BRIGHTNESS_MAX,
	.FineAdjustConstant	  = FINE_ADJUST_CONST,
	.AccelerationConstant = ACCEL_CONST,
};

typedef struct
{
	u8 Version;

	u8 OperatingMode; // This should be a bitfield, but you cannot take the address of a bitfield when attempting to ready from
					  // eeprom/pgmspace.

	u8 RGBBrightness;
	u8 DetentBrightness;
	u8 IndicatorBrightness;

	sEEVirtualEncoder VirtualEncoders[NUM_VIRTUAL_BANKS][NUM_ENCODERS][NUM_VIRTUAL_ENCODER_TYPES];
	sEEVirtualUber	  VirtualUbers[NUM_VIRTUAL_BANKS][NUM_ENCODERS][NUM_VIRTUAL_UBER_TYPES];
	sEEVirtualSwitch  VirtualSwitches[NUM_VIRTUAL_BANKS][NUM_ENCODERS];
} sEEData; // Data stored in EEPROM

// This data is in eeprom - access only via eeprom read/write functions
// The initialised values will be present within the .eep file - which has to manually written to
// cpu via AVRdude, standard users cannot do this, therefore the default values can also be written
// by calling the Data_FactoryReset function in this file.
sEEData mEEData EEMEM = {
	.Version			 = EE_DATA_VERSION,
	.OperatingMode		 = DEFAULT_MODE,
	.RGBBrightness		 = BRIGHTNESS_MAX,
	.DetentBrightness	 = BRIGHTNESS_MAX,
	.IndicatorBrightness = BRIGHTNESS_MAX,
};

static inline void EE_ReadVirtualEncoder(sVirtualEncoder* pDest, sEEVirtualEncoder* pSrc)
{
	sEEVirtualEncoder shadow = {0};
	eeprom_read_block(&shadow, pSrc, sizeof(sEEVirtualEncoder));

	pDest->FineAdjust	= shadow.FineAdjust;
	pDest->DisplayStyle = shadow.DisplayStyle;
	pDest->HasDetent	= shadow.HasDetent;
	pDest->MidiConfig	= shadow.MidiConfig;

	Hue2RGB(shadow.DetentHue, &pDest->DetentColour);
	Hue2RGB(shadow.RGBHue, &pDest->RGBColour);

	pDest->DisplayInvalid = true;
}

static inline void EE_WriteVirtualEncoder(sVirtualEncoder* pSrc, sEEVirtualEncoder* pDest)
{
	sEEVirtualEncoder data = {0};

	data.FineAdjust	  = pSrc->FineAdjust;
	data.DisplayStyle = pSrc->DisplayStyle;
	data.HasDetent	  = pSrc->HasDetent;
	data.MidiConfig	  = pSrc->MidiConfig;

	RGB2Hue(&data.DetentHue, &pSrc->DetentColour);
	RGB2Hue(&data.RGBHue, &pSrc->RGBColour);

	eeprom_write_block(&data, pDest, sizeof(sEEVirtualEncoder));
}

static inline void EE_ReadVirtualUber(sVirtualUber* pDest, sEEVirtualUber* pSrc)
{
	sEEVirtualUber shadow = {0};
	eeprom_read_block(&shadow, pSrc, sizeof(sEEVirtualUber));

	*pDest = shadow;
}

static inline void EE_WriteVirtualUber(sVirtualUber* pSrc, sEEVirtualUber* pDest)
{
	eeprom_write_block(pSrc, pDest, sizeof(sEEVirtualUber));
}

static inline void EE_ReadVirtualSwitch(sVirtualSwitch* pDest, sEEVirtualSwitch* pSrc)
{
	sEEVirtualSwitch shadow = {0};
	eeprom_read_block(&shadow, pSrc, sizeof(sEEVirtualSwitch));

	pDest->Mode		  = shadow.Mode;
	pDest->MidiConfig = shadow.MidiConfig;
}

static inline void EE_WriteVirtualSwitch(sVirtualSwitch* pSrc, sEEVirtualSwitch* pDest)
{
	sEEVirtualSwitch data = {0};

	data.Mode		= pSrc->Mode;
	data.MidiConfig = pSrc->MidiConfig;

	eeprom_write_block(&data, pDest, sizeof(sEEVirtualSwitch));
}

void Data_Init(void)
{
	NVM.CMD = 0x00; // Set NVM command register to NOP as per pgmspace.

	while (!eeprom_is_ready()) {} // wait for eeprom ready

	// Read the version stored in eeprom, if this doesnt match then factory reset the unit.
	gData.DataVersion  = eeprom_read_byte(&mEEData.Version);
	gData.FactoryReset = (gData.DataVersion != EE_DATA_VERSION);

	if (gData.FactoryReset)
	{
		Data_FactoryReset();
        gData.FactoryReset = false;
        Display_Flash(200, 5);
	}
	else
	{
		Data_RecallUserSettings();
        Display_Flash(200, 2);
	}
}

void Data_FactoryReset(void)
{
	eeprom_write_byte(&mEEData.Version, (u8)EE_DATA_VERSION);

	// At this point in execution gData should not have been modified and should be initialised with the default values (see top of this
	// file)
	eeprom_write_byte(&mEEData.RGBBrightness, gData.RGBBrightness);
	eeprom_write_byte(&mEEData.DetentBrightness, gData.DetentBrightness);
	eeprom_write_byte(&mEEData.IndicatorBrightness, gData.IndicatorBrightness);
	eeprom_write_byte(&mEEData.OperatingMode, gData.OperatingMode);

	Encoder_FactoryReset(); // firstly reset the encoder states in SRAM, then write this to EEPROM
	for (int bank = 0; bank < NUM_VIRTUAL_BANKS; bank++)
	{
		for (int encoder = 0; encoder < NUM_ENCODERS; encoder++)
		{
			sEncoderState* pEncoder = &gData.EncoderStates[bank][encoder];

			EE_WriteVirtualEncoder(&pEncoder->Primary, &mEEData.VirtualEncoders[bank][encoder][VIRTUALENCODER_PRIMARY]);
			EE_WriteVirtualEncoder(&pEncoder->Secondary, &mEEData.VirtualEncoders[bank][encoder][VIRTUALENCODER_SECONDARY]);
			EE_WriteVirtualUber(&pEncoder->PrimaryUber, &mEEData.VirtualUbers[bank][encoder][VIRTUALUBER_PRIMARY]);
			EE_WriteVirtualUber(&pEncoder->SecondaryUber, &mEEData.VirtualUbers[bank][encoder][VIRTUALUBER_SECONDARY]);
			EE_WriteVirtualSwitch(&pEncoder->Switch, &mEEData.VirtualSwitches[bank][encoder]);
		}
	}

	gData.DataVersion = EE_DATA_VERSION;
}

void Data_RecallUserSettings(void)
{
    gData.RGBBrightness		  = eeprom_read_byte(&mEEData.RGBBrightness);
	gData.DetentBrightness	  = eeprom_read_byte(&mEEData.DetentBrightness);
	gData.IndicatorBrightness = eeprom_read_byte(&mEEData.IndicatorBrightness);
	gData.OperatingMode		  = eeprom_read_byte(&mEEData.OperatingMode);

	for (int bank = 0; bank < NUM_VIRTUAL_BANKS; bank++)
	{
		for (int encoder = 0; encoder < NUM_ENCODERS; encoder++)
		{
			sEncoderState* pEncoder = &gData.EncoderStates[bank][encoder];

			EE_ReadVirtualEncoder(&pEncoder->Primary, &mEEData.VirtualEncoders[bank][encoder][VIRTUALENCODER_PRIMARY]);
			EE_ReadVirtualEncoder(&pEncoder->Secondary, &mEEData.VirtualEncoders[bank][encoder][VIRTUALENCODER_SECONDARY]);
			EE_ReadVirtualUber(&pEncoder->PrimaryUber, &mEEData.VirtualUbers[bank][encoder][VIRTUALUBER_PRIMARY]);
			EE_ReadVirtualUber(&pEncoder->SecondaryUber, &mEEData.VirtualUbers[bank][encoder][VIRTUALUBER_SECONDARY]);
			EE_ReadVirtualSwitch(&pEncoder->Switch, &mEEData.VirtualSwitches[bank][encoder]);
		}
	}
}