/*
 * File: Data.h ( 28th November 2021 )
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

#pragma once

#include <avr/pgmspace.h>

#include "Comms/CommsTypes.h"
#include "system/types.h"
#include "Input/Encoder.h"
#include "system/HardwareDescription.h"

typedef enum {
  DEFAULT_MODE,
  TEST_MODE,
  BOOTLOADER_MODE,

  NUM_OPERATING_MODES,
} eOperatingMode;

typedef struct {
  uint8_t OperatingMode;

  uint8_t  FirmwareVersion;
  uint16_t DataVersion;
  bool     FactoryReset;

  uint16_t NetworkAddress;

  uint8_t RGBBrightness;
  uint8_t DetentBrightness;
  uint8_t IndicatorBrightness;

  bool LerpLayerRGB; // Interpolate between RGB layer colours within "transition
                     // arcs"

  sHardwareEncoder HardwareEncoders[NUM_ENCODERS];
  sEncoderState    EncoderStates[NUM_VIRTUAL_BANKS][NUM_ENCODERS];

  // Still need side switches here...

  uint8_t AccelerationConstant;
  uint8_t FineAdjustConstant;
  uint8_t CurrentBank;
} sData;

extern sData gData;

static inline void Data_PGMReadBlock(void* pDest, const void* pSrc,
                                     uint8_t size) {
  // uint8_t* dest = (uint8_t*)pDest;
  // for (uint8_t i = 0; i < size; i++)
  // {
  //     dest[i] = pgm_read_byte(i + (const uint8_t*)pSrc);
  // }

  memcpy_P(pDest, pSrc, size);
}

void Data_Init(void);
void Data_WriteDefaultsToEEPROM(void);
void Data_RecallEEPROMSettings(void);
bool Data_CheckAndPerformFactoryReset(bool CheckUserResetRequest);

NetAddress Data_GetNetworkAddress(void);
void       Data_SetNetworkAddress(NetAddress NewAddress);
