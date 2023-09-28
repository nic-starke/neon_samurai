/*
 * File: Colour.c ( 28th November 2021 )
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

#include "Display/Colour.h"
#include "system/Data.h"
#include "system/types.h"

/**
 * @brief Clamp the hue to the maximum hue value.
 * 
 * @param Hue The value to be clamped.
 * @return uint16_t The clamped value.
 */
static inline uint16_t ClampHue(uint16_t Hue) // TODO - maybe this should just be a define?
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

/**
 * @brief Converts a hue value to an RGB struct.
 * 
 * @param Hue The hue value to convert.
 * @param pRGB A pointer to an RGB struct that will contain the converted values.
 */
void Hue2RGB(uint16_t Hue, sRGB* pRGB)
{
    Hue = ClampHue(Hue); //TODO - does not currently handle saturation or value.
    fast_hsv2rgb_8bit(Hue, SATURATION_MAX, VALUE_MAX, &pRGB->Red, &pRGB->Green, &pRGB->Blue);
}