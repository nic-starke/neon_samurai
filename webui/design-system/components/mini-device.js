// A small Twister, for places a full chassis will not fit - a sidebar row, a
// device picker, a preset card.
//
// Accurate, not decorative: the encoder positions, the eleven indicator LEDs
// and their lit state, the RGB arc colour and the knob angle are all the real
// values, taken from the same props buildEncoder() is given. What it drops is
// everything that only reads at full size - cap knurling, bevels, gradients,
// glow and shadow. Flat shapes, right positions.
//
// It draws in the device's own coordinate system and lets the SVG viewBox do
// the scaling, so nothing here re-derives a proportion that could drift from
// geometry.js.

import { svgEl } from "./dom.js";
import { GEOMETRY as G } from "../geometry.js";

const MAX_BRIGHTNESS = 255; // matches led-mask.js

// As buildDeviceChassis() computes it: four bodies, three gaps, padding either
// side. 888 for the real part.
const GRID_GAP = G.pitch - G.bodySize;
const CHASSIS = 4 * G.bodySize + 3 * GRID_GAP + 2 * (G.edgeFirst - G.bodySize / 2);

const rad = (deg) => (deg * Math.PI) / 180;

function ledColor(props, index) {
	if (!props?.powered) return "var(--ds-led-powered-off)";

	if (props.ledColorOverride?.index === index) return props.ledColorOverride.color;

	if (props.ledBrightness) {
		const frac = Math.max(0, Math.min(1, props.ledBrightness[index] / MAX_BRIGHTNESS));
		return `color-mix(in srgb, var(--ds-led-on) ${(frac * 100).toFixed(0)}%, var(--ds-led-off))`;
	}

	return props.litMask?.[index] ? "var(--ds-led-on)" : "var(--ds-led-off)";
}

function encoder(cx, cy, props) {
	const g = svgEl("g", {});

	g.appendChild(
		svgEl("circle", { cx, cy, r: G.bodySize / 2, fill: "#141518" }),
	);

	// The RGB arc is a fixed segment at twelve o'clock - it shows colour, not
	// position. Same dash maths as rgb-arc.js, without the glow filter.
	const circumference = 2 * Math.PI * G.arcRadius;
	const off = props?.rgbOff !== false;
	g.appendChild(
		svgEl("circle", {
			cx,
			cy,
			r: G.arcRadius,
			fill: "none",
			stroke: off
				? props?.powered === false
					? "var(--ds-led-powered-off)"
					: "var(--ds-led-off)"
				: (props.rgbColor ?? "var(--ds-accent)"),
			"stroke-width": G.arcWidth,
			"stroke-linecap": "round",
			"stroke-dasharray": `${G.arcLength} ${(circumference + 50).toFixed(0)}`,
			"stroke-dashoffset": (-(circumference / 2 - G.arcLength / 2)).toFixed(2),
			transform: `rotate(-90 ${cx} ${cy})`,
		}),
	);

	// Eleven LEDs across ledArcSpan, centred on top - the same angles
	// buildLedRing() places them at.
	for (let i = 0; i < G.ledCount; i++) {
		const angle = -(G.ledArcSpan / 2) + (G.ledArcSpan / (G.ledCount - 1)) * i;
		g.appendChild(
			svgEl("circle", {
				cx: cx + Math.sin(rad(angle)) * G.ledRadius,
				cy: cy - Math.cos(rad(angle)) * G.ledRadius,
				r: G.ledSize / 2,
				fill: ledColor(props, i),
			}),
		);
	}

	const knobR = G.knobSize / 2;
	g.appendChild(svgEl("circle", { cx, cy, r: knobR, fill: "#26282b" }));

	// A plain circle cannot show which way it is turned, so the indicator the
	// knurling would otherwise imply is drawn explicitly.
	const turn = rad(props?.knobRotation ?? 0);
	g.appendChild(
		svgEl("line", {
			x1: cx + Math.sin(turn) * (knobR * 0.35),
			y1: cy - Math.cos(turn) * (knobR * 0.35),
			x2: cx + Math.sin(turn) * (knobR * 0.85),
			y2: cy - Math.cos(turn) * (knobR * 0.85),
			stroke: props?.powered === false ? "#3a3d42" : "#6e747c",
			"stroke-width": G.ledSize * 0.55,
			"stroke-linecap": "round",
		}),
	);

	return g;
}

/**
 * @param p.size      width in pixels; height matches
 * @param p.encoders  16 prop objects in grid order, as buildEncoder() takes.
 *                    Null or absent renders an unpowered device.
 * @param p.accent    border colour, for showing selection or state
 */
export function buildMiniDevice(p = {}) {
	const size = p.size ?? 56;

	const svg = svgEl("svg", {
		width: size,
		height: size,
		viewBox: `0 0 ${CHASSIS} ${CHASSIS}`,
		style: "flex:none; display:block;",
		"aria-hidden": "true",
	});

	svg.appendChild(
		svgEl("rect", {
			x: 0,
			y: 0,
			width: CHASSIS,
			height: CHASSIS,
			rx: G.cornerRadius,
			fill: "#1b1d21",
			stroke: p.accent ?? "var(--ds-border)",
			"stroke-width": 6,
		}),
	);

	for (let i = 0; i < 16; i++) {
		const cx = G.edgeFirst + (i % 4) * G.pitch;
		const cy = G.edgeFirst + Math.floor(i / 4) * G.pitch;
		svg.appendChild(encoder(cx, cy, p.encoders?.[i] ?? null));
	}

	return svg;
}
