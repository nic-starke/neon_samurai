// LEDs sit at fixed angular positions, not a continuous arc - `count`
// LEDs spread evenly across `arcSpan` degrees, centred on top. For a
// firmware-accurate lit pattern (bar graph / center-out detent / etc,
// matching mf_draw_encoder() in src/led/led.c) compute one with
// led-mask.js's computeLitMask() and pass it as `litMask`.

import { elc } from "./dom.js";

export function buildLedRing(p) {
	const count = p.count ?? 11;
	const radius = p.radius ?? 37;
	const size = p.size ?? 9;
	const span = p.arcSpan ?? 270;
	const value = p.value ?? 31;
	const max = p.max ?? 127;
	const powered = p.powered ?? true;
	const lit = Math.round(count * ((value || 0) / (max || 1)));
	const litMask = p.litMask ?? Array.from({ length: count }, (_, i) => i < lit);

	const frag = document.createDocumentFragment();
	for (let s = 0; s < count; s++) {
		const angle = -(span / 2) + (span / (count - 1)) * s;
		const litSeg = powered && Boolean(litMask[s]);
		const offColor = powered ? "var(--ds-led-off)" : "var(--ds-led-powered-off)";
		frag.appendChild(
			elc("div", {
				style:
					`position:absolute; top:50%; left:50%; width:${size}px; height:${size}px; border-radius:50%; ` +
					`background:${litSeg ? "var(--ds-led-on)" : offColor}; ` +
					`box-shadow:${litSeg ? "var(--ds-glow), inset 0 0 2px rgba(0,0,0,0.25)" : "inset 0 1px 1.5px rgba(0,0,0,0.4)"}; ` +
					`transform:translate(-50%,-50%) rotate(${angle}deg) translateY(-${radius}px);`,
			}),
		);
	}
	return frag;
}
