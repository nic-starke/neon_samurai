/*
 * File: Input.c ( 27th November 2021 )
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

/*
 * Rotory decoder based on
 * https://github.com/buxtronix/arduino/tree/master/libraries/Rotary Copyright
 * 2011 Ben Buxton. Licenced under the GNU GPL Version 3. Contact: bb@cactii.net
 */

#include "Input.h"
#include "GPIO.h"
#include "DataTypes.h"
#include "Timer.h"
#include "Settings.h"
#include "Display.h"
#include "Data.h"

#define SIDE_SWITCH_PORT    (PORTA)
#define ENCODER_PORT        (PORTC)
#define PIN_ENCODER_LATCH   (0)
#define PIN_ENCODER_CLK     (1)
#define PIN_ENCODER_DATA_IN (2)

#define DEBOUNCE_BUF_LEN (10)

#define INPUT_TIMER (TCC1)

#define BOOTLOADER_SWITCH_MASK     (SWITCH_MASK(0) | SWITCH_MASK(3) | SWITCH_MASK(12) | SWITCH_MASK(15))
#define BOOTLOADER_SWITCH_VAL      (0x9009)
#define ALT_BOOTLOADER_SWITCH_MASK (SWITCH_MASK(15) | SWITCH_MASK(14) | SWITCH_MASK(11) | SWITCH_MASK(10))
#define ALT_BOOTLOADER_SWITCH_VAL  (0xCC00)
#define RESET_SWITCH_MASK          (SWITCH_MASK(0) | SWITCH_MASK(15))
#define RESET_SWITCH_VAL           (0x8001)

typedef struct
{
    u16 Buffer[DEBOUNCE_BUF_LEN]; // Raw switch states from shift register
    u8  Index;
    u16 bfChangedStates;   // Which switches changed states in current tick
    u16 bfDebouncedStates; // Debounced switch state - use this as the current switch state
} sEncoderSwitches;

typedef struct
{
    u8 Buffer[DEBOUNCE_BUF_LEN];
    u8 Index;
    u8 bfChangedStates;
    u8 bfDebouncedStates;
} sSideSwitches;

typedef enum
{
    R_START,
    R_CCW_BEGIN,
    R_CW_BEGIN,
    R_START_M,
    R_CW_BEGIN_M,
    R_CCW_BEGIN_M,

    NUM_ENCODER_ROTARY_STATES,
} eEncoderRotationState;

// clang-format off

static const eEncoderRotationState EncoderRotLUT[NUM_ENCODER_ROTARY_STATES][4] = {
		{R_START_M,            R_CW_BEGIN,     R_CCW_BEGIN,  R_START},					// R_START (00)
		{R_START_M | DIR_CCW,  R_START,        R_CCW_BEGIN,  R_START},					// R_CCW_BEGIN
		{R_START_M | DIR_CW,   R_CW_BEGIN,     R_START,      R_START},					// R_CW_BEGIN
		{R_START_M,            R_CCW_BEGIN_M,  R_CW_BEGIN_M, R_START},					// R_START_M (11)
		{R_START_M,            R_START_M,      R_CW_BEGIN_M, R_START | DIR_CW},			// R_CW_BEGIN_M
		{R_START_M,            R_CCW_BEGIN_M,  R_START_M,    R_START | DIR_CCW},		// R_CCW_BEGIN_M
};

// clang-format on

// bit-fields for current shift register states
static vu16 mEncoderSR_CH_A;
static vu16 mEncoderSR_CH_B;
static u8   mEncoderRotationStates[NUM_ENCODERS];

static volatile sEncoderSwitches mEncoderSwitchStates;
static volatile sSideSwitches    mSideSwitchStates;

