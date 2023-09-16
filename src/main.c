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
#include "USB.h"

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

static inline void BootAnimation(void)
{

    for (u16 br = 0; br < BRIGHTNESS_MAX; br += 3)
    {
        Display_SetMaxBrightness(br);
        Display_Update();
        //Delay_MS(10);
    }
}

int main(void)
{
    // Do not adjust the order of these init functions!

    System_Init();
    DMA_Init();
    USART_Init();
    SoftTimer_Init();
    Input_Init();
    Encoder_Init();

    USB_Init();
    USBMidi_Init();

    Display_SetMaxBrightness(0);
    Display_Init();

    GlobalInterruptEnable();

    // When eeprom/user settings are working correctly reinstate data init and remove factory reset.
    Data_Init();
    Encoder_FactoryReset();

    // Update inputs, check special input combinations
    for (int i = 0; i < 100; i++)
    {
        Input_Update();
        Input_CheckSpecialSwitchCombos();
    }

    switch (gData.OperatingMode)
    {
        case TEST_MODE: SetupTest(); break;

        case BOOTLOADER_MODE: System_StartBootloader(); break;

        case DEFAULT_MODE:
        default: BootAnimation(); break;
    }

    while (1)
    {
        Input_Update();

        switch (gData.OperatingMode)
        {
            case DEFAULT_MODE:
            {
                Display_Update();
                Encoder_Update();
                // SideSwitch_Update();
                USBMidi_Update();
                break;
            }

            case TEST_MODE: RunTest(); break;

            default: break;
        }
        USB_USBTask();
    }
}