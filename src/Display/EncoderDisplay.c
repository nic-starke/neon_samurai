/*
 * File: EncoderDisplay.c ( 25th November 2021 )
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

#include <string.h>

#include "EncoderDisplay.h"
#include "Encoder.h"
#include "Types.h"
#include "Display.h"
#include "HardwareDescription.h"
#include "Input.h"

#define INDVAL_2_INDCOUNT(x) (((x) / ENCODER_MAX_VAL) * NUM_INDICATOR_LEDS)
#define SET_FRAME(f, x)      ((f) &= ~(x))
#define UNSET_FRAME(f, x)    ((f) |= (x))
#define PWM_CHECK(f, br)     ((f * MAGIC_BRIGHTNESS_VAL) < (br))

void EncoderDisplay_Test(void)
{
    for (int encoder = 0; encoder < NUM_ENCODERS; encoder++)
    {
        DisplayFrame frames[DISPLAY_BUFFER_SIZE];

        if ((bool)EncoderSwitchCurrentState(SWITCH_MASK(encoder)))
        {
            memset(frames, LED_ON, sizeof(frames));
        }
        else
        {
            memset(frames, LED_OFF, sizeof(frames));
        }

        Display_SetEncoderFrames(encoder, frames);
    }
}

void EncoderDisplay_Render(sEncoderState* pEncoder, DisplayFrame* pFrames, int EncoderIndex)
{
    sVirtualEncoder* pVE = (Encoder_IsSecondaryEnabled(pEncoder) ? &pEncoder->Secondary : &pEncoder->Primary);

    if (pVE->DisplayInvalid == false)
    {
        return;
    }

    switch ((eEncoderDisplayStyle)pVE->DisplayStyle)
    {
        case 
    }
}