void Input_Init(void)
{
    for (int sidesw = 0; sidesw < NUM_SIDE_SWITCHES; sidesw++)
    {
        GPIO_SetPinDirection(&SIDE_SWITCH_PORT, sidesw, GPIO_INPUT);
        GPIO_SetPinMode(&SIDE_SWITCH_PORT, sidesw, PORT_OPC_PULLUP_gc);
    }

    GPIO_SetPinDirection(&ENCODER_PORT, PIN_ENCODER_LATCH, GPIO_OUTPUT);
    GPIO_SetPinDirection(&ENCODER_PORT, PIN_ENCODER_CLK, GPIO_OUTPUT);
    GPIO_SetPinDirection(&ENCODER_PORT, PIN_ENCODER_DATA_IN, GPIO_INPUT);

    GPIO_SetPinLevel(&ENCODER_PORT, PIN_ENCODER_LATCH, HIGH);
    GPIO_SetPinLevel(&ENCODER_PORT, PIN_ENCODER_LATCH, LOW);

    sTimer_Type0Config timerConfig = {
        .pTimer       = &INPUT_TIMER,
        .ClockSource  = TC_CLKSEL_DIV1024_gc,
        .Timer        = TIMER_TCC1,
        .WaveformMode = TC_WGMODE_NORMAL_gc,
    };

    Timer_Type0Init(&timerConfig);
    Timer_EnableChannelInterrupt(timerConfig.pTimer, TIMER_CHANNEL_A, PRIORITY_MED);
    timerConfig.pTimer->CCA = (u16)INPUT_SCAN_RATE;
}

void Input_Update(void)
{
    // Update encoder rotations
    for (u8 encoder = 0; encoder < NUM_ENCODERS; encoder++)
    {
        bool chA = (bool)(mEncoderSR_CH_A & (1u << encoder));
        bool chB = (bool)(mEncoderSR_CH_B & (1u << encoder));
        u8   val = (chB << 1) | chA;

        mEncoderRotationStates[encoder] = EncoderRotLUT[mEncoderRotationStates[encoder] & 0x0F][val];
    }

    // Debounce switches and set current states;
    u16 prevEncSwitchStates  = mEncoderSwitchStates.bfDebouncedStates;
    u16 prevSideSwitchStates = mSideSwitchStates.bfDebouncedStates;

    mEncoderSwitchStates.bfDebouncedStates = 0xFFFF;
    mSideSwitchStates.bfDebouncedStates    = 0xFF;

    for (u8 i = 0; i < DEBOUNCE_BUF_LEN; i++)
    {
        mEncoderSwitchStates.bfDebouncedStates &= mEncoderSwitchStates.Buffer[i];
        mSideSwitchStates.bfDebouncedStates &= mSideSwitchStates.Buffer[i];
    }

    mEncoderSwitchStates.bfChangedStates = mEncoderSwitchStates.bfDebouncedStates ^ prevEncSwitchStates;
    mSideSwitchStates.bfChangedStates    = mSideSwitchStates.bfDebouncedStates ^ prevSideSwitchStates;
}

bool Input_IsResetPressed(void)
{
    return (bool)(EncoderSwitchWasPressed(RESET_SWITCH_MASK) == RESET_SWITCH_VAL);
}

void Input_CheckSpecialSwitchCombos(void)
{
    u16 state = mEncoderSwitchStates.bfDebouncedStates;
    if (((state & BOOTLOADER_SWITCH_MASK) == BOOTLOADER_SWITCH_VAL) || ((state & ALT_BOOTLOADER_SWITCH_MASK) == ALT_BOOTLOADER_SWITCH_VAL))
    {
        gData.OperatingMode = BOOTLOADER_MODE;
    }
}

u8 Encoder_GetDirection(u8 EncoderIndex)
{
    return mEncoderRotationStates[EncoderIndex] & 0x30;
}

u16 EncoderSwitchWasPressed(u16 Mask)
{
    return (mEncoderSwitchStates.bfChangedStates & mEncoderSwitchStates.bfDebouncedStates) & Mask;
}

