/*
 * File: EncoderDisplay.c ( 25th November 2021 )
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

#include <math.h>
#include <string.h>

#include "system/Data.h"
#include "system/types.h"
#include "Display/Display.h"
#include "Input/Encoder.h"
#include "Display/EncoderDisplay.h"
#include "system/HardwareDescription.h"
#include "Input/Input.h"
#include "Display/fast_hsv2rgb.h"

// Converts an integer range to a number of indicator LEDs
#define INDVAL_2_INDCOUNT(x)                                                   \
  (((x) / ENCODER_MAX_VAL) *                                                   \
   NUM_INDICATOR_LEDS) // FIXME - there is no division hardware, consider fixed
                       // point multiplication.
#define SET_FRAME(f, x)   ((f) &= ~(x))
#define UNSET_FRAME(f, x) ((f) |= (x))
#define PWM_CHECK(f, br)                                                       \
  ((f * MAGIC_BRIGHTNESS_VAL) < (br)) // Check if this frame needs to be
                                      // rendered based on a brightness value.

/**
 * @brief Renders a single display frame for RGB leds only.
 *
 * @param pFrame A pointer to a display frame to store the rendered frame data.
 * @param FrameIndex The frame index (within its parent frame buffer)
 * @param pEncoder A pointer to the encode state information.
 */
static inline void
RenderFrame_RGB(DisplayFrame* pFrame, int FrameIndex,
                sEncoderState* pEncoder) // FIXME - should this be inline?
{
  float brightnessCoeff =
      gData.RGBBrightness /
      (float)BRIGHTNESS_MAX; // FIXME: floating point division! this is
                             // expensive, replace with something else!

  if (PWM_CHECK(FrameIndex, pEncoder->RGBColour.Red * brightnessCoeff)) {
    SET_FRAME(*pFrame, LEDMASK(LED_RGB_RED));
  }

  if (PWM_CHECK(FrameIndex, pEncoder->RGBColour.Green * brightnessCoeff)) {
    SET_FRAME(*pFrame, LEDMASK(LED_RGB_GREEN));
  }

  if (PWM_CHECK(FrameIndex, pEncoder->RGBColour.Blue * brightnessCoeff)) {
    SET_FRAME(*pFrame, LEDMASK(LED_RGB_BLUE));
  }
}

/**
 * @brief Renders a single frame for the detent led.
 *
 * @param pFrame A pointer to a display frame to store the rendered frame data.
 * @param FrameIndex The frame index (within its parent frame buffer)
 * @param pEncoder A pointer to the encode state information.
 */
static inline void
RenderFrame_Detent(DisplayFrame* pFrame, int FrameIndex,
                   sEncoderState* pEncoder) // FIXME - should this be inline?
{
  if (PWM_CHECK(FrameIndex, gData.DetentBrightness)) {
    if (PWM_CHECK(FrameIndex, pEncoder->DetentColour.Red)) {
      SET_FRAME(*pFrame, LEDMASK(LED_DET_RED));
    }

    if (PWM_CHECK(FrameIndex, pEncoder->DetentColour.Blue)) {
      SET_FRAME(*pFrame, LEDMASK(LED_DET_BLUE));
    }
  }

  // There is a white indicator LED in the same position as the detent LEDs,
  // unset it.
  UNSET_FRAME(*pFrame, LEDMASK(LED_IND_6));
}

/**
 * @brief Render a test frame for a specific encoder.
 *
 * @param EncoderIndex The encoder to test.
 */
static inline void
Render_Test(int EncoderIndex) // FIXME - should this be inline?
{
  DisplayFrame frames[DISPLAY_BUFFER_SIZE] = {0};

  for (int frame = 0; frame < DISPLAY_BUFFER_SIZE; frame++) {
    frames[frame] = LEDS_OFF; // set all LEDs off to start
    SET_FRAME(frames[frame], LEDMASK(EncoderIndex));
  }

  Display_SetEncoderFrames(EncoderIndex, &frames[0]);
}

