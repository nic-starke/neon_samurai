/*
 * File: Input.c ( 27th November 2021 )
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

#include "Input/Input.h"
#include "system/Data.h"
#include "system/types.h"
#include "Display/Display.h"
#include "Peripheral/GPIO.h"
#include "MIDI/MIDI.h"
#include "system/Settings.h"
#include "Peripheral/Timer.h"
#include "USB/USB.h"

#define SIDE_SWITCH_PORT    (PORTA)
#define ENCODER_PORT        (PORTC)
#define PIN_ENCODER_LATCH   (0)
#define PIN_ENCODER_CLK     (1)
#define PIN_ENCODER_DATA_IN (2)

#define DEBOUNCE_BUF_LEN (10)

#define INPUT_TIMER (TCC1)

/*
    Switch masks are used for special input combinations.
    For example - the bootloader can be entered during powerup by holding in
   encoder switches 0, 3, 12 and 15. (all 4 corners)
*/
#define BOOTLOADER_SWITCH_MASK                                                 \
  (SWITCH_MASK(0) | SWITCH_MASK(3) | SWITCH_MASK(12) | SWITCH_MASK(15))
#define BOOTLOADER_SWITCH_VAL      (0x9009)
#define ALT_BOOTLOADER_SWITCH_MASK (SWITCH_MASK(15) | SWITCH_MASK(11))
#define ALT_BOOTLOADER_SWITCH_VAL  (0x8800)
#define RESET_SWITCH_MASK          (SWITCH_MASK(0) | SWITCH_MASK(15))
#define RESET_SWITCH_VAL           (0x8001)
#define MIDI_MIRROR_SWITCH_MASK    (SWITCH_MASK(14) | SWITCH_MASK(13))
#define MIDI_MIRROR_SWITCH_VAL     (0x6000)

typedef struct {
  uint16_t Buffer[DEBOUNCE_BUF_LEN]; // Raw switch states from shift register
  uint8_t  Index;
  uint16_t raw_state;        // Which switches changed states in current tick
  uint16_t debounces_states; // Debounced switch state - use this as the
                             // current switch state
} sEncoderSwitches;

typedef struct {
  uint8_t Buffer[DEBOUNCE_BUF_LEN];
  uint8_t Index;
  uint8_t raw_state;
  uint8_t debounces_states;
} sSideSwitches;

typedef enum {
  QUAD_START,
  QUAD_CCW,
  QUAD_CW,
  QUAD_MIDDLE,
  QUAD_MID_CW,
  QUAD_MID_CCW,

  QUAD_NB,
} eQuadratureState;

// clang-format off

/*
 * Rotory decoder based on
 * https://github.com/buxtronix/arduino/tree/master/libraries/Rotary Copyright
 * 2011 Ben Buxton. Licenced under the GNU GPL Version 3. Contact: bb@cactii.net
 */
static const eQuadratureState EncoderRotLUT[QUAD_NB][4] = {       // Current Quadrature GrayCode
		{QUAD_MIDDLE,            QUAD_CW,       QUAD_CCW,        QUAD_START},				// 00
		{QUAD_MIDDLE | DIR_CCW,  QUAD_START,       QUAD_CCW,        QUAD_START},				// 01
		{QUAD_MIDDLE | DIR_CW,   QUAD_CW,       QUAD_START,        QUAD_START},				// 10
		{QUAD_MIDDLE,            QUAD_MID_CCW,      QUAD_MID_CW,       QUAD_START},				// 11
		{QUAD_MIDDLE,            QUAD_MIDDLE,       QUAD_MID_CW,       QUAD_START | DIR_CW},		// 100
		{QUAD_MIDDLE,            QUAD_MID_CCW,      QUAD_MIDDLE,        QUAD_START | DIR_CCW},		// 101
};

// clang-format on

// bit-fields for current shift register states
static volatile uint16_t mEncoderSR_CH_A;
static volatile uint16_t mEncoderSR_CH_B;
static uint8_t           mEncoderRotationStates[NUM_ENCODERS];

// the current states of switches
static volatile sEncoderSwitches mEncoderSwitchStates;
static volatile sSideSwitches    mSideSwitchStates;

/**
 * @brief The init function for the input module.
 * Initialises the GPIO for the encoder shift registers
 * Initialises the timer peripheral used to generate an interrupt for input
 * scanning.
 */
