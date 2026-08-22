// Which of the 11 indicator LEDs are lit for a given position, display mode
// and detent state, mirroring mf_draw_encoder() in src/led/led.c bit-for-bit.
// The LUTs below are transcribed from its INDICATOR_MASKS/BAR_GRAPH_MASKS/
// CENTER_OUT_MASKS. Kept out of the renderer so it is testable without a DOM.
//
// Firmware packs the 11 LEDs into bits 15..5 of a u16; this module works in
// booleans (index 0 = indicator 1) and only touches the bitfield where it is
// transcribing a LUT directly.

export const NUM_INDICATOR_LEDS = 11;
export const CENTER_INDICATOR = 6; // 1-based, matches firmware's CENTER_INDICATOR
export const ENC_MAX = 255; // ENC_MAX in io/encoder.h
export const ENC_MID = 127; // ENC_MID = ENC_MAX/2, integer division
export const NUM_BCM_PLANES = 8; // NUM_BCM_PLANES in system/hardware.h
export const MAX_BRIGHTNESS = 255; // MAX_BRIGHTNESS - 8 binary-weighted planes

// Transcribed from gamma_lut[] in src/led/color.c (gamma 2.2, 0-255).
// The firmware feeds the leading LED's sub-position through this before it
// becomes a BCM duty cycle, so the preview must too or the ramp looks linear.
const GAMMA_LUT = [
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,
  2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7,
  8, 8, 8, 9, 9, 9, 10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16,
  16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 22, 22, 23, 23, 24, 25, 25, 26, 26,
  27, 28, 28, 29, 30, 30, 31, 32, 33, 33, 34, 35, 35, 36, 37, 38, 39, 39, 40,
  41, 42, 43, 43, 44, 45, 46, 47, 48, 49, 49, 50, 51, 52, 53, 54, 55, 56, 57,
  58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 73, 74, 75, 76, 77,
  78, 79, 81, 82, 83, 84, 85, 87, 88, 89, 90, 91, 93, 94, 95, 97, 98, 99, 100,
  102, 103, 105, 106, 107, 109, 110, 111, 113, 114, 116, 117, 119, 120, 121,
  123, 124, 126, 127, 129, 130, 132, 133, 135, 137, 138, 140, 141, 143, 145,
  146, 148, 149, 151, 153, 154, 156, 158, 159, 161, 163, 165, 166, 168, 170,
  172, 173, 175, 177, 179, 181, 182, 184, 186, 188, 190, 192, 194, 196, 197,
  199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221, 223, 225, 227,
  229, 231, 234, 236, 238, 240, 242, 244, 246, 248, 251, 253, 255,
];

// enum display_mode (system/hardware.h)
export const DisplayMode = Object.freeze({ SINGLE: 0, MULTI: 1, MULTI_PWM: 2 });

function maskBit(n) {
  // INDICATOR_MASK(n) = 0x8000 >> (n-1), n is 1-based.
  return 0x8000 >> (n - 1);
}

const INDICATOR_MASKS = [
  0,
  ...Array.from({ length: NUM_INDICATOR_LEDS }, (_, i) => maskBit(i + 1)),
];

const BAR_GRAPH_MASKS = [
  0x0000, 0x8000, 0xc000, 0xe000, 0xf000, 0xf800, 0xfc00, 0xfe00, 0xff00,
  0xff80, 0xffc0, 0xffe0,
];

const CENTER_OUT_MASKS = [
  0x0000, 0xf800, 0x7800, 0x3800, 0x1800, 0x0800, 0x0000, 0x0200, 0x0300,
  0x0380, 0x03c0, 0x03e0,
];

const CENTER_INDICATOR_MASK = maskBit(CENTER_INDICATOR);

function leadingLedIndex(currentPos) {
  if (currentPos === 0) return 1;
  if (currentPos >= ENC_MAX) return NUM_INDICATOR_LEDS;
  let ledIndex = Math.floor(
    (currentPos * NUM_INDICATOR_LEDS + ENC_MAX - 1) / ENC_MAX
  );
  if (ledIndex < 1) ledIndex = 1;
  if (ledIndex > NUM_INDICATOR_LEDS) ledIndex = NUM_INDICATOR_LEDS;
  return ledIndex;
}

/** Unpack a firmware-shaped u16 indicator bitmask into an 11-element
 * boolean array, index 0 = indicator 1 (leftmost), matching led-ring.js's
 * `litMask` ordering (LED 0 = first drawn, at -arcSpan/2). */
function unpackMask(bits) {
  return Array.from({ length: NUM_INDICATOR_LEDS }, (_, i) =>
    Boolean(bits & INDICATOR_MASKS[i + 1])
  );
}

