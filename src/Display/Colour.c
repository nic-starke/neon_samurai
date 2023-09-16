/*
 * File: Colour.c ( 28th November 2021 )
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

#include "Colour.h"
#include "Data.h"

static inline u16 ClampHue(u16 Hue)
{
    if (Hue >= HUE_MAX)
    {
        return HUE_MAX;
    }
    else
    {
        return Hue;
    }
}

void Hue2RGB(u16 Hue, sRGB* pRGB)
{
    Hue = ClampHue(Hue);
    fast_hsv2rgb_8bit(Hue, SATURATION_MAX, gData.RGBBrightness, &pRGB->Red, &pRGB->Green, &pRGB->Blue);
}