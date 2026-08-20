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

export function hsvToCss(hue, sat, val) {
	const { r, g, b } = hsvToRgb(hue, sat, val);
	return `rgb(${r}, ${g}, ${b})`;
}

function clamp(v, lo, hi) {
	return Math.min(hi, Math.max(lo, v));
}