/**
 * @brief Render all frames for an encoder - renders in the Dot display style.
 *
 * @param pEncoder A pointer to the encode state information.
 * @param EncoderIndex The encoder index.
 */
static inline void
RenderEncoder_Dot(sEncoderState* pEncoder,
                  int            EncoderIndex) // FIXME - should this be inline?
{
  float indicatorCountFloat =
      (pEncoder->CurrentValue / (float)ENCODER_MAX_VAL) *
      NUM_INDICATOR_LEDS; // FIXME: floating point division! this is expensive,
                          // replace with something else!
  uint8_t indicatorCountInt = ceilf(indicatorCountFloat);

  // Always draw the first indicator if in detent mode, the last one is always
  // drawn based on above calculations.
  if (indicatorCountInt == 0 && pEncoder->HasDetent) {
    indicatorCountInt = 1;
  }

  DisplayFrame frames[DISPLAY_BUFFER_SIZE] = {0};

  for (int frame = 0; frame < DISPLAY_BUFFER_SIZE; frame++) {
    frames[frame] = LEDS_OFF; // set all LEDs off to start

    RenderFrame_RGB(&frames[frame], frame, pEncoder); // Render the RGB segment

    if (PWM_CHECK(frame,
                  gData.IndicatorBrightness)) // Render the indicator LEDs
    {
      SET_FRAME(frames[frame], LEDMASK(NUM_ENCODER_LEDS - indicatorCountInt));
    }

    if (pEncoder->HasDetent && indicatorCountInt == 6) {
      RenderFrame_Detent(&frames[frame], frame, pEncoder);
    }
  }

  Display_SetEncoderFrames(EncoderIndex, &frames[0]);
}

/**
 * @brief Render all frames for an encoder - renders in the Bar display style.
 *
 * @param pEncoder A pointer to the encode state information.
 * @param EncoderIndex The encoder index.
 */
static inline void
RenderEncoder_Bar(sEncoderState* pEncoder,
                  int            EncoderIndex) // FIXME - should this be inline?
{
  float indicatorCountFloat =
      (pEncoder->CurrentValue / (float)ENCODER_MAX_VAL) *
      NUM_INDICATOR_LEDS; // FIXME: floating point division! this is expensive,
                          // replace with something else!
  uint8_t indicatorCountInt = ceilf(indicatorCountFloat);
  bool    drawDetent        = (indicatorCountInt == 6);
  bool    clearLeft         = (indicatorCountInt >= 6);
  bool    reverse           = (!clearLeft);

  if (reverse) {
    indicatorCountInt -= 1;
  }

  DisplayFrame frames[DISPLAY_BUFFER_SIZE] = {0};

  for (int frame = 0; frame < DISPLAY_BUFFER_SIZE; frame++) {
    frames[frame] = LEDS_OFF;

    RenderFrame_RGB(&frames[frame], frame, pEncoder);

    if (PWM_CHECK(frame, gData.IndicatorBrightness)) {
      SET_FRAME(frames[frame], LEDMASK_IND & ~(LEDS_OFF >> indicatorCountInt));
    }

    if (pEncoder->HasDetent) {
      if (reverse) {
        if (PWM_CHECK(frame, gData.IndicatorBrightness)) {
          frames[frame] ^= 0xFC00;
        } else {
          UNSET_FRAME(frames[frame], LEDMASK_IND);
        }
      } else if (clearLeft) {
        UNSET_FRAME(frames[frame], 0xF800);
      }

      if (drawDetent) {
        RenderFrame_Detent(&frames[frame], frame, pEncoder);
      }
    }
  }

  Display_SetEncoderFrames(EncoderIndex, &frames[0]);
}

/**
 * @brief Render all frames for an encoder - renders in the Blended Bar display
 * style.
 *
 * @param pEncoder A pointer to the encode state information.
 * @param EncoderIndex The encoder index.
 */