void Input_Init(void) {
  for (int sidesw = 0; sidesw < NUM_SIDE_SWITCHES; sidesw++) {
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
  Timer_EnableChannelInterrupt(timerConfig.pTimer, TIMER_CHANNEL_A,
                               PRIORITY_MED);
  timerConfig.pTimer->CCA = (uint16_t)INPUT_SCAN_RATE;
}

/**
 * @brief The input update function - to be called in the main loop.
 *
 */
void Input_Update(void) {
  // Update encoder rotations
  for (uint8_t encoder = 0; encoder < NUM_ENCODERS; encoder++) {
    bool    chA = (bool)(mEncoderSR_CH_A & (1u << encoder));
    bool    chB = (bool)(mEncoderSR_CH_B & (1u << encoder));
    uint8_t val = (chB << 1) | chA;

    mEncoderRotationStates[encoder] =
        EncoderRotLUT[mEncoderRotationStates[encoder] & 0x0F][val];
  }

  // Debounce switches and set current states;
  uint16_t prevEncSwitchStates  = mEncoderSwitchStates.debounces_states;
  uint16_t prevSideSwitchStates = mSideSwitchStates.debounces_states;

  mEncoderSwitchStates.debounces_states = 0xFFFF;
  mSideSwitchStates.debounces_states    = 0xFF;

  for (uint8_t i = 0; i < DEBOUNCE_BUF_LEN; i++) {
    mEncoderSwitchStates.debounces_states &= mEncoderSwitchStates.Buffer[i];
    mSideSwitchStates.debounces_states &= mSideSwitchStates.Buffer[i];
  }

  mEncoderSwitchStates.raw_state =
      mEncoderSwitchStates.debounces_states ^ prevEncSwitchStates;
  mSideSwitchStates.raw_state =
      mSideSwitchStates.debounces_states ^ prevSideSwitchStates;
}

/**
 * @brief Check if the "RESET" input combination was pressed by the user
 *
 * @return true or false
 */
bool Input_IsResetPressed(void) {
  uint16_t state = mEncoderSwitchStates.debounces_states;
  return ((state & RESET_SWITCH_MASK) == RESET_SWITCH_VAL);
}

/**
 * @brief Checks special switch combinations (except "RESET").
 * Should be called during powerup.
 *
 */
void Input_CheckSpecialSwitchCombos(void) {
  uint16_t state = mEncoderSwitchStates.debounces_states;
  if (((state & BOOTLOADER_SWITCH_MASK) == BOOTLOADER_SWITCH_VAL) ||
      ((state & ALT_BOOTLOADER_SWITCH_MASK) == ALT_BOOTLOADER_SWITCH_VAL)) {
    gData.OperatingMode = BOOTLOADER_MODE;
  }

  if ((state & MIDI_MIRROR_SWITCH_MASK) == MIDI_MIRROR_SWITCH_VAL) {
    MIDI_MirrorInput(true);
  }
}

/**
 * @brief Get the current direction of rotation for a specific encoder.
 *
 * @param EncoderIndex The hardware index of the encoder.
 * @return uint8_t The direction - DIR_STATIONARY / DIR_CW (clockwise) / DIR_CCW
 * (counter clockwise)
 */
uint8_t Encoder_GetDirection(uint8_t EncoderIndex) {
  return mEncoderRotationStates[EncoderIndex] & 0x30;
}

/**
 * @brief Check if an encoder switch was pressed.
 * This can check all encoders, or a specific set by using a mask.
 * @param Mask - Can be used to mask which encoder to check
 * @return uint16_t A bitfield of the current encoder switch states.
 */
uint16_t EncoderSwitchWasPressed(uint16_t Mask) {
  return (mEncoderSwitchStates.raw_state &
          mEncoderSwitchStates.debounces_states) &
         Mask;
}

/**
 * @brief Check if an encoder switch was released.
 * This can check all encoders, or a specific set by using a mask.
 * @param Mask - Can be used to mask which encoder to check
 * @return uint16_t A bitfield of the current encoder switch states.
 */
uint16_t EncoderSwitchWasReleased(uint16_t Mask) {
  return (mEncoderSwitchStates.raw_state &
          (~mEncoderSwitchStates.debounces_states)) &
         Mask;
}

/**
 * @brief CGet the current encoder switch state.
 * This can check all encoders, or a specific set by using a mask.
 * @param Mask - Can be used to mask which encoder to check
 * @return uint16_t A bitfield of the current encoder switch states.
 */
uint16_t EncoderSwitchCurrentState(uint16_t Mask) {
  return mEncoderSwitchStates.debounces_states & Mask;
}

/**
 * @brief Check if a side switch was pressed.
 * This can check all side switches, or a specific set by using a mask.
 * @param Mask - Can be used to mask which side switch to check
 * @return uint8_t A bitfield of the current side switch states. Note - there
 * are only 6 side switches.
 */
uint8_t SideSwitchWasPressed(uint8_t Mask) {
  return (mSideSwitchStates.raw_state & mSideSwitchStates.debounces_states) &
         Mask;
}

/**
 * @brief Check if a side switch was released.
 * This can check all side switches, or a specific set by using a mask.
 * @param Mask - Can be used to mask which side switch to check
 * @return uint8_t A bitfield of the current side switch states. Note - there
 * are only 6 side switches.
 */
uint8_t SideSwitchWasReleased(uint8_t Mask) {
  return (mSideSwitchStates.raw_state & (~mSideSwitchStates.debounces_states)) &
         Mask;
}

/**
 * @brief Get the current state of a side switch.
 * This can check all side switches, or a specific set by using a mask.
 * @param Mask - Can be used to mask which side switch to check
 * @return uint8_t A bitfield of the current side switch states. Note - there
 * are only 6 side switches.
 */
uint8_t SideSwitchCurrentState(uint8_t Mask) {
  return mSideSwitchStates.debounces_states & Mask;
}

/**
 * @brief The update function for side switches.
 * This performs debouncing by averaging samples over time.
 */
static inline void UpdateSideSwitchStates(void) {
  mSideSwitchStates.Buffer[mSideSwitchStates.Index] = 0;
  for (int sidesw = 0; sidesw < NUM_SIDE_SWITCHES; sidesw++) {
    uint8_t state = (bool)GPIO_GetPinLevel(&SIDE_SWITCH_PORT, sidesw);
    SET_REG(mSideSwitchStates.Buffer[mSideSwitchStates.Index],
            (state << sidesw));
  }

  mSideSwitchStates.Index = (mSideSwitchStates.Index + 1) % DEBOUNCE_BUF_LEN;
}

/**
 * @brief The update function for encoder switches.
 * This performs debouncing by averaging samples over time.
 * This clocks the encoder shift registers to obtain the current state.
 */
static inline void UpdateEncoderSwitchStates(void) {
  mEncoderSwitchStates.Buffer[mEncoderSwitchStates.Index] = 0;
  for (int encodersw = 0; encodersw < NUM_ENCODER_SWITCHES; encodersw++) {
    GPIO_SetPinLevel(&ENCODER_PORT, PIN_ENCODER_CLK, LOW);
    uint16_t state = (bool)GPIO_GetPinLevel(&ENCODER_PORT, PIN_ENCODER_DATA_IN);
    SET_REG(mEncoderSwitchStates.Buffer[mEncoderSwitchStates.Index],
            (!state << encodersw));
    GPIO_SetPinLevel(&ENCODER_PORT, PIN_ENCODER_CLK, HIGH);
  }

  mEncoderSwitchStates.Index =
      (mEncoderSwitchStates.Index + 1) % DEBOUNCE_BUF_LEN;
}

/**
 * @brief The update function for encoder rotation.
 * This performs debouncing by averaging samples over time.
 * This clocks the encoder shift registers to obtain the current quadrature
 * states.
 */
static inline void UpdateEncoderQuadratureStates(void) {
  mEncoderSR_CH_A = mEncoderSR_CH_B = 0;
  for (int encoder = 0; encoder < NUM_ENCODERS; encoder++) {
    GPIO_SetPinLevel(&ENCODER_PORT, PIN_ENCODER_CLK, LOW);
    SET_REG(mEncoderSR_CH_A,
            GPIO_GetPinLevel(&ENCODER_PORT, PIN_ENCODER_DATA_IN)
                ? (1u << encoder)
                : 0);
    GPIO_SetPinLevel(&ENCODER_PORT, PIN_ENCODER_CLK, HIGH);

    GPIO_SetPinLevel(&ENCODER_PORT, PIN_ENCODER_CLK, LOW);
    SET_REG(mEncoderSR_CH_B,
            GPIO_GetPinLevel(&ENCODER_PORT, PIN_ENCODER_DATA_IN)
                ? (1u << encoder)
                : 0);
    GPIO_SetPinLevel(&ENCODER_PORT, PIN_ENCODER_CLK, HIGH);
  }
}

/**
 * @brief The timer interrupt for input scanning.
 * Latches the encoder shift registers to read data.
 * Calls input scanning/update functions for side switches, encoder switches,
 * encoder quadrature outputs.
 */
ISR(TCC1_CCA_vect) {
  INPUT_TIMER.CCA = INPUT_SCAN_RATE + INPUT_TIMER.CNT;
  GPIO_SetPinLevel(&ENCODER_PORT, PIN_ENCODER_LATCH, HIGH);
  UpdateSideSwitchStates();
  UpdateEncoderSwitchStates();
  UpdateEncoderQuadratureStates();
  GPIO_SetPinLevel(&ENCODER_PORT, PIN_ENCODER_LATCH, LOW);
}
