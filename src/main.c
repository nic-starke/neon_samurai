/*
 * File: main.c ( 7th November 2021 )
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

#ifndef __AVR_ATxmega128A4U__
#warning "This project has only been tested on the XMEGA 128A4U, continue at your own risk"
#endif

#include <Common/Common.h>
#include <Drivers/USB/USB.h>
#include <avr/wdt.h>

#include "Config.h"
#include "System.h"
#include "Display.h"
#include "DMA.h"
#include "USART.h"
#include "Input.h"
#include "EncoderDisplay.h"
#include "SoftTimer.h"
#include "Data.h"

static sSoftTimer timer = {0};

static inline void SetupTest(void)
{
    SoftTimer_Start(&timer);
}

static inline void RunTest(void)
{
    if (SoftTimer_Elapsed(&timer) >= 250)
    {
        Display_Test();
        SoftTimer_Start(&timer);
    }
}

int main(void)
{
    // Do not adjust the order of these init functions!

    System_Init();
    DMA_Init();
    USART_Init();
    SoftTimer_Init();
    Display_Init();
    Input_Init();
    Encoder_Init();

    USB_Init();

    GlobalInterruptEnable();

    Data_Init();

    if (gData.OperatingMode == TEST_MODE)
    {
        SetupTest();
    }

    while (1)
    {
        // Display_Update();
        Input_Update();

        switch (gData.OperatingMode)
        {
            case DEFAULT_MODE:
                EncoderDisplay_Test();
                // SideSwitch_Update();
                // Encoder_Update();
                // USBMidi_Update();
                break;

            case TEST_MODE: RunTest(); break;

            default: break;
        }

        USB_USBTask();
    }
}