/*
 * File: Data.h ( 28th November 2021 )
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

#include "Types.h"
#include "Encoder.h"
#include "HardwareDescription.h"

typedef enum
{
	DEFAULT_MODE,

	NUM_OPERATING_MODES,
} eOperatingMode;

typedef struct
{
	eOperatingMode OperatingMode;

	u8 RGBBrightness;
	u8 DetentBrightness;
	u8 IndicatorBrightness;

	sHardwareEncoder HardwareEncoders[NUM_ENCODERS];
	sEncoderState	 EncoderStates[NUM_ENCODERS];
	sVirtualEncoder	 VirtualEncoders[NUM_VIRTUAL_ENCODERS];
	sVirtualUber	 VirtualUbers[NUM_VIRTUAL_UBERS];
    sVirtualSwitch  VirtualEncoderSwitches[NUM_VIRTUAL_SWITCHES];

    // Still need side switches here...

	u8 AccelerationConstant;
	u8 FineAdjustConstant;

	u8 CurrentBank : 2;
	u8 Reserved	   : 6;
} sData;

extern sData gData;

void Data_Init(void);