/*
 * File: Display.h ( 13th November 2021 )
 * Project: Muffin
 * Copyright 2021 bxzn (mail@bxzn.one)
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

#define LED_ON               (0x00)
#define LED_OFF              (0xFF)
#define LEDS_ON              (0x0000)
#define LEDS_OFF             (0xFFFF)
#define LEDMASK(x)           (1u << (x))
#define LEDMASK_DET          (0xC000)
#define LEDMASK_RGB          (0x3800)
#define LEDMASK_IND          (0xFFE0)
#define LEDMASK_IND_NODETENT (0xFFBF)
#define BRIGHTNESS_MAX       (255) // Determines number of discrete PWM LED brightness levels for display driver
#define BRIGHTNESS_MIN       (0)
#define DISPLAY_BUFFER_SIZE  (32)    // Number of frames in display buffer
#define MAGIC_BRIGHTNESS_VAL ((u8)8) // Precalculated - (MAX_BRIGHTNESS / DISPLAY_BUFFER_SIZE)

typedef enum
{
    LED_TYPE_INDICATOR,
    LED_TYPE_DETENT,
    LED_TYPE_RGB,
} eLEDType;

// LEDs in order of the shift register pinout
typedef enum
{
    LED_DET_BLUE,
    LED_DET_RED,
    LED_RGB_BLUE,
    LED_RGB_RED,
    LED_RGB_GREEN,
    LED_IND_11,
    LED_IND_10,
    LED_IND_9,
    LED_IND_8,
    LED_IND_7,
    LED_IND_6,
    LED_IND_5,
    LED_IND_4,
    LED_IND_3,
    LED_IND_2,
    LED_IND_1,

    NUM_ENCODER_LEDS,
} eEncoderLED;

/* A display frame is a bitfield. Each bit corresponds to the state of a single
 * LED There are 16 LEDS per encoder therefore uint16_t is used. */
typedef u16 DisplayFrame;

void Display_Init(void);
void Display_Update(void);
void Display_ClearAll(void);
void Display_SetEncoderFrames(int EncoderIndex, DisplayFrame* pFrames);
void Display_Test(void);
void Display_Flash(int intervalMS, int Count);

// Sets all the LEDs for a specified encoder on or off based on State.
//  EncoderIndex range is 0 to 15
void Display_SetEncoder(int EncoderIndex, bool State);

void Display_SetDetentBrightness(u8 Brightness);
void Display_SetRGBBrightness(u8 Brightness);
void Display_SetIndicatorBrightness(u8 Brightness);
void Display_SetMaxBrightness(u8 Brightness);
