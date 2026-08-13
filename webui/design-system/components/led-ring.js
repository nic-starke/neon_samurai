// led-ring.js - the ring of discrete indicator LEDs around an encoder
// (11 on real hardware, NUM_INDICATOR_LEDS in system/hardware.h). Ported
// from Encoder.dc.html's indicator-LED loop.
//
// The LEDs sit at fixed angular positions, not a continuous arc - `count`
// LEDs spread evenly across `arcSpan` degrees, centred on top. "Lit" here
// is driven by a simple value/max ratio when no explicit `litMask` prop
// is given; for a firmware-accurate pattern (bar graph / center-out
// detent / etc, matching mf_draw_encoder() in src/led/led.c) compute one
// with led-mask.js's computeLitMask() and pass it as `litMask` instead -
// see webui/js/ui.js's encoder grid for that usage.

import { elc } from "./dom.js";

/**
 * @param {object} p
 * @param {number} [p.count] - number of indicator LEDs (11 on real hardware)
 * @param {number} [p.radius] - ring radius, px
 * @param {number} [p.size] - individual LED diameter, px
 * @param {number} [p.arcSpan] - total arc the ring covers, degrees
 * @param {number} [p.value] - current value, used with `max` to compute a lit count if `litMask` isn't given
 * @param {number} [p.max]
 * @param {boolean[]} [p.litMask] - explicit per-LED on/off state (index 0 = first LED clockwise from arc start); overrides value/max when given, for callers driving real device/firmware indicator patterns rather than a simple fill
 */
export function buildLedRing(p) {
	const count = p.count ?? 11;
	const radius = p.radius ?? 37;
	const size = p.size ?? 9;
	const span = p.arcSpan ?? 270;
	const value = p.value ?? 31;
	const max = p.max ?? 127;
	const lit = Math.round(count * ((value || 0) / (max || 1)));
	const litMask = p.litMask ?? Array.from({ length: count }, (_, i) => i < lit);

	const frag = document.createDocumentFragment();
	for (let s = 0; s < count; s++) {
		const angle = -(span / 2) + (span / (count - 1)) * s;
		const litSeg = Boolean(litMask[s]);
		frag.appendChild(
			elc("div", {
				style:
					`position:absolute; top:50%; left:50%; width:${size}px; height:${size}px; border-radius:50%; ` +
					`background:${litSeg ? "var(--ds-led-on)" : "var(--ds-led-off)"}; ` +
					`box-shadow:${litSeg ? "var(--ds-glow), inset 0 0 2px rgba(0,0,0,0.25)" : "inset 0 1px 1.5px rgba(0,0,0,0.4)"}; ` +
					`transform:translate(-50%,-50%) rotate(${angle}deg) translateY(-${radius}px);`,
			}),
		);
	}
	return frag;
}
