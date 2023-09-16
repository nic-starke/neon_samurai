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

static sSoftTimer mTestTimer = {0};

static void SetupOperatingMode(void);
static void SetupTest(void);
static void RunTest(void);
static void BootAnimation(void);

int main(void)
{
    /** ** WARNING ** Do not adjust the order of these init functions! **/
    System_Init();

    // Peripherals and hardware and middleware
    DMA_Init();
    USART_Init();
    Input_Init();
    USB_Init();
    Display_Init();
    Serial_Init();
    SoftTimer_Init();
    Encoder_Init();

    GlobalInterruptEnable();    // Enable global interrupts so that the display gets updated, and so I/O can be read.
    SetupOperatingMode();

    Data_Init();
    Comms_Init();               // Modules that require comms should be initialised after this point.
    Network_Init();
    MIDI_Init();

    EncoderDisplay_UpdateAllColours();    // FIXME - A better approach would be a single function to completely reset the display.

    Serial_Print("Booted\r\n");

#ifdef VSER_ENABLE
    SoftTimer_Stop(&mTestTimer);
    SoftTimer_Start(&mTestTimer);
    while (SoftTimer_Elapsed(&mTestTimer) < 2500) // Poll the USB and serial for a bit so that host is immediately ready for serial comms
    {
        Serial_Update();
        USB_USBTask();
    }
    SoftTimer_Stop(&mTestTimer);
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
                // SideSwitch_Update(); // FIXME Not yet implemented
                MIDI_Update();
                Network_Update();
                Comms_Update();
                break;
            }

            case TEST_MODE: RunTest(); break;

            default: break;
        }

        Serial_Update();
        USB_USBTask();
    }
}



/**
 * @brief Sets the operating mode based on special input switch combinations.
 * @warning This must only be executed if global interrupts are enabled.
 * @warning Execute this before Data_Init() to ensure EEPROM wipes occur before data recall.
 */
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

/**
 * @brief Setup any test mode variables
 * 
 */
static void SetupTest(void)
{
    SoftTimer_Start(&mTestTimer);
}

/**
 * @brief The function that gets called when running test mode.
 * 
 */
static void RunTest(void)
{
    if (SoftTimer_Elapsed(&mTestTimer) >= 250)
    {
        Display_Test();
        SoftTimer_Start(&mTestTimer);
    }
}

/**
 * @brief A simple boot animation.
 * 
 */
static void BootAnimation(void)
{
    // TODO Implement a separate timer for use as an arbitrary task scheduler.
    // Use this to run non-blocking animations.

    // for (u16 br = 0; br < BRIGHTNESS_MAX; br += 3)
    // {

    //     // Display_SetMaxBrightness(br);
    //     // Display_Update();
    //     //Delay_MS(10);
    // }
}