static inline void
RenderEncoder_BlendedBar(sEncoderState* pEncoder,
                         int EncoderIndex) // FIXME - should this be inline?
{
  float indicatorCountFloat =
      (pEncoder->CurrentValue / (float)ENCODER_MAX_VAL) *
      NUM_INDICATOR_LEDS; // FIXME: floating point division! this is expensive,
                          // replace with something else!
  uint8_t indicatorCountInt = floorf(indicatorCountFloat);
  uint8_t partialIndicatorBrightness =
      (uint8_t)((indicatorCountFloat - indicatorCountInt) * BRIGHTNESS_MAX);
  uint8_t partialIndicatorIndex = NUM_ENCODER_LEDS - indicatorCountInt - 1;
  bool    drawDetent            = (ceilf(indicatorCountFloat) == 6);
  bool    clearLeft             = (indicatorCountInt > 5);
  bool    reverse               = (!clearLeft);

  DisplayFrame frames[DISPLAY_BUFFER_SIZE] = {0};

  for (int frame = 0; frame < DISPLAY_BUFFER_SIZE; frame++) {
    frames[frame] = LEDS_OFF;

    RenderFrame_RGB(&frames[frame], frame, pEncoder);

    if (PWM_CHECK(frame, gData.IndicatorBrightness)) {
      SET_FRAME(frames[frame], LEDMASK_IND & ~(LEDS_OFF >> indicatorCountInt));

      if (PWM_CHECK(frame, partialIndicatorBrightness)) {
        SET_FRAME(frames[frame], LEDMASK(partialIndicatorIndex));
      }
    }

    if (pEncoder->HasDetent) {
      if (reverse) {
        if (PWM_CHECK(frame, gData.IndicatorBrightness)) {
          frames[frame] ^= 0xFC00;
        } else {
          UNSET_FRAME(frames[frame], LEDMASK_IND);
        }
      } else if (clearLeft) {
        UNSET_FRAME(frames[frame], 0xF800);
      }

      if (drawDetent) {
        RenderFrame_Detent(&frames[frame], frame, pEncoder);
      }
    }
  }

  Display_SetEncoderFrames(EncoderIndex, &frames[0]);
}

/**
 * @brief A display test function.
 */
void EncoderDisplay_Test(void) {
  for (int encoder = 0; encoder < NUM_ENCODERS; encoder++) {
    DisplayFrame frames[DISPLAY_BUFFER_SIZE];

    if ((bool)EncoderSwitchCurrentState(SWITCH_MASK(encoder))) {
      memset(frames, LED_ON, sizeof(frames));
    } else {
      memset(frames, LED_OFF, sizeof(frames));
    }

    Display_SetEncoderFrames(encoder, frames);
  }
}

/**
 * @brief Render a complete display buffer of frames for a specific encoder.
 *
 * @param pEncoder A pointer to the encoder state to render.
 * @param EncoderIndex The encoder index.
 */
void EncoderDisplay_Render(sEncoderState* pEncoder, int EncoderIndex) {
  if (pEncoder->DisplayInvalid == false) {
    return; // nothing to do so return
  }

  switch ((eEncoderDisplayStyle)pEncoder->DisplayStyle) {
  case STYLE_DOT: RenderEncoder_Dot(pEncoder, EncoderIndex); break;

  case STYLE_BAR: RenderEncoder_Bar(pEncoder, EncoderIndex); break;

  case STYLE_BLENDED_BAR:
    RenderEncoder_BlendedBar(pEncoder, EncoderIndex);
    break;

  default: break;
  }

  pEncoder->DisplayInvalid = false;
}

/**
 * @brief Invalidate all encoder displays - they will all be re-rendered on the
 * next main loop.
 *
 */
void EncoderDisplay_InvalidateAll(void) {
  for (int bank = 0; bank < NUM_VIRTUAL_BANKS; bank++) {
    for (int encoder = 0; encoder < NUM_ENCODERS; encoder++) {
      gData.EncoderStates[bank][encoder].DisplayInvalid = true;
    }
  }
}

