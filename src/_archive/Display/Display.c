/*
 * File: Display.c ( 13th November 2021 )
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

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "Peripheral/DMA.h"
#include "system/Data.h"
#include "system/types.h"
#include "Display/Display.h"
#include "Display/EncoderDisplay.h"
#include "Peripheral/GPIO.h"
#include "Display/Gamma.h"
#include "system/HardwareDescription.h"
#include "system/Settings.h"
#include "system/SoftTimer.h"
#include "Peripheral/Timer.h"
#include "Peripheral/USART.h"

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

volatile DisplayFrame   DisplayBuffer[DISPLAY_BUFFER_SIZE][NUM_ENCODERS];
static volatile uint8_t mCurrentFrame = 0;

/**
 * @brief Enable the display (LEDs)
 */
static inline void EnableDisplay(void) {
  GPIO_SetPinLevel(&DISPLAY_SR_PORT, PIN_SR_ENABLE, LOW);
}

/**
 * @brief Disable the display (LEDs)
 */
static inline void DisableDisplay(void) {
  GPIO_SetPinLevel(&DISPLAY_SR_PORT, PIN_SR_ENABLE, HIGH);
}

/**
 * @brief Flushes the display shift registers.
 */
static inline void FlushDisplayRegisters(void) {
  for (int frame = 0; frame < DISPLAY_BUFFER_SIZE; frame++) {
    GPIO_SetPinLevel(&DISPLAY_SR_PORT, PIN_SR_LATCH, HIGH);
    GPIO_SetPinLevel(&DISPLAY_SR_PORT, PIN_SR_LATCH, LOW);
  }
}

/**
 * @brief Set the display frames for a specific encoder.
 *
 * @param EncoderIndex The encoder to set the display frames for.
 * @param pFrames A pointer to an array of display frames.
 */
void Display_SetEncoderFrames(int          EncoderIndex,
                              DisplayFrame (*pFrames)[DISPLAY_BUFFER_SIZE]) {
  if (pFrames == NULL) {
    return;
  }

  for (int frame = 0; frame < DISPLAY_BUFFER_SIZE; frame++) {
    // Encoder index is offset so that 0 will be the top left encoder display
    DisplayBuffer[frame][NUM_ENCODERS - 1 - EncoderIndex] = pFrames[frame];
  }
}

/**
 * @brief Sets the display buffer to for all encoders to LED_OFF.
 * Effectively turns off the display.
 */
void Display_ClearAll(void) {
  memset(&DisplayBuffer, LED_OFF, sizeof(DisplayBuffer));
}

/**
 * @brief A test function to that iterates through each encoder and enables its
 * leds.
 */
void Display_Test(void) {
  static int testEncoder = 0;

  for (int encoder = 0; encoder < NUM_ENCODERS; encoder++) {
    for (int frame = 0; frame < DISPLAY_BUFFER_SIZE; frame++) {
      DisplayBuffer[frame][encoder] =
          (encoder == testEncoder) ? LEDS_ON : LEDS_OFF;
    }
  }

  testEncoder = (testEncoder + 1) % NUM_ENCODERS;
}

/**
 * @brief Flash all LEDs a set number of times with a pause between each flash.
 * WARNING - this is a blocking function.
 * @param intervalMS The pause interval between flashes.
 * @param Count The number of times to flash.
 */
void Display_Flash(int intervalMS,
                   int Count) // TODO - convert to non-blocking when animation
                              // system is in place.
{
  sSoftTimer timer = {0};
  SoftTimer_Start(&timer);

  do {
    while (SoftTimer_Elapsed(&timer) < intervalMS) {}
    SoftTimer_Start(&timer);
    for (int encoder = 0; encoder < NUM_ENCODERS; encoder++) {
      for (int frame = 0; frame < DISPLAY_BUFFER_SIZE; frame++) {
        DisplayBuffer[frame][encoder] = LEDS_ON;
      }
    }

    while (SoftTimer_Elapsed(&timer) < intervalMS) {}
    SoftTimer_Start(&timer);
    for (int encoder = 0; encoder < NUM_ENCODERS; encoder++) {
      for (int frame = 0; frame < DISPLAY_BUFFER_SIZE; frame++) {
        DisplayBuffer[frame][encoder] = LEDS_OFF;
      }
    }
  } while (--Count > 0);
}

/**
 * @brief A helper function that will enable or disable all LEDs for a specified
 * encoder. Not to be used for for default operation.
 *
 * @param EncoderIndex The encoder to set.
 * @param State The state to set the LEDs to.
 */
void Display_SetEncoder(int EncoderIndex, bool State) {
  for (int frame = 0; frame < DISPLAY_BUFFER_SIZE; frame++) {
    DisplayBuffer[frame][NUM_ENCODERS - EncoderIndex - 1] =
        State ? LEDS_ON : LEDS_OFF;
  }
}

/**
 * @brief Initialises the display system.
 * Initialises the shift registers for all the LEDs.
 * Configures the USART peripheral for tranmission of display frames.
 * Configures the DMA peripheral for data transfer between application and USART
 * peripheral. Configures a timer peripheral as an interrupt to provide
 * controlled timing of the DMA transactions.
 */
