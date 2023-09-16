/*
 * File: Display.c ( 13th November 2021 )
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

#include <avr/io.h>
#include <string.h>

#include "Display.h"
#include "HardwareDescription.h"
#include "Types.h"
#include "DMA.h"
#include "USART.h"
#include "Timer.h"
#include "Settings.h"
#include "GPIO.h"
#include "SoftTimer.h"

#define DISPLAY_DMA_CH  (0)
#define DISPLAY_USART   (USARTD0)
#define USART_BAUD      (4000000)
#define DISPLAY_SR_PORT (PORTD)
#define DISPLAY_TIMER   (TCC0)

#define PIN_SR_ENABLE (0)
#define PIN_SR_CLK    (1)
#define PIN_SR_DATA   (3)
#define PIN_SR_LATCH  (4)
#define PIN_SR_RESET  (5)

volatile DisplayFrame DisplayBuffer[DISPLAY_BUFFER_SIZE][NUM_ENCODERS];
static vu8            mCurrentFrame;

static inline void EnableDisplay(void)
{
    GPIO_SetPinLevel(&DISPLAY_SR_PORT, PIN_SR_ENABLE, LOW);
}

static inline void DisableDisplay(void)
{
    GPIO_SetPinLevel(&DISPLAY_SR_PORT, PIN_SR_ENABLE, HIGH);
}

void Display_SetEncoderFrames(int EncoderIndex, DisplayFrame* pFrames)
{
    if (pFrames == NULL)
    {
        return;
    }

    for (int frame = 0; frame < DISPLAY_BUFFER_SIZE; frame++)
    {
        DisplayBuffer[frame][EncoderIndex] = pFrames[frame];
    }
}

void Display_ClearAll(void)
{
    memset(DisplayBuffer, LED_OFF, sizeof(DisplayBuffer));
}

void Display_Test(void)
{
    static int testEncoder = 0;

    for (int encoder = 0; encoder < NUM_ENCODERS; encoder++)
    {
        for (int frame = 0; frame < DISPLAY_BUFFER_SIZE; frame++)
        {
            DisplayBuffer[frame][encoder] = (encoder == testEncoder) ? LEDS_ON : LEDS_OFF;
        }
    }

    testEncoder = (testEncoder + 1) % NUM_ENCODERS;
}

void Display_Flash(int intervalMS, int Count)
{
    sSoftTimer timer = {0};
    SoftTimer_Start(&timer);

    do
    {
        while (SoftTimer_Elapsed(&timer) < intervalMS) {}
        SoftTimer_Start(&timer);
        for (int encoder = 0; encoder < NUM_ENCODERS; encoder++)
        {
            for (int frame = 0; frame < DISPLAY_BUFFER_SIZE; frame++)
            {
                DisplayBuffer[frame][encoder] = LEDS_ON;
            }
        }

        while (SoftTimer_Elapsed(&timer) < intervalMS) {}
        SoftTimer_Start(&timer);
        for (int encoder = 0; encoder < NUM_ENCODERS; encoder++)
        {
            for (int frame = 0; frame < DISPLAY_BUFFER_SIZE; frame++)
            {
                DisplayBuffer[frame][encoder] = LEDS_OFF;
            }
        }
    } while (--Count > 0);
}

void Display_Init(void)
{
    Display_ClearAll();

    GPIO_SetPinDirection(&DISPLAY_SR_PORT, PIN_SR_ENABLE, GPIO_OUTPUT);
    GPIO_SetPinDirection(&DISPLAY_SR_PORT, PIN_SR_CLK, GPIO_OUTPUT);
    GPIO_SetPinDirection(&DISPLAY_SR_PORT, PIN_SR_DATA, GPIO_OUTPUT);
    GPIO_SetPinDirection(&DISPLAY_SR_PORT, PIN_SR_LATCH, GPIO_OUTPUT);
    GPIO_SetPinDirection(&DISPLAY_SR_PORT, PIN_SR_RESET, GPIO_OUTPUT);

    const sDMA_ChannelConfig dmaConfig = {
        .pChannel = DMA_GetChannelPointer(DISPLAY_DMA_CH),

        .BurstLength          = DMA_CH_BURSTLEN_1BYTE_gc,
        .BytesPerTransfer     = DISPLAY_BUFFER_SIZE,  // 1 byte per SR
        .DoubleBufferMode     = DMA_DBUFMODE_CH01_gc, // Channels 0 and 1 in double buffer mode
        .DstAddress           = (u16)(uintptr_t)&DISPLAY_USART.DATA,
        .DstAddressingMode    = DMA_CH_DESTDIR_FIXED_gc,
        .DstReloadMode        = DMA_CH_DESTRELOAD_NONE_gc,
        .ErrInterruptPriority = PRIORITY_OFF,
        .InterruptPriority    = PRIORITY_OFF,
        .Repeats              = 0, // Single shot
        .SrcAddress           = (u16)(uintptr_t)DisplayBuffer,
        .SrcAddressingMode    = DMA_CH_SRCDIR_INC_gc,          // Auto increment through buffer array
        .SrcReloadMode        = DMA_CH_SRCRELOAD_NONE_gc,      // Address reload handled manually in
                                                               // display update isr
        .TriggerSource        = DMA_CH_TRIGSRC_USARTD0_DRE_gc, // USART SPI Master will trigger DMA
                                                               // transaction
    };

    DMA_InitChannel(&dmaConfig);

    const sUSART_ModuleConfig usartConfig = {
        .pUSART    = &DISPLAY_USART,
        .BaudRate  = USART_BAUD,
        .DataOrder = MSB_FIRST,
        .SPIMode   = SPI_MODE_0,
    };

    USART_InitModule(&usartConfig);

    sTimer_Type0Config timerConfig = {
        .pTimer       = &DISPLAY_TIMER,
        .ClockSource  = TC_CLKSEL_DIV256_gc,
        .Timer        = TIMER_TCC0,
        .WaveformMode = TC_WGMODE_NORMAL_gc,
    };

    Timer_Type0Init(&timerConfig);
    Timer_EnableChannelInterrupt(timerConfig.pTimer, TIMER_CHANNEL_A, PRIORITY_HI);
    timerConfig.pTimer->CCA = (u16)DISPLAY_REFRESH_RATE;

    // EncoderDisplays_Invalidate()

    EnableDisplay();

    DMA_EnableChannel(DMA_GetChannelPointer(DISPLAY_DMA_CH));

    GPIO_SetPinLevel(&DISPLAY_SR_PORT, PIN_SR_RESET, LOW);
    GPIO_SetPinLevel(&DISPLAY_SR_PORT, PIN_SR_RESET, HIGH);
}

// Display Interrupt
ISR(TCC0_CCA_vect)
{
    // Increment the timer
    DISPLAY_TIMER.CCA = DISPLAY_REFRESH_RATE + DISPLAY_TIMER.CNT;

    // Latch the SR so the data that has been previously transmitted gets set to  the SR outputs
    GPIO_SetPinLevel(&DISPLAY_SR_PORT, PIN_SR_LATCH, HIGH);
    GPIO_SetPinLevel(&DISPLAY_SR_PORT, PIN_SR_LATCH, LOW);

    DMA_EnableChannel(DMA_GetChannelPointer(DISPLAY_DMA_CH));

    // Reset the source address if we are at the end of the display buffer
    if (mCurrentFrame++ >= (DISPLAY_BUFFER_SIZE - 1))
    {
        while (DMA_ChannelBusy(DISPLAY_DMA_CH)) {}
        u8 flags = IRQ_DisableInterrupts();
        DMA_SetChannelSourceAddress(DMA_GetChannelPointer(DISPLAY_DMA_CH), (u16)(uintptr_t)DisplayBuffer);
        IRQ_EnableInterrupts(flags);
        mCurrentFrame = 0;
    }
}