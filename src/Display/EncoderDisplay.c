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
#include <math.h>

#include "EncoderDisplay.h"
#include "Encoder.h"
#include "DataTypes.h"
#include "Display.h"
#include "HardwareDescription.h"
#include "Input.h"
#include "Data.h"

#define INDVAL_2_INDCOUNT(x) (((x) / ENCODER_MAX_VAL) * NUM_INDICATOR_LEDS)
#define SET_FRAME(f, x)      ((f) &= ~(x))
#define UNSET_FRAME(f, x)    ((f) |= (x))
#define PWM_CHECK(f, br)     ((f * MAGIC_BRIGHTNESS_VAL) < (br))

// Render the RGB LEDs on a single frame.
static inline void RenderFrame_RGB(DisplayFrame* pFrame, int FrameIndex, sVirtualEncoder* pVE)
{
    float brightnessCoeff = gData.RGBBrightness / (float)BRIGHTNESS_MAX;

    if (PWM_CHECK(FrameIndex, pVE->RGBColour.Red * brightnessCoeff))
    {
        SET_FRAME(*pFrame, LEDMASK(LED_RGB_RED));
    }

    if (PWM_CHECK(FrameIndex, pVE->RGBColour.Green * brightnessCoeff))
    {
        SET_FRAME(*pFrame, LEDMASK(LED_RGB_GREEN));
    }

    if (PWM_CHECK(FrameIndex, pVE->RGBColour.Blue * brightnessCoeff))
    {
        SET_FRAME(*pFrame, LEDMASK(LED_RGB_BLUE));
    }
}

// Render the Detent Red/Blue LEDs on a single frame
static inline void RenderFrame_Detent(DisplayFrame* pFrame, int FrameIndex, sVirtualEncoder* pVE)
{
    if (PWM_CHECK(FrameIndex, gData.DetentBrightness))
    {
        if (PWM_CHECK(FrameIndex, pVE->DetentColour.Red))
        {
            SET_FRAME(*pFrame, LEDMASK(LED_DET_RED));
        }

        if (PWM_CHECK(FrameIndex, pVE->DetentColour.Blue))
        {
            SET_FRAME(*pFrame, LEDMASK(LED_DET_BLUE));
        }
    }

    // There is a white indicator LED in the same position as the detent LEDs, unset it.
    UNSET_FRAME(*pFrame, LEDMASK(LED_IND_6));
}

static inline void RenderEncoder_Dot(sVirtualEncoder* pVE, int EncoderIndex)
{
    float indicatorCountFloat = (pVE->CurrentValue / (float)ENCODER_MAX_VAL) * NUM_INDICATOR_LEDS;
    u8    indicatorCountInt   = ceilf(indicatorCountFloat);

    // Always draw the first indicator if in detent mode, the last one is always drawn based on above calculations.
    if (indicatorCountInt == 0 && pVE->HasDetent)
    {
        indicatorCountInt = 1;
    }

    DisplayFrame frames[DISPLAY_BUFFER_SIZE] = {0};

    for (int frame = 0; frame < DISPLAY_BUFFER_SIZE; frame++)
    {
        frames[frame] = LEDS_OFF; // set all LEDs off to start

        RenderFrame_RGB(&frames[frame], frame, pVE); // Render the RGB segment

        if (PWM_CHECK(frame, gData.IndicatorBrightness)) // Render the indicator LEDs
        {
            SET_FRAME(frames[frame], LEDMASK(NUM_ENCODER_LEDS - indicatorCountInt));
        }

        if (pVE->HasDetent && indicatorCountInt == 6)
        {
            RenderFrame_Detent(&frames[frame], frame, pVE);
        }
    }

    Display_SetEncoderFrames(EncoderIndex, &frames[0]);
}

static inline void Render_Test(int EncoderIndex)
{
    DisplayFrame frames[DISPLAY_BUFFER_SIZE] = {0};

    for (int frame = 0; frame < DISPLAY_BUFFER_SIZE; frame++)
    {
        frames[frame] = LEDS_OFF; // set all LEDs off to start
        SET_FRAME(frames[frame], LEDMASK(EncoderIndex));
    }

    Display_SetEncoderFrames(EncoderIndex, &frames[0]);
}

