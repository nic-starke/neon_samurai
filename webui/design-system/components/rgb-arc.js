import { svgEl } from "./dom.js";

const GLOW_BLUR = 5;

export function buildRgbArc(p) {
	const bodySize = p.bodySize ?? 91;
	const center = bodySize / 2;
	const radius = p.radius ?? 37.5;
	const length = p.length ?? 25;
	const width = p.width ?? 9;
	const circumference = 2 * Math.PI * radius;
	const off = Boolean(p.off);
	const powered = p.powered ?? true;
	const color = off
		? (powered ? "var(--ds-led-off)" : "var(--ds-led-powered-off)")
		: (p.color ?? "var(--ds-accent)");

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
			// off suppresses the glow entirely, not just the colour - a real
			// unlit LED doesn't emit light.
			style: off ? "" : `filter:drop-shadow(0 0 ${GLOW_BLUR}px ${color});`,
		}),
	);
	return svg;
}
