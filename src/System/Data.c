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

#include "Config.h"
#include "Data.h"
#include "Display.h"
#include "Encoder.h"
#include "Colour.h"

sData gData = {
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

	u8 RGBBrightness;
	u8 DetentBrightness;
	u8 IndicatorBrightness;

	u8 OperatingMode : 4;
	u8 Reserved		 : 4;

	sEEVirtualEncoder VirtualEncoders[NUM_VIRTUAL_BANKS][NUM_ENCODERS][NUM_VIRTUAL_ENCODER_TYPES];
	sEEVirtualUber	  VirtualUbers[NUM_VIRTUAL_BANKS][NUM_ENCODERS][NUM_VIRTUAL_UBER_TYPES];
	sEEVirtualSwitch  VirtualSwitches[NUM_VIRTUAL_BANKS][NUM_ENCODERS];
} sEEData; // Data stored in EEPROM

// This data is in eeprom - access only via eeprom read/write functions
sEEData mEEData EEMEM = {0};

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

static inline void EE_ReadVirtualUber(sVirtualUber* pDest, sEEVirtualUber* pSrc)
{
	sEEVirtualUber shadow = {0};
	eeprom_read_block(&shadow, pSrc, sizeof(sEEVirtualUber));

	*pDest = shadow;
}

static inline void EE_ReadVirtualSwitch(sVirtualSwitch* pDest, sEEVirtualSwitch* pSrc)
{
	sEEVirtualSwitch shadow = {0};
	eeprom_read_block(&shadow, pSrc, sizeof(sEEVirtualSwitch));

	pDest->MidiConfig = shadow.MidiConfig;
}

void Data_Init(void)
{
	while (!eeprom_is_ready()) {}						 // wait for eeprom ready
	for (int bank = 0; bank < NUM_VIRTUAL_BANKS; bank++) // Recall stored user settings.
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