/**
 * @brief Set the RGB LED colour for an encoder based on HSV colour.
 *
 * @param pEncoder A pointer to the encoder state.
 * @param pNewColour A pointer to the new colour to set to.
 */
void EncoderDisplay_SetRGBColour(sEncoderState* pEncoder, sHSV* pNewColour) {
  fast_hsv2rgb_8bit(pNewColour->Hue, pNewColour->Saturation, pNewColour->Value,
                    &pEncoder->RGBColour.Red, &pEncoder->RGBColour.Green,
                    &pEncoder->RGBColour.Blue);
}

/**
 * @brief Set the RGB LED colour for an encoder based on an HSV hue.
 *
 * @param pEncoder A pointer to the encoder state.
 * @param HSVHue The new hue to set to.
 */
void EncoderDisplay_SetRGBColour_Hue(sEncoderState* pEncoder, uint16_t HSVHue) {
  Hue2RGB(HSVHue, &pEncoder->RGBColour);
}

/**
 * @brief Set the detent LED colour for an encoder based on an HSV colour.
 *
 * @param pEncoder A pointer to the encoder state.
 * @param pNewColour A pointer to the new colour to set to.
 */
void EncoderDisplay_SetDetentColour(sEncoderState* pEncoder, sHSV* pNewColour) {
  fast_hsv2rgb_8bit(pNewColour->Hue, pNewColour->Saturation, pNewColour->Value,
                    &pEncoder->DetentColour.Red, &pEncoder->DetentColour.Green,
                    &pEncoder->DetentColour.Blue);
}

/**
 * @brief Set the detent LED colour for an encoder based on a HSV hue.
 *
 * @param pEncoder A pointer to the encoder state.
 * @param HSVHue The new hue to set to.
 */
void EncoderDisplay_SetDetentColour_Hue(sEncoderState* pEncoder,
                                        uint16_t       HSVHue) {
  Hue2RGB(HSVHue, &pEncoder->DetentColour);
}

/**
 * @brief Sets the RGB and Detent LED colour structs for all encoders to the
 * corresponding Hue values in the encoder states in RAM.
 *
 */
void EncoderDisplay_UpdateAllColours(void) {
  for (int bank = 0; bank < NUM_VIRTUAL_BANKS; bank++) {
    for (int encoder = 0; encoder < NUM_ENCODERS; encoder++) {
      sEncoderState* pEncoder = &gData.EncoderStates[bank][encoder];

      EncoderDisplay_SetRGBColour_Hue(pEncoder,
                                      pEncoder->Layers[VIRTUAL_LAYER_A].RGBHue);
      EncoderDisplay_SetDetentColour_Hue(pEncoder, pEncoder->DetentHue);

      pEncoder->DisplayInvalid = true;
    }
  }
}

/**
 * @brief Sets an encoders' indicator value (which is its current value).
 *
 * @param EncoderIndex The index of the encoder to update.
 * @param Value The new encoder value - automatic conversion from an 8-bit
 * value.
 */
void EncoderDisplay_SetIndicatorValueU8(uint8_t EncoderIndex, uint8_t Value) {
  gData.EncoderStates[gData.CurrentBank][EncoderIndex].CurrentValue =
      (Value << 8);
  gData.EncoderStates[gData.CurrentBank][EncoderIndex].DisplayInvalid = true;
}

/**
 * @brief Sets an encoders' indicator value (which is its current value).
 *
 * @param EncoderIndex The index of the encoder to update.
 * @param Value The new encoder value.
 */
void EncoderDisplay_SetIndicatorValueU16(uint8_t EncoderIndex, uint16_t Value) {
  gData.EncoderStates[gData.CurrentBank][EncoderIndex].CurrentValue   = Value;
  gData.EncoderStates[gData.CurrentBank][EncoderIndex].DisplayInvalid = true;
}
