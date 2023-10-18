/*
 * File: Input.h ( 27th November 2021 )
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

#include "core/types.h"
#include "core/HardwareDescription.h"

#define DIR_STATIONARY (0x00)
#define DIR_CW         (0x10)
#define DIR_CCW        (0x20)

#define SWITCH_ACTIVE   (0x01)
#define SWITCH_INACTIVE (0x00)
#define SWITCH_MASK(x)  (1u << x)

void Input_Init(void);
void Input_Update(void);
bool Input_IsResetPressed(void);
void Input_CheckSpecialSwitchCombos(void);

uint8_t  Encoder_GetDirection(uint8_t EncoderIndex);
uint16_t Encoder_GetSwitchState(
    uint16_t Mask); // mask can be (SWITCH_MASK(0) | SWITCH_MASK(3)| ...);
uint16_t EncoderSwitchWasPressed(uint16_t Mask);
uint16_t EncoderSwitchWasReleased(uint16_t Mask);
uint16_t EncoderSwitchCurrentState(uint16_t Mask);
uint8_t  SideSwitchWasPressed(uint8_t Mask);
uint8_t  SideSwitchWasReleased(uint8_t Mask);
uint8_t  SideSwitchCurrentState(uint8_t Mask);