void Display_Init(void) {
  Display_ClearAll();

  GPIO_SetPinDirection(&DISPLAY_SR_PORT, PIN_SR_ENABLE, GPIO_OUTPUT);
  GPIO_SetPinDirection(&DISPLAY_SR_PORT, PIN_SR_CLK, GPIO_OUTPUT);
  GPIO_SetPinDirection(&DISPLAY_SR_PORT, PIN_SR_DATA, GPIO_OUTPUT);
  GPIO_SetPinDirection(&DISPLAY_SR_PORT, PIN_SR_LATCH, GPIO_OUTPUT);
  GPIO_SetPinDirection(&DISPLAY_SR_PORT, PIN_SR_RESET, GPIO_OUTPUT);

  const sDMA_ChannelConfig dmaConfig = {
      .pChannel = DMA_GetChannelPointer(DISPLAY_DMA_CH),

      .BurstLength      = DMA_CH_BURSTLEN_1BYTE_gc,
      .BytesPerTransfer = NUM_LED_SHIFT_REGISTERS, // 1 byte per SR
      .DoubleBufferMode =
          DMA_DBUFMODE_CH01_gc, // Channels 0 and 1 in double buffer mode
      .DstAddress           = (uint16_t)(uintptr_t)&DISPLAY_USART.DATA,
      .DstAddressingMode    = DMA_CH_DESTDIR_FIXED_gc,
      .DstReloadMode        = DMA_CH_DESTRELOAD_NONE_gc,
      .ErrInterruptPriority = PRIORITY_OFF,
      .InterruptPriority    = PRIORITY_OFF,
      .Repeats              = 0, // Single shot
      .SrcAddress           = (uint16_t)(uintptr_t)DisplayBuffer,
      .SrcAddressingMode =
          DMA_CH_SRCDIR_INC_gc, // Auto increment through buffer array
      .SrcReloadMode =
          DMA_CH_SRCRELOAD_NONE_gc, // Address reload handled manually in
                                    // display update isr
      .TriggerSource = DMA_CH_TRIGSRC_USARTD0_DRE_gc, // USART SPI Master will
                                                      // trigger DMA transaction
  };

  DMA_InitChannel(&dmaConfig);

  const sUSART_ModuleConfig usartConfig = {
      .pUSART    = &DISPLAY_USART,
      .BaudRate  = USART_BAUD,
      .DataOrder = DO_LSB_FIRST,
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
  Timer_EnableChannelInterrupt(timerConfig.pTimer, TIMER_CHANNEL_A,
                               PRIORITY_HI);
  DISPLAY_TIMER.CCA = (uint16_t)DISPLAY_REFRESH_RATE;

  EncoderDisplay_InvalidateAll();
  FlushDisplayRegisters();

  GPIO_SetPinLevel(&DISPLAY_SR_PORT, PIN_SR_RESET, LOW);
  GPIO_SetPinLevel(&DISPLAY_SR_PORT, PIN_SR_RESET, HIGH);
}

/**
 * @brief Renders the display based on the current encoder data.
 * This should be called in the main loop.
 * WARNING - rendering is expensive, dont do it if not required!.
 */
void Display_Update(void) {
  for (int encoder = 0; encoder < NUM_ENCODERS; encoder++) {
    EncoderDisplay_Render(&gData.EncoderStates[gData.CurrentBank][encoder],
                          encoder);
  }
}

/**
 * @brief Set the brightness of all detent LEDs
 *
 * @param Brightness The brightness to set to.
 */
void Display_SetDetentBrightness(uint8_t Brightness) {
  // NVM.CMD                = NVM_CMD_NO_OPERATION_gc;
  gData.DetentBrightness = pgm_read_byte(&gamma_lut[Brightness]);
  EncoderDisplay_InvalidateAll();
}

/**
 * @brief Sets the brightness of all RGB LEDs.
 *
 * @param Brightness The brightness to set to.
 */
void Display_SetRGBBrightness(uint8_t Brightness) {
  // NVM.CMD                   = NVM_CMD_NO_OPERATION_gc;
  gData.RGBBrightness = pgm_read_byte(&gamma_lut[Brightness]);
  EncoderDisplay_InvalidateAll();
}

/**
 * @brief Sets the brightness of all indicator LEDs.
 *
 * @param Brightness The brightness to set to.
 */
void Display_SetIndicatorBrightness(uint8_t Brightness) {
  // NVM.CMD                   = NVM_CMD_NO_OPERATION_gc;
  gData.IndicatorBrightness = pgm_read_byte(&gamma_lut[Brightness]);
  EncoderDisplay_InvalidateAll();
}

/**
 * @brief Set the brightness for all LEDs.
 *
 * @param Brightness The brightness to set to.
 */
void Display_SetMaxBrightness(uint8_t Brightness) {
  Display_SetDetentBrightness(Brightness);
  Display_SetRGBBrightness(Brightness);
  Display_SetIndicatorBrightness(Brightness);
}

/**
 * @brief The timer interrupt to handle the display DMA transactions.
 */
ISR(TCC0_CCA_vect) {
  // Increment the timer
  DISPLAY_TIMER.CCA = DISPLAY_REFRESH_RATE + DISPLAY_TIMER.CNT;

  // Latch the SR so the data that has been previously transmitted gets set to
  // the SR outputs
  GPIO_SetPinLevel(&DISPLAY_SR_PORT, PIN_SR_LATCH, HIGH);
  GPIO_SetPinLevel(&DISPLAY_SR_PORT, PIN_SR_LATCH, LOW);

  // Enable the DMA channel - this will start the transactions.
  DMA_EnableChannel(DMA_GetChannelPointer(DISPLAY_DMA_CH));

  // Reset the source address if we are at the end of the display buffer
  if (mCurrentFrame++ >= (DISPLAY_BUFFER_SIZE - 1)) {
    while (DMA_ChannelBusy(DISPLAY_DMA_CH)) {
    } // TODO - is blocking here a good idea? maybe the address increment should
      // be automatically handled.
    uint8_t flags = IRQ_DisableInterrupts();
    DMA_SetChannelSourceAddress(DMA_GetChannelPointer(DISPLAY_DMA_CH),
                                (uint16_t)(uintptr_t)DisplayBuffer);
    mCurrentFrame = 0;
    IRQ_EnableInterrupts(flags);
  }
}
