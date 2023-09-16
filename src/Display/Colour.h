/*
 * File: Colour.h ( 28th November 2021 )
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

#include "DataTypes.h"
#include "fast_hsv2rgb.h"

#define HUE_MAX        (HSV_HUE_MAX)
#define SATURATION_MAX (HSV_SAT_MAX)
#define VALUE_MAX      (HSV_VAL_MAX)

#define RGB_MAX_VAL (UINT8_MAX)
#define RGB_MIN_VAL (0)

// clang-format off
#define RGB_RED         {255, 0, 0}
#define RGB_YELLOW      {255, 255, 0}
#define RGB_GREEN       {0, 255, 0}
#define RGB_CYAN        {0, 255, 255}
#define RGB_BLUE        {0, 0, 255}
#define RGB_FUSCHIA     {255, 0, 255}
#define RGB_WHITE       {255, 255, 255}

#define HSV_RED         {0, SATURATION_MAX, VALUE_MAX}
#define HSV_BLUE        {50, SATURATION_MAX, VALUE_MAX}
#define HSV_GREEN       {200, SATURATION_MAX, VALUE_MAX}

#define HUE_RED         (0)
#define HUE_GREEN       (HUE_MAX/2)
#define HUE_BLUE        (HUE_MAX)
// clang-format on

typedef struct
{
    u16 Hue;
    u8  Saturation;
    u8  Value;
} sHSV;

typedef struct
{
    u8 Red;
    u8 Green;
    u8 Blue;
} sRGB;

void Hue2RGB(u16 Hue, sRGB* pRGB);