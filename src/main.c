/*
 * File: main.c ( 7th November 2021 )
 * Project: Muffin
 * Copyright 2021 - 2021 bxzn (mail@bxzn.one)
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

#include "Comms.h"
#include "Config.h"
#include "DMA.h"
#include "Data.h"
#include "Display.h"
#include "EncoderDisplay.h"
#include "Input.h"
#include "MIDI.h"
#include "Network.h"
#include "SoftTimer.h"
#include "System.h"
#include "USART.h"
#include "USB.h"

#define ENABLE_SERIAL
#include "VirtualSerial.h"

static sSoftTimer timer = {0};

static void SetupOperatingMode(void);
static void SetupTest(void);
static void RunTest(void);
static void BootAnimation(void);

// Can only execute after global interrupts have been enabled
// Must be before data init.
static void SetupOperatingMode(void)
{
    // Run update a few times to flush buffers
    for (int i = 0; i < 100; i++)
    {
        Input_Update();
    }

    Input_CheckSpecialSwitchCombos();

    // Check if need to start any special modes
    switch (gData.OperatingMode)
    {
        case TEST_MODE: SetupTest(); break;

        case BOOTLOADER_MODE: System_StartBootloader(); break;

        case DEFAULT_MODE:
        default: BootAnimation(); break;
    }
}

static void SetupTest(void)
{
    SoftTimer_Start(&timer);
}

static void RunTest(void)
{
    if (SoftTimer_Elapsed(&timer) >= 250)
    {
        Display_Test();
        SoftTimer_Start(&timer);
    }
}

static void BootAnimation(void)
{

    // for (u16 br = 0; br < BRIGHTNESS_MAX; br += 3)
    // {

    //     // Display_SetMaxBrightness(br);
    //     // Display_Update();
    //     //Delay_MS(10);
    // }
}

int main(void)
{
    // Do not adjust the order of these init functions!
    System_Init();

    // Peripherals and hardware
    DMA_Init();
    USART_Init();
    Input_Init();
    USB_Init();
    Display_Init();
    Serial_Init();
    SoftTimer_Init();

    // Middleware
    Encoder_Init();

    GlobalInterruptEnable();
    SetupOperatingMode();

    Data_Init();
    Comms_Init(); // Modules that require comms should be initialised after this point.
    Network_Init();
    MIDI_Init();

    EncoderDisplay_UpdateAllColours();

    Serial_Print("Booted\r\n");

#ifdef VSER_ENABLE

    SoftTimer_Stop(&timer);
    SoftTimer_Start(&timer);
    while (SoftTimer_Elapsed(&timer) < 2500) // Poll the USB and serial for a bit so that host is immediately ready for serial comms
    {
        Serial_Update();
        USB_USBTask();
    }
    SoftTimer_Stop(&timer);
#endif

    Network_StartDiscovery();

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
                MIDI_Update();
                Network_Update();
                break;
            }

            case TEST_MODE: RunTest(); break;

            default: break;
        }

        Serial_Update();
        USB_USBTask();
    }
}