u16 EncoderSwitchWasReleased(u16 Mask)
{
    return (mEncoderSwitchStates.bfChangedStates & (~mEncoderSwitchStates.bfDebouncedStates)) & Mask;
}

u16 EncoderSwitchCurrentState(u16 Mask)
{
    return mEncoderSwitchStates.bfDebouncedStates & Mask;
}

u8 SideSwitchWasPressed(u8 Mask)
{
    return (mSideSwitchStates.bfChangedStates & mSideSwitchStates.bfDebouncedStates) & Mask;
}

u8 SideSwitchWasReleased(u8 Mask)
{
    return (mSideSwitchStates.bfChangedStates & (~mSideSwitchStates.bfDebouncedStates)) & Mask;
}

u8 SideSwitchCurrentState(u8 Mask)
{
    return mSideSwitchStates.bfDebouncedStates & Mask;
}

static inline void UpdateSideSwitchStates(void)
{
    mSideSwitchStates.Buffer[mSideSwitchStates.Index] = 0;
    for (int sidesw = 0; sidesw < NUM_SIDE_SWITCHES; sidesw++)
    {
        u8 state = (bool)GPIO_GetPinLevel(&SIDE_SWITCH_PORT, sidesw);
        SET_REG(mSideSwitchStates.Buffer[mSideSwitchStates.Index], (state << sidesw));
    }

    mSideSwitchStates.Index = (mSideSwitchStates.Index + 1) % DEBOUNCE_BUF_LEN;
}

static inline void UpdateEncoderSwitchStates(void)
{
    mEncoderSwitchStates.Buffer[mEncoderSwitchStates.Index] = 0;
    for (int encodersw = 0; encodersw < NUM_ENCODER_SWITCHES; encodersw++)
    {
        GPIO_SetPinLevel(&ENCODER_PORT, PIN_ENCODER_CLK, LOW);
        u16 state = (bool)GPIO_GetPinLevel(&ENCODER_PORT, PIN_ENCODER_DATA_IN);
        SET_REG(mEncoderSwitchStates.Buffer[mEncoderSwitchStates.Index], (!state << encodersw));
        GPIO_SetPinLevel(&ENCODER_PORT, PIN_ENCODER_CLK, HIGH);
    }

    mEncoderSwitchStates.Index = (mEncoderSwitchStates.Index + 1) % DEBOUNCE_BUF_LEN;
}

static inline void UpdateEncoderQuadratureStates(void)
{
    mEncoderSR_CH_A = mEncoderSR_CH_B = 0;
    for (int encoder = 0; encoder < NUM_ENCODERS; encoder++)
    {
        GPIO_SetPinLevel(&ENCODER_PORT, PIN_ENCODER_CLK, LOW);
        SET_REG(mEncoderSR_CH_A, GPIO_GetPinLevel(&ENCODER_PORT, PIN_ENCODER_DATA_IN) ? (1u << encoder) : 0);
        GPIO_SetPinLevel(&ENCODER_PORT, PIN_ENCODER_CLK, HIGH);

        GPIO_SetPinLevel(&ENCODER_PORT, PIN_ENCODER_CLK, LOW);
        SET_REG(mEncoderSR_CH_B, GPIO_GetPinLevel(&ENCODER_PORT, PIN_ENCODER_DATA_IN) ? (1u << encoder) : 0);
        GPIO_SetPinLevel(&ENCODER_PORT, PIN_ENCODER_CLK, HIGH);
    }
}

ISR(TCC1_CCA_vect)
{
    INPUT_TIMER.CCA = INPUT_SCAN_RATE + INPUT_TIMER.CNT;
    GPIO_SetPinLevel(&ENCODER_PORT, PIN_ENCODER_LATCH, HIGH);
    UpdateSideSwitchStates();
    UpdateEncoderSwitchStates();
    UpdateEncoderQuadratureStates();
    GPIO_SetPinLevel(&ENCODER_PORT, PIN_ENCODER_LATCH, LOW);
}