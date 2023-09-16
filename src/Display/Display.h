/*
 * File: Display.h ( 13th November 2021 )
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

#define LED_ON	 (0x00)
#define LED_OFF	 (0xFF)
#define LEDS_ON	 (0x0000)
#define LEDS_OFF (0xFFFF)

#define LEDMASK(x)			 (1u << x)
#define LEDMASK_DET			 (0xC000)
#define LEDMASK_RGB			 (0x3800)
#define LEDMASK_IND			 (0xFFE0)
#define LEDMASK_IND_NODETENT (0xFFBF)

/* A display frame is a bitfield. Each bit corresponds to the state of a single LED
 * There are 16 LEDS per encoder therefore uint16_t is used. */
typedef u16 DisplayFrame;

#define DISPLAY_BUFFER_SIZE	 (32)  // Number of frames in display buffer
#define BRIGHTNESS_MAX		 (255) // Determines number of discrete PWM LED brightness levels for display driver
#define BRIGHTNESS_MIN		 (0)
#define MAGIC_BRIGHTNESS_VAL ((u8)8) // Precalculated - (MAX_BRIGHTNESS / DISPLAY_BUFFER_SIZE)

void Display_Init(void);