// color.js - HSV <-> RGB conversion matching the firmware's color model.
//
// The firmware (src/led/hsv2rgb.c, src/led/color.c) uses a non-standard hue
// range of 0-1535 (6 sextants x 256 steps, not the usual 0-360) with
// saturation/value in 0-255. Its actual conversion is a fixed-point,
// pointer-swapping "sextant" algorithm optimized for AVR - see
// fast_hsv2rgb_8bit() in hsv2rgb.c for the byte-exact original, which is
// what the firmware itself uses when it renders these values to hardware.
//
// This module is NOT a byte-exact port of that fixed-point algorithm - it's
// a standard HSV->RGB formula remapped onto the same 0-1535/0-255/0-255
// input range, used only to render an accurate-looking preview swatch in
// the browser. The device is always the source of truth for what actually
// lights up; this preview may differ by at most a rounding step from the
// firmware's own gamma-corrected BCM output.

export const HUE_MAX = 1535; // 6 sextants x 256 - 1
export const SAT_MAX = 255;
export const VAL_MAX = 255;

/**
 * Convert firmware-range HSV (hue 0-1535, sat 0-255, val 0-255) to sRGB
 * 0-255 per channel, for CSS/canvas display.
 * @param {number} hue 0-1535
 * @param {number} sat 0-255
 * @param {number} val 0-255
 * @returns {{r:number, g:number, b:number}}
 */
export function hsvToRgb(hue, sat, val) {
	hue = clamp(hue, 0, HUE_MAX);
	sat = clamp(sat, 0, SAT_MAX);
	val = clamp(val, 0, VAL_MAX);

	if (sat === 0) {
		return { r: val, g: val, b: val };
	}

	// Remap the firmware's 0-1535 hue range to the standard 0-6 sextant
	// float used by the textbook HSV formula, then to 0-360 degrees.
	const h360 = (hue / (HUE_MAX + 1)) * 360;
	const s01 = sat / SAT_MAX;
	const v01 = val / VAL_MAX;

	const c = v01 * s01;
	const hPrime = h360 / 60;
	const x = c * (1 - Math.abs((hPrime % 2) - 1));
	const m = v01 - c;

	let r1 = 0, g1 = 0, b1 = 0;
	if (hPrime >= 0 && hPrime < 1) { r1 = c; g1 = x; b1 = 0; }
	else if (hPrime < 2) { r1 = x; g1 = c; b1 = 0; }
	else if (hPrime < 3) { r1 = 0; g1 = c; b1 = x; }
	else if (hPrime < 4) { r1 = 0; g1 = x; b1 = c; }
	else if (hPrime < 5) { r1 = x; g1 = 0; b1 = c; }
	else { r1 = c; g1 = 0; b1 = x; }

	return {
		r: Math.round((r1 + m) * 255),
		g: Math.round((g1 + m) * 255),
		b: Math.round((b1 + m) * 255),
	};
}

/**
 * @returns {string} a CSS rgb() color string for the given firmware-range HSV.
 */
export function hsvToCss(hue, sat, val) {
	const { r, g, b } = hsvToRgb(hue, sat, val);
	return `rgb(${r}, ${g}, ${b})`;
}

/**
 * Convert sRGB 0-255 per channel to firmware-range HSV (hue 0-1535, sat
 * 0-255, val 0-255). Used only for e.g. importing a preset that stored a
 * CSS color; the device itself is never sent RGB, only HSV (see
 * MF_SYSEX_PARAM_VMAP_RGB, which is a *read-only* gamma-corrected BCM
 * mirror of the HSV value, not an independently settable color).
 */
export function rgbToHsv(r, g, b) {
	r = clamp(r, 0, 255) / 255;
	g = clamp(g, 0, 255) / 255;
	b = clamp(b, 0, 255) / 255;

	const max = Math.max(r, g, b);
	const min = Math.min(r, g, b);
	const delta = max - min;

	let h360 = 0;
	if (delta !== 0) {
		if (max === r) h360 = 60 * (((g - b) / delta) % 6);
		else if (max === g) h360 = 60 * ((b - r) / delta + 2);
		else h360 = 60 * ((r - g) / delta + 4);
	}
	if (h360 < 0) h360 += 360;

	const sat = max === 0 ? 0 : delta / max;
	const val = max;

	return {
		hue: Math.round((h360 / 360) * (HUE_MAX + 1)) % (HUE_MAX + 1),
		sat: Math.round(sat * SAT_MAX),
		val: Math.round(val * VAL_MAX),
	};
}

function clamp(v, lo, hi) {
	return Math.min(hi, Math.max(lo, v));
}
