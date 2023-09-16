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

sData gData = 
{
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

	sEEVirtualEncoder VirtualEncoders[NUM_VIRTUAL_ENCODERS];
    sEEVirtualUber VirtualUbers[NUM_VIRTUAL_UBERS]; 
    sEEVirtualSwitch VirtualSwitches[NUM_VIRTUAL_SWITCHES];
} sEEData; // Data stored in EEPROM

sEEData mEEData EEMEM = 
{
    .Version = 1,
};

static u8 Version = 0;

void Data_Init(void)
{
    // To get the compiler to assign memory these variables need to be assigned/used in some way.
	gData.EncoderStates[0].pPrimary	  = &gData.VirtualEncoders[0];
	gData.EncoderStates[0].pSecondary = &gData.VirtualEncoders[1];
	// Read EE user settings to RAM.

    while(!eeprom_is_ready()) {} // wait for eeprom ready
    
    Version = eeprom_read_byte(&mEEData.Version);
    Version++;
    eeprom_write_byte(&mEEData.Version, Version);
}