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
#include "SoftTimer.h"
#include "System.h"
#include "USART.h"
#include "USB.h"

#define ENABLE_SERIAL
#include "VirtualSerial.h"

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

    // for (u16 br = 0; br < BRIGHTNESS_MAX; br += 3)
    // {

    //     // Display_SetMaxBrightness(br);
    //     // Display_Update();
    //     //Delay_MS(10);
    // }
}

#include "CBOR.h"
static inline void CBORTest(void)
{
    sEncoderState initial = {0};
    sEncoderState decoded = {0};

    initial.CurrentValue  = 50;
    initial.PreviousValue = 100;

    // What is a correct stack size of this thing?
    UsefulBuf_MAKE_STACK_UB(buff, sizeof(sEncoderState));
    UsefulBufC encodedThing;
    QCBORError err;

    encodedThing = CBOREncode_Encoder(&initial, buff);
    if (UsefulBuf_IsNULLC(encodedThing))
    {
        Serial_Print("Failed to encode\r\n");
    }

    err = CBORDecode_Encoder(encodedThing, &decoded);
    if (err)
    {
        Serial_Print("Failed to decode\r\n");
    }

    Serial_Print("Size of encoded cbor is: ");
    char buf[128] = "";
    sprintf(buf, "%d bytes, bufsize is %d", encodedThing.len, sizeof(__pBufbuff));
    Serial_Print(buf);
    Serial_Print("\r\n");

    if (decoded.CurrentValue == initial.CurrentValue)
    {
        Serial_Print("Success - encode == decode\r\n");
    }
}

int main(void)
{
    // Do not adjust the order of these init functions!
    System_Init();
    DMA_Init();
    USART_Init();
    SoftTimer_Init();


    Comms_Init(); // Modules that register with comms should be initialised after this point.
    MIDI_Init();
    Input_Init();
    Encoder_Init();
    USB_Init();
    Serial_Init();
    Display_Init();

    GlobalInterruptEnable();

    Serial_Print("Booted\r\n");

    // Update inputs, check special input combinations
    for (int i = 0; i < 100; i++)
    {
        Input_Update();
        Input_CheckSpecialSwitchCombos();
    }

    // Check if need to start any special modes
    switch (gData.OperatingMode)
    {
        case TEST_MODE: SetupTest(); break;

        case BOOTLOADER_MODE: System_StartBootloader(); break;

        case DEFAULT_MODE:
        default: BootAnimation(); break;
    }

    // Reset encoders, read user settings (or reset eeprom), then update displays
    Encoders_ResetToDefaultConfig();
    Data_Init();
    EncoderDisplay_UpdateAllColours();

    // Poll the USB and serial for a bit so that host is immediately ready for serial comms
#ifdef VSER_ENABLE
    SoftTimer_Stop(&timer);
    SoftTimer_Start(&timer);
    while (SoftTimer_Elapsed(&timer) < 2500)
    {
        Serial_Update();
        USB_USBTask();
    }
    SoftTimer_Stop(&timer);
#endif

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
