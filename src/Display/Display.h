/*
 * File: Display.h ( 31st October 2021 )
 * Project: Muffin
 * Copyright 2021 - 2021 Nic Starke (mail@bxzn.one)
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

#define BRIGHTNESS_MIN (0)
#define BRIGHTNESS_MAX (255) // if this changes then recalculate MAGIC_BRIGHTNESS_VAL

#define LED_ON	 (0x00)
#define LED_OFF	 (0xFF)
#define LEDS_ON	 (0x0000)
#define LEDS_OFF (0xFFFF)

#define LEDMASK(x)			 (1u << x)
#define LEDMASK_DET			 (0xC000)
#define LEDMASK_RGB			 (0x3800)
#define LEDMASK_IND			 (0xFFE0)
#define LEDMASK_IND_NODETENT (0xFFBF)

#define DISPLAY_BUF_SIZE (32)

#define MAGIC_BRIGHTNESS_VAL ((u8)8) // fixed calculation actual calculation is MAX_BRIGHTNESS/DISPLAY_BUF_SIZE

typedef u16 Frame; // A frame is 16 bits - 1 bit per LED (16 LEDs per encoder)

void Display_Init(void);