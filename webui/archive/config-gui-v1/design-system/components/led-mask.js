// led-mask.js - computes which of the 11 indicator LEDs are lit for a
// given encoder position/display mode/detent state, mirroring the
// firmware's mf_draw_encoder() (src/led/led.c) bit-for-bit (the LUTs
// below are transcribed directly from its INDICATOR_MASKS/
// BAR_GRAPH_MASKS/CENTER_OUT_MASKS). This is the bridge between real
// device/model state and led-ring.js's `litMask` prop - kept separate
// from rendering so it can be unit-tested and reused without a DOM.
//
// Firmware represents the 11 LEDs as bits 15..5 of a u16 (bit 15 =
// indicator 1, bit 5 = indicator 11); this module works in a plainer
// "array of 11 booleans, index 0 = indicator 1" shape instead and only
// deals in that bitfield where directly transcribing a LUT.

export const NUM_INDICATOR_LEDS = 11;
export const CENTER_INDICATOR = 6; // 1-based, matches firmware's CENTER_INDICATOR
export const ENC_MAX = 255; // ENC_MAX in io/encoder.h
export const ENC_MID = 127; // ENC_MID = ENC_MAX/2, integer division

// enum display_mode (system/hardware.h) - re-exported here so callers
// don't need to reach into device-model.js just for this.
export const DisplayMode = Object.freeze({ SINGLE: 0, MULTI: 1, MULTI_PWM: 2 });

function maskBit(n) {
	// INDICATOR_MASK(n) = 0x8000 >> (n-1), n is 1-based.
	return 0x8000 >> (n - 1);
}

const INDICATOR_MASKS = [0, ...Array.from({ length: NUM_INDICATOR_LEDS }, (_, i) => maskBit(i + 1))];

const BAR_GRAPH_MASKS = [
	0x0000, 0x8000, 0xc000, 0xe000, 0xf000, 0xf800, 0xfc00, 0xfe00, 0xff00, 0xff80, 0xffc0, 0xffe0,
];

const CENTER_OUT_MASKS = [
	0x0000, 0xf800, 0x7800, 0x3800, 0x1800, 0x0800, 0x0000, 0x0200, 0x0300, 0x0380, 0x03c0, 0x03e0,
];

const CENTER_INDICATOR_MASK = maskBit(CENTER_INDICATOR);

function leadingLedIndex(currentPos) {
	if (currentPos === 0) return 1;
	if (currentPos >= ENC_MAX) return NUM_INDICATOR_LEDS;
	// Ceiling division, matches the firmware's integer math exactly.
	let ledIndex = Math.floor((currentPos * NUM_INDICATOR_LEDS + ENC_MAX - 1) / ENC_MAX);
	if (ledIndex < 1) ledIndex = 1;
	if (ledIndex > NUM_INDICATOR_LEDS) ledIndex = NUM_INDICATOR_LEDS;
	return ledIndex;
}

/** Unpack a firmware-shaped u16 indicator bitmask into an 11-element
 * boolean array, index 0 = indicator 1 (leftmost), matching led-ring.js's
 * `litMask` ordering (LED 0 = first drawn, at -arcSpan/2). */
function unpackMask(bits) {
	return Array.from({ length: NUM_INDICATOR_LEDS }, (_, i) => Boolean(bits & INDICATOR_MASKS[i + 1]));
}

/**
 * Compute the lit/unlit state of all 11 indicator LEDs for one encoder,
 * matching mf_draw_encoder()'s base_indicator_state (the steady-state
 * pattern - this does not model the PWM/BCM sub-frame dimming of
 * DIS_MODE_MULTI_PWM's leading LED, which is a hardware brightness-frame
 * detail with no meaningful analogue at browser refresh rates; the
 * leading LED is rendered fully on instead).
 *
 * @param {object} p
 * @param {number} p.position - 0-255, current encoder position (vmap.curr_pos)
 * @param {number} p.displayMode - DisplayMode.SINGLE/MULTI/MULTI_PWM
 * @param {boolean} p.detent
 * @returns {boolean[]} length-11 array, index 0 = indicator 1
 */
export function computeLitMask({ position, displayMode, detent }) {
	const ledIndex = leadingLedIndex(position);
	const isAtMid = position === ENC_MID;

	let state;
	switch (displayMode) {
		case DisplayMode.SINGLE:
			state = INDICATOR_MASKS[ledIndex];
			break;
		case DisplayMode.MULTI:
		case DisplayMode.MULTI_PWM:
			// PWM sub-frame dimming intentionally not modelled - see doc comment.
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

	return unpackMask(state);
}