static inline void RenderEncoder_Bar(sVirtualEncoder* pVE, int EncoderIndex)
{
    float indicatorCountFloat = (pVE->CurrentValue / (float)ENCODER_MAX_VAL) * NUM_INDICATOR_LEDS;
    u8    indicatorCountInt   = ceilf(indicatorCountFloat);
    bool  drawDetent          = (indicatorCountInt == 6);
    bool  clearLeft           = (indicatorCountInt >= 6);
    bool  reverse             = (!clearLeft);

    if (reverse)
    {
        indicatorCountInt -= 1;
    }

    DisplayFrame frames[DISPLAY_BUFFER_SIZE] = {0};

    for (int frame = 0; frame < DISPLAY_BUFFER_SIZE; frame++)
    {
        frames[frame] = LEDS_OFF;

        RenderFrame_RGB(&frames[frame], frame, pVE);

        if (PWM_CHECK(frame, gData.IndicatorBrightness))
        {
            SET_FRAME(frames[frame], LEDMASK_IND & ~(LEDS_OFF >> indicatorCountInt));
        }

        if (pVE->HasDetent)
        {
            if (reverse)
            {
                if (PWM_CHECK(frame, gData.IndicatorBrightness))
                {
                    frames[frame] ^= 0xFC00;
                }
                else
                {
                    UNSET_FRAME(frames[frame], LEDMASK_IND);
                }
            }
            else if (clearLeft)
            {
                UNSET_FRAME(frames[frame], 0xF800);
            }

            if (drawDetent)
            {
                RenderFrame_Detent(&frames[frame], frame, pVE);
            }
        }
    }

    Display_SetEncoderFrames(EncoderIndex, &frames[0]);
}

static inline void RenderEncoder_BlendedBar(sVirtualEncoder* pVE, int EncoderIndex)
{
    float indicatorCountFloat        = (pVE->CurrentValue / (float)ENCODER_MAX_VAL) * NUM_INDICATOR_LEDS;
    u8    indicatorCountInt          = floorf(indicatorCountFloat);
    u8    partialIndicatorBrightness = (u8)((indicatorCountFloat - indicatorCountInt) * BRIGHTNESS_MAX);
    u8    partialIndicatorIndex      = NUM_ENCODER_LEDS - indicatorCountInt - 1;
    bool  drawDetent                 = (ceilf(indicatorCountFloat) == 6);
    bool  clearLeft                  = (indicatorCountInt > 5);
    bool  reverse                    = (!clearLeft);

    DisplayFrame frames[DISPLAY_BUFFER_SIZE] = {0};

    for (int frame = 0; frame < DISPLAY_BUFFER_SIZE; frame++)
    {
        frames[frame] = LEDS_OFF;

        RenderFrame_RGB(&frames[frame], frame, pVE);

        if (PWM_CHECK(frame, gData.IndicatorBrightness))
        {
            SET_FRAME(frames[frame], LEDMASK_IND & ~(LEDS_OFF >> indicatorCountInt));

            if (PWM_CHECK(frame, partialIndicatorBrightness))
            {
                SET_FRAME(frames[frame], LEDMASK(partialIndicatorIndex));
            }
        }

        if (pVE->HasDetent)
        {
            if (reverse)
            {
                if (PWM_CHECK(frame, gData.IndicatorBrightness))
                {
                    frames[frame] ^= 0xFC00;
                }
                else
                {
                    UNSET_FRAME(frames[frame], LEDMASK_IND);
                }
            }
            else if (clearLeft)
            {
                UNSET_FRAME(frames[frame], 0xF800);
            }

            if (drawDetent)
            {
                RenderFrame_Detent(&frames[frame], frame, pVE);
            }
        }
    }

    Display_SetEncoderFrames(EncoderIndex, &frames[0]);
}

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

void EncoderDisplay_Render(sEncoderState* pEncoder, int EncoderIndex)
{

    sVirtualEncoder* pVE = (pEncoder->PrimaryEnabled ? &pEncoder->Primary : &pEncoder->Secondary);

    if (pVE->DisplayInvalid == false)
    {
        return; // nothing to do so return
    }

    switch ((eEncoderDisplayStyle)pVE->DisplayStyle)
    {
        case STYLE_DOT: RenderEncoder_Dot(pVE, EncoderIndex); break;

        case STYLE_BAR: RenderEncoder_Bar(pVE, EncoderIndex); break;

        case STYLE_BLENDED_BAR: RenderEncoder_BlendedBar(pVE, EncoderIndex); break;

        default: break;
    }

    pVE->DisplayInvalid = false;
}

void EncoderDisplay_SetColour(sVirtualEncoder* pEncoder, sRGB* pEncoderColour, sHSV* pNewColour)
{
    fast_hsv2rgb_8bit(pNewColour->Hue, pNewColour->Saturation, pNewColour->Value, &pEncoderColour->Red, &pEncoderColour->Green, &pEncoderColour->Blue);
    pEncoder->DisplayInvalid = true;
}

// Invalidates all encoder displays in all banks
void EncoderDisplay_InvalidateAll(void)
{
    for(int bank = 0; bank < NUM_VIRTUAL_BANKS; bank++)
    {
        for(int encoder = 0; encoder < NUM_ENCODERS; encoder++)
        {
            gData.EncoderStates[bank][encoder].Primary.DisplayInvalid = true;
            gData.EncoderStates[bank][encoder].Secondary.DisplayInvalid = true;
        }
    }
}