// Mirrors mf_draw_encoder()'s MULTI_PWM brightness calculation, including the
// gamma weighting and the detent left-side inversion. Returns the leading
// LED's duty cycle (0-255) within the current inter-LED span.
function pwmBrightness(position, ledIndex, isDetent) {
  let brightness;
  if (position === ENC_MAX) {
    brightness = MAX_BRIGHTNESS;
  } else if (position > 0) {
    const scaledPos = Math.floor(
      (position * NUM_INDICATOR_LEDS * 256) / ENC_MAX
    );
    const basePosForLed = (ledIndex - 1) * 256;
    const posInLed = scaledPos >= basePosForLed ? scaledPos - basePosForLed : 0;
    brightness = GAMMA_LUT[Math.min(posInLed, 255)];
  } else {
    brightness = 0;
  }

  // Firmware's detent left-side inversion: mirrors the ramp so it fills
  // toward the centre, matching CENTER_OUT_MASKS on that side.
  if (isDetent && ledIndex < CENTER_INDICATOR) {
    brightness = MAX_BRIGHTNESS - brightness;
  }

  return brightness;
}

/**
 * Compute the lit/unlit state of all 11 indicator LEDs for one encoder,
 * matching mf_draw_encoder()'s base_indicator_state (the steady-state
 * bit pattern only - not per-LED brightness; see computeLedBrightness()
 * for the DIS_MODE_MULTI_PWM leading-LED fade).
 *
 * @param {object} p
 * @param {number} p.position - 0-255, current encoder position (vmap.curr_pos)
 * @param {number} p.displayMode - DisplayMode.SINGLE/MULTI/MULTI_PWM
 * @param {boolean} p.detent
 * @returns {boolean[]} length-11 array, index 0 = indicator 1
 */
export function computeLitMask({ position, displayMode, detent }) {
  return unpackMask(baseIndicatorState({ position, displayMode, detent }));
}

/**
 * Compute per-LED brightness (0-255, MAX_BRIGHTNESS max) for all 11
 * indicator LEDs, matching mf_draw_encoder()'s per-frame BCM dimming
 * loop. For SINGLE/MULTI modes this is just each lit LED at full
 * brightness (255) and each unlit LED at 0 - the meaningful difference
 * from computeLitMask() only shows up in MULTI_PWM, where the leading
 * LED fades in smoothly rather than snapping on.
 *
 * @param {object} p same shape as computeLitMask()
 * @returns {number[]} length-11 array, index 0 = indicator 1, values 0-255
 */
export function computeLedBrightness({ position, displayMode, detent }) {
  const ledIndex = leadingLedIndex(position);
  const state = baseIndicatorState({ position, displayMode, detent });
  const litMask = unpackMask(state);

  if (displayMode !== DisplayMode.MULTI_PWM) {
    return litMask.map((lit) => (lit ? MAX_BRIGHTNESS : 0));
  }

  // mf_draw_encoder() does not dim the centre LED while it substitutes for
  // the leading one in detent mode.
  const dimTargetIndex =
    detent && ledIndex === CENTER_INDICATOR ? null : ledIndex;
  const leadingBrightness =
    dimTargetIndex === null
      ? MAX_BRIGHTNESS
      : pwmBrightness(position, ledIndex, detent);

  return litMask.map((lit, i) => {
    const oneBasedIndex = i + 1;
    if (!lit) return 0;
    if (oneBasedIndex === dimTargetIndex) return leadingBrightness;
    return MAX_BRIGHTNESS;
  });
}

// Detent colour for the centre indicator slot, or null when it should not
// show. Firmware drives red and blue as two BCM channels into the same
// physical LED as indicator 6, lit only at dead centre with detent on.
export function computeDetentColorOverride({ position, detent, rb }) {
  if (!detent || position !== ENC_MID || !rb) return null;
  const r = rb.r ?? 0;
  const b = rb.b ?? 0;
  if (r === 0 && b === 0) return null;

  const rFrac = Math.min(1, r / MAX_BRIGHTNESS);
  const bFrac = Math.min(1, b / MAX_BRIGHTNESS);
  // Additive blend: two BCM channels on one LED read as magenta when both
  // are lit, not as a 50/50 average.
  const red = Math.round(rFrac * 255);
  const blue = Math.round(bFrac * 255);
  const green = Math.round(Math.min(rFrac, bFrac) * 40);

  return {
    index: CENTER_INDICATOR - 1,
    color: `rgb(${red}, ${green}, ${blue})`,
  };
}

function baseIndicatorState({ position, displayMode, detent }) {
  const ledIndex = leadingLedIndex(position);
  const isAtMid = position === ENC_MID;

  let state;
  switch (displayMode) {
    case DisplayMode.SINGLE:
      state = INDICATOR_MASKS[ledIndex];
      break;
    case DisplayMode.MULTI:
    case DisplayMode.MULTI_PWM:
      state = detent ? CENTER_OUT_MASKS[ledIndex] : BAR_GRAPH_MASKS[ledIndex];
      break;
    default:
      state = 0;
  }

  if (detent) {
    if (!isAtMid) {
      state |= CENTER_INDICATOR_MASK;
    } else {
      state &= ~CENTER_INDICATOR_MASK;
    }
  }

  return state;
}
