// rgb-arc.js - the RGB backlight indicator arc on an encoder: a short,
// fixed-length coloured stroke (not a value gauge - see the vmap's own
// hsv colour, distinct from the indicator LED ring's on/off pattern).
// Ported from Encoder.dc.html.

import { svgEl } from "./dom.js";

/**
 * @param {object} p
 * @param {number} [p.bodySize] - encoder body diameter, px (arc is centred in it)
 * @param {number} [p.radius]
 * @param {number} [p.width] - stroke width, px
 * @param {number} [p.length] - visible arc length, px
 * @param {string} [p.color] - CSS colour string
 * @param {boolean} [p.off] - when true, renders in the unlit LED colour with no glow, instead of `color`'s glow - an unlit RGB LED does not glow, so this must suppress the drop-shadow entirely rather than just dimming the stroke colour
 */
export function buildRgbArc(p) {
	const bodySize = p.bodySize ?? 91;
	const center = bodySize / 2;
	const radius = p.radius ?? 37.5;
	const length = p.length ?? 25;
	const width = p.width ?? 9;
	const circumference = 2 * Math.PI * radius;
	const off = Boolean(p.off);
	const color = off ? "var(--ds-led-off)" : (p.color ?? "var(--ds-accent)");

	const svg = svgEl("svg", {
		width: bodySize,
		height: bodySize,
		viewBox: `0 0 ${bodySize} ${bodySize}`,
		style: "position:absolute; inset:0; overflow:visible;",
	});
	svg.appendChild(
		svgEl("circle", {
			cx: center,
			cy: center,
			r: radius,
			fill: "none",
			stroke: color,
			"stroke-width": width,
			"stroke-linecap": "round",
			"stroke-dasharray": `${length} ${(circumference + 50).toFixed(0)}`,
			"stroke-dashoffset": (-(circumference / 2 - length / 2)).toFixed(2),
			transform: `rotate(-90 ${center} ${center})`,
			// Off state: no glow at all - a real unlit RGB LED doesn't emit
			// light, so a dimmed-but-still-glowing arc would misrepresent it.
			style: off ? "" : `filter:drop-shadow(0 0 5px ${color});`,
		}),
	);
	return svg;
}
