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

#include <avr/pgmspace.h>

#include "DataTypes.h"
#include "Encoder.h"
#include "HardwareDescription.h"

typedef enum
{
    DEFAULT_MODE,
    TEST_MODE,
    BOOTLOADER_MODE,

    NUM_OPERATING_MODES,
} eOperatingMode;

typedef struct
{
    eOperatingMode OperatingMode;

    u8   FirmwareVersion;
    u16  DataVersion;
    bool FactoryReset;

    u8 RGBBrightness;
    u8 DetentBrightness;
    u8 IndicatorBrightness;

    bool LerpLayerRGB; // Interpolate between RGB layer colours within "transition arcs"

    sHardwareEncoder HardwareEncoders[NUM_ENCODERS];
    sEncoderState    EncoderStates[NUM_VIRTUAL_BANKS][NUM_ENCODERS];

    // Still need side switches here...

    u8 AccelerationConstant;
    u8 FineAdjustConstant;
    u8 CurrentBank;
} sData;

extern sData gData;

static inline void Data_PGMReadBlock(void* pDest, const void* pSrc, u8 size)
{
    // u8* dest = (u8*)pDest;
    // for (u8 i = 0; i < size; i++)
    // {
    //     dest[i] = pgm_read_byte(i + (const u8*)pSrc);
    // }

    memcpy_P(pDest, pSrc, size);
}

void Data_Init(void);
// void Data_FactoryReset(void);
// void Data_RecallUserSettings(void);
