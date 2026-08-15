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
export const NUM_PWM_FRAMES = 32; // NUM_PWM_FRAMES in system/hardware.h - BCM brightness steps

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

// Mirrors mf_draw_encoder()'s PWM-brightness calculation for
// DIS_MODE_MULTI_PWM exactly (src/led/led.c), including its 32-bit
// intermediate scaling and the detent left-side inversion quirk. Returns
// 0-31 (NUM_PWM_FRAMES-1 max) - the leading LED's effective duty cycle
// within the current inter-LED span, i.e. how "filled in" it is between
// its neighbours rather than a plain on/off bar segment.
function pwmBrightness(position, ledIndex, isDetent) {
	let brightness;
	if (position === ENC_MAX) {
		brightness = NUM_PWM_FRAMES - 1;
	} else if (position > 0) {
		const scaledPos = Math.floor((position * NUM_INDICATOR_LEDS * 256) / ENC_MAX);
		const basePosForLed = (ledIndex - 1) * 256;
		const posInLed = scaledPos >= basePosForLed ? scaledPos - basePosForLed : 0;
		brightness = (posInLed * NUM_PWM_FRAMES) >> 8;
		if (brightness >= NUM_PWM_FRAMES) brightness = NUM_PWM_FRAMES - 1;
	} else {
		brightness = 0;
	}

	// Brightness inversion quirk for detent mode, left side (firmware's
	// own comment - "Apply brightness inversion quirk for detent mode,
	// left side"): mirrors the ramp so it fills toward the center on the
	// left half of a detent-mode ring, matching the center-out fill
	// direction CENTER_OUT_MASKS already gives the rest of that side.
	if (isDetent && ledIndex < CENTER_INDICATOR) {
		brightness = NUM_PWM_FRAMES - 1 - brightness;
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
 * Compute per-LED brightness (0-31, NUM_PWM_FRAMES-1 max) for all 11
 * indicator LEDs, matching mf_draw_encoder()'s per-frame BCM dimming
 * loop. For SINGLE/MULTI modes this is just each lit LED at full
 * brightness (31) and each unlit LED at 0 - the meaningful difference
 * from computeLitMask() only shows up in MULTI_PWM, where the leading
 * LED fades in smoothly rather than snapping on.
 *
 * @param {object} p same shape as computeLitMask()
 * @returns {number[]} length-11 array, index 0 = indicator 1, values 0-31
 */
export function computeLedBrightness({ position, displayMode, detent }) {
	const ledIndex = leadingLedIndex(position);
	const state = baseIndicatorState({ position, displayMode, detent });
	const litMask = unpackMask(state);

	if (displayMode !== DisplayMode.MULTI_PWM) {
		return litMask.map((lit) => (lit ? NUM_PWM_FRAMES - 1 : 0));
	}

	// Same "don't dim the center LED while it's substituting for the
	// leading LED in detent mode" exception mf_draw_encoder() applies
	// (see its is_detent && led_index == CENTER_INDICATOR branch).
	const dimTargetIndex = detent && ledIndex === CENTER_INDICATOR ? null : ledIndex;
	const leadingBrightness = dimTargetIndex === null ? NUM_PWM_FRAMES - 1 : pwmBrightness(position, ledIndex, detent);

	return litMask.map((lit, i) => {
		const oneBasedIndex = i + 1;
		if (!lit) return 0;
		if (oneBasedIndex === dimTargetIndex) return leadingBrightness;
		return NUM_PWM_FRAMES - 1;
	});
}

/**
 * Compute the detent red/blue LED's colour override for the center
 * indicator slot, or null when it shouldn't show. Firmware drives red
 * and blue as two independent BCM channels into the *same* physical LED
 * position as indicator 6 (see led-ring.js's `colorOverride` doc
 * comment) - only while detent is on and the knob is at dead centre
 * (mf_draw_encoder(): "Only show detent RB LEDs when at middle
 * position"), and only the white indicator is suppressed there (see
 * baseIndicatorState()'s CENTER_INDICATOR_MASK clear) to let it show.
 *
 * @param {object} p
 * @param {number} p.position
 * @param {boolean} p.detent
 * @param {{r: number, b: number}} [p.rb] - gamma-corrected BCM, 0-31 each (struct rb_8)
 * @returns {{index: number, color: string}|null} `index` is 0-based (4 = indicator 6, matching led-ring.js's litMask ordering)
 */
export function computeDetentColorOverride({ position, detent, rb }) {
	if (!detent || position !== ENC_MID || !rb) return null;
	const r = rb.r ?? 0;
	const b = rb.b ?? 0;
	if (r === 0 && b === 0) return null;

	const rFrac = Math.min(1, r / (NUM_PWM_FRAMES - 1));
	const bFrac = Math.min(1, b / (NUM_PWM_FRAMES - 1));
	// Additive-ish blend, matching two independent BCM channels driving
	// the same physical LED rather than a single hue - a real device
	// with both channels lit shows magenta/purple, not a 50/50 average.
	const red = Math.round(rFrac * 255);
	const blue = Math.round(bFrac * 255);
	const green = Math.round(Math.min(rFrac, bFrac) * 40); // slight lift when both channels are lit, avoids a muddy magenta reading as flat purple

	return {
		index: CENTER_INDICATOR - 1, // CENTER_INDICATOR is 1-based; litMask/led-ring.js are 0-based
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
