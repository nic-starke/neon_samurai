// A small Twister, for places a full chassis will not fit - a sidebar row, a
// device picker, a preset card.
//
// Accurate, not decorative: the encoder positions, the eleven indicator LEDs
// and their lit state and the RGB arc colour are all the real values, taken
// from the same props buildEncoder() is given. What it drops is everything
// that only reads at full size - cap knurling, bevels, gradients, glow,
// shadow, and the knob's angle. Flat shapes, right positions.
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

// No props at all is not the same as a props object with no `powered` field.
// The former is the placeholder for a device with no data yet - detected but
// not connected - and has to render unlit. The latter is a live encoder from
// a connected device, which never sets `powered` at all and is always
// powered - only the explicit disconnected chassis sets it to false. Getting
// this backwards either leaves every indicator dark on real hardware, or
// crashes reading a field off a null placeholder - it has been both.
export function isPowered(props) {
	return props ? (props.powered ?? true) : false;
}

export function ledColor(props, index) {
	if (!isPowered(props)) return "var(--ds-led-powered-off)";

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
				? isPowered(props)
					? "var(--ds-led-off)"
					: "var(--ds-led-powered-off)"
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

	// Flat, unmarked. The knob's angle is not drawn - the LED ring already
	// says where the encoder is, and a pointer at this size reads as clutter.
	g.appendChild(svgEl("circle", { cx, cy, r: G.knobSize / 2, fill: "#26282b" }));

	return g;
}

/**
 * @param p.size      width in pixels; height matches
 * @param p.encoders  16 prop objects in grid order, as buildEncoder() takes.
 *                    Null or absent renders an unpowered device.
 * @param p.outerStroke  colour for a ring drawn outside the chassis edge -
 *                         e.g. to mark the row it belongs to as connected.
 *                         Absent by default; the chassis's own border is
 *                         always drawn regardless.
 * @param p.key       marks the element so a caller can find and replace it
 */
export function buildMiniDevice(p = {}) {
	const size = p.size ?? 56;

	// The ring sits outside 0..CHASSIS, which the SVG's own viewport would
	// otherwise clip - overflow:visible on the root is what lets it (and any
	// stroke straddling the true edge) draw past that boundary uncropped.
	const svg = svgEl("svg", {
		width: size,
		height: size,
		viewBox: `0 0 ${CHASSIS} ${CHASSIS}`,
		style: "flex:none; display:block; overflow:visible;",
		"aria-hidden": "true",
		...(p.key ? { "data-mini": p.key } : {}),
	});

	if (p.outerStroke) {
		// Drawn in the same 0..CHASSIS space as everything else, but a fixed
		// number of device-space units shrinks to a fraction of a real pixel
		// at this element's actual display size and disappears - a ring meant
		// to read as a crisp couple of pixels has to be sized from the real
		// display size instead, not the coordinate space.
		const displayScale = size / CHASSIS;
		const gap = 3 / displayScale;
		const width = 2.5 / displayScale;
		svg.appendChild(
			svgEl("rect", {
				x: -gap,
				y: -gap,
				width: CHASSIS + gap * 2,
				height: CHASSIS + gap * 2,
				rx: G.cornerRadius + gap,
				fill: "none",
				stroke: p.outerStroke,
				"stroke-width": width,
			}),
		);
	}

	svg.appendChild(
		svgEl("rect", {
			x: 0,
			y: 0,
			width: CHASSIS,
			height: CHASSIS,
			rx: G.cornerRadius,
			fill: "#1b1d21",
			stroke: "var(--ds-border)",
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
