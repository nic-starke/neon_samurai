// twin-render.js - pure-ish rendering primitives for the "digital twin"
// device view (webui/twin.html): a skeuomorphic render of the physical
// Midi Fighter Twister chassis, ported from a Claude Design Canvas
// prototype ("Twister Digital Twin.dc.html", importing "Encoder.dc.html"
// and "ChromaCap.dc.html"). No framework/build step - same constraint as
// the rest of webui/.
//
// This is a standalone visual/design-tuning tool, not wired to the real
// device model (see webui/js/device-model.js) or the sysex protocol - it
// exists to preview and dial in the chassis/encoder/cap geometry that a
// future real device-twin view could reuse, not to reflect live hardware
// state. See twin.js for the page's state and control panel, which is the
// only thing here with an opinion about what the numbers mean.
//
// The colour math below (shift/hsvHex) is standard 0-360/0-100/0-100 HSV
// for cosmetic shading of plastic/material tones - unrelated to color.js's
// firmware-range (0-1535/0-255/0-255) HSV model used elsewhere in this
// app, so it is not shared with that module.

const SVG_NS = "http://www.w3.org/2000/svg";

/** Create an HTML element. `opts.style` is a CSS text string (matching
 * this codebase's existing inline-style convention in ui.js). */
export function elc(tag, opts = {}) {
	const e = document.createElement(tag);
	if (opts.style) e.style.cssText = opts.style;
	if (opts.class) e.className = opts.class;
	if (opts.text !== undefined) e.textContent = opts.text;
	if (opts.title !== undefined) e.title = opts.title;
	if (opts.onClick) e.addEventListener("click", opts.onClick);
	if (opts.attrs) {
		for (const [k, v] of Object.entries(opts.attrs)) e.setAttribute(k, v);
	}
	for (const child of opts.children || []) e.appendChild(child);
	return e;
}

/** Create an SVG element. */
export function svgEl(tag, attrs = {}) {
	const e = document.createElementNS(SVG_NS, tag);
	for (const [k, v] of Object.entries(attrs)) e.setAttribute(k, String(v));
	return e;
}

function hexToRgb(hex) {
	const h = (hex || "#4a4d55").replace("#", "");
	const s = h.length === 3 ? h.split("").map((c) => c + c).join("") : h;
	return [parseInt(s.slice(0, 2), 16), parseInt(s.slice(2, 4), 16), parseInt(s.slice(4, 6), 16)];
}

/** Lighten (amount > 0) or darken (amount < 0) a hex colour, returning a
 * CSS rgb() string. amount is roughly -1..1. */
export function shift(hex, amount) {
	const [r, g, b] = hexToRgb(hex);
	const f = (v) =>
		Math.max(0, Math.min(255, Math.round(amount > 0 ? v + (255 - v) * amount : v * (1 + amount))));
	return `rgb(${f(r)},${f(g)},${f(b)})`;
}

/** Standard-range (h:0-360, s/v:0-100) HSV to hex, for cosmetic material
 * tones. Not the firmware colour model - see color.js for that. */
export function hsvHex(h, s, v) {
	const S = s / 100;
	const V = v / 100;
	const c = V * S;
	const hp = (((h % 360) + 360) % 360) / 60;
	const x = c * (1 - Math.abs((hp % 2) - 1));
	const seg = [
		[c, x, 0],
		[x, c, 0],
		[0, c, x],
		[0, x, c],
		[x, 0, c],
		[c, 0, x],
	][Math.floor(hp) % 6];
	const m = V - c;
	return (
		"#" +
		seg
			.map((n) => Math.round((n + m) * 255).toString(16).padStart(2, "0"))
			.join("")
	);
}

const MM_TO_UNITS = 10; // drawing units per mm, matching ChromaCap.dc.html's `U`

/**
 * Build the top-view (looking straight down) SVG of one knurled encoder
 * cap: a fluted cylinder rendered as a scalloped silhouette plus one
 * shaded face per flute, ported from ChromaCap.dc.html's top-view branch
 * (its side-elevation view, dimension lines and text callouts are all
 * disabled by Encoder.dc.html's dc-import and are not reproduced here).
 */
export function buildCapTopSvg(p) {
	const color = p.color ?? "#4a4d55";
	const baseDia = p.baseDia ?? 18.5;
	const gripDiaBottom = p.gripDiaBottom ?? 15;
	const gripDiaTop = p.gripDiaTop ?? 13.5;
	const ribCount = p.ribCount ?? 19;
	const innerDia = p.innerScallopDia ?? 7.5;

	const halfBase = (baseDia * MM_TO_UNITS) / 2;
	const halfGripB = (gripDiaBottom * MM_TO_UNITS) / 2;
	const halfGripT = (gripDiaTop * MM_TO_UNITS) / 2;
	const halfInner = (innerDia * MM_TO_UNITS) / 2;

	// topViewPad is always 0 here (Encoder.dc.html pins it), so the view
	// origin sits exactly at the base radius.
	const topCX = halfBase;
	const vbSize = halfBase * 2;

	const flutes = ribCount;
	const step = (Math.PI * 2) / flutes;
	const lightAngle = (((p.lightAngle ?? 315) - 90) * Math.PI) / 180;
	const lightAngle2 = lightAngle + (((p.lightOffset ?? 180) * Math.PI) / 180);
	const light2Strength = p.light2Strength ?? 0.6;
	const fluteDepth = Math.max(1.2, (halfGripB - halfGripT) * 0.42);
	const rPeak = halfGripB;
	const rValley = halfGripB - fluteDepth;

	// Scalloped silhouette: one valley->peak->valley lobe per flute.
	let scallop = "";
	for (let i = 0; i < flutes; i++) {
		const aPeak = -Math.PI / 2 + step * i;
		const aV0 = aPeak - step / 2;
		const aV1 = aPeak + step / 2;
		const v0x = topCX + Math.cos(aV0) * rValley;
		const v0y = topCX + Math.sin(aV0) * rValley;
		const v1x = topCX + Math.cos(aV1) * rValley;
		const v1y = topCX + Math.sin(aV1) * rValley;
		const cr = rPeak + (rPeak - rValley) * 0.9;
		const cxp = topCX + Math.cos(aPeak) * cr;
		const cyp = topCX + Math.sin(aPeak) * cr;
		scallop +=
			(i === 0 ? `M ${v0x.toFixed(2)} ${v0y.toFixed(2)}` : "") +
			` Q ${cxp.toFixed(2)} ${cyp.toFixed(2)} ${v1x.toFixed(2)} ${v1y.toFixed(2)}`;
	}
	scallop += " Z";

	// One shaded face per flute; brightness from a two-light Lambert term
	// so the ring reads as lit rather than a flat repeating pattern.
	const faces = [];
	for (let i = 0; i < flutes; i++) {
		const aPeak = -Math.PI / 2 + step * i;
		const l1 = Math.max(0, Math.cos(aPeak - lightAngle));
		const l2 = Math.max(0, Math.cos(aPeak - lightAngle2)) * light2Strength;
		const lambert = Math.min(1, l1 + l2);
		const amt = -0.2 + 0.42 * lambert;
		const half = step * 0.4;
		const rIn = halfGripT;
		const p1 = [topCX + Math.cos(aPeak - half) * rIn, topCX + Math.sin(aPeak - half) * rIn];
		const p2 = [topCX + Math.cos(aPeak - half) * rValley, topCX + Math.sin(aPeak - half) * rValley];
		const p3 = [topCX + Math.cos(aPeak + half) * rValley, topCX + Math.sin(aPeak + half) * rValley];
		const p4 = [topCX + Math.cos(aPeak + half) * rIn, topCX + Math.sin(aPeak + half) * rIn];
		const cr = rPeak + (rPeak - rValley) * 0.5;
		const cxp = topCX + Math.cos(aPeak) * cr;
		const cyp = topCX + Math.sin(aPeak) * cr;
		faces.push({
			d:
				`M ${p1[0].toFixed(2)} ${p1[1].toFixed(2)} ` +
				`L ${p2[0].toFixed(2)} ${p2[1].toFixed(2)} ` +
				`Q ${cxp.toFixed(2)} ${cyp.toFixed(2)} ${p3[0].toFixed(2)} ${p3[1].toFixed(2)} ` +
				`L ${p4[0].toFixed(2)} ${p4[1].toFixed(2)} ` +
				`A ${rIn.toFixed(2)} ${rIn.toFixed(2)} 0 0 0 ${p1[0].toFixed(2)} ${p1[1].toFixed(2)} Z`,
			fill: shift(color, amt),
		});
	}

	const svg = svgEl("svg", {
		viewBox: `0 0 ${vbSize.toFixed(0)} ${vbSize.toFixed(0)}`,
		width: p.size,
		height: p.size,
		style: "position:absolute; inset:0; overflow:visible;",
	});

	svg.appendChild(svgEl("circle", { cx: topCX, cy: topCX, r: halfBase, fill: shift(color, -0.34) }));
	svg.appendChild(svgEl("path", { d: scallop, fill: shift(color, -0.3) }));
	for (const f of faces) svg.appendChild(svgEl("path", { d: f.d, fill: f.fill }));
	svg.appendChild(svgEl("circle", { cx: topCX, cy: topCX, r: halfGripT, fill: p.topFaceHex ?? "#3a3f4a" }));
	svg.appendChild(svgEl("circle", { cx: topCX, cy: topCX, r: halfInner, fill: p.innerFaceHex ?? "#2c2f36" }));
	svg.appendChild(
		svgEl("circle", {
			cx: topCX,
			cy: topCX,
			r: Math.max(0, halfInner - 0.5),
			fill: "none",
			stroke: shift(color, -0.18),
			"stroke-width": 1,
		}),
	);

	return svg;
}

/**
 * Build one encoder: plastic ring body, RGB indicator arc, indicator LED
 * ring, and the knurled cap (via buildCapTopSvg), ported from
 * Encoder.dc.html.
 */
export function buildEncoder(p) {
	const bodySize = p.bodySize ?? 91;
	const center = bodySize / 2;
	const value = p.value ?? 31;
	const max = p.max ?? 127;
	const count = p.ledCount ?? 11;
	const ledRadius = p.ledRadius ?? 37;
	const ledSize = p.ledSize ?? 9;
	const span = p.ledArcSpan ?? 270;
	const lit = Math.round(count * ((value || 0) / (max || 1)));
	const showLabel = p.showLabel ?? true;

	const outer = elc("div", {
		style: `cursor:pointer; display:flex; flex-direction:column; align-items:center; justify-content:center; gap:${showLabel ? 9 : 0}px; flex-shrink:0; padding:${showLabel ? 6 : 0}px;`,
		title: p.label ?? "",
		onClick: p.onSelect,
	});

	const bodyWrap = elc("div", {
		style: `position:relative; width:${bodySize}px; height:${bodySize}px; display:flex; align-items:center; justify-content:center;`,
	});
	outer.appendChild(bodyWrap);

	const selShadow = p.selected ? "0 0 0 3px var(--twin-accent)" : "0 0 0 0 transparent";
	const body = elc("div", {
		style:
			`position:relative; width:${bodySize}px; height:${bodySize}px; border-radius:50%; ` +
			"background:radial-gradient(circle at 50% 118%, #1e2026 0%, #0a0b0d 55%), linear-gradient(157deg, #2f333c 0%, #14161a 34%, #08090b 68%, #101216 100%); " +
			`box-shadow:${selShadow}, inset 0 1.5px 1px rgba(255,255,255,0.11), inset 0 -2px 3px rgba(255,255,255,0.025), 0 4px 10px rgba(0,0,0,0.65); ` +
			"display:flex; align-items:center; justify-content:center; overflow:hidden;",
	});
	bodyWrap.appendChild(body);

	body.appendChild(
		elc("div", {
			style:
				"position:absolute; top:-12%; left:2%; width:74%; height:46%; border-radius:50%; background:linear-gradient(180deg, rgba(255,255,255,0.15) 0%, rgba(255,255,255,0.03) 55%, transparent 100%); filter:blur(3px); pointer-events:none;",
		}),
	);
	body.appendChild(
		elc("div", {
			style:
				"position:absolute; bottom:-6%; right:6%; width:52%; height:30%; border-radius:50%; background:linear-gradient(0deg, rgba(255,255,255,0.07) 0%, transparent 100%); filter:blur(4px); pointer-events:none;",
		}),
	);

	// RGB indicator arc - a fixed-length accent stroke, not a value gauge.
	const arcRadius = p.arcRadius ?? 37.5;
	const arcLength = p.arcLength ?? 25;
	const arcWidth = p.arcWidth ?? 9;
	const circumference = 2 * Math.PI * arcRadius;
	const rgbColor = p.rgbColor ?? "#ff3b6b";
	const arcSvg = svgEl("svg", {
		width: bodySize,
		height: bodySize,
		viewBox: `0 0 ${bodySize} ${bodySize}`,
		style: "position:absolute; inset:0; overflow:visible;",
	});
	arcSvg.appendChild(
		svgEl("circle", {
			cx: center,
			cy: center,
			r: arcRadius,
			fill: "none",
			stroke: rgbColor,
			"stroke-width": arcWidth,
			"stroke-linecap": "round",
			"stroke-dasharray": `${arcLength} ${(circumference + 50).toFixed(0)}`,
			"stroke-dashoffset": (-(circumference / 2 - arcLength / 2)).toFixed(2),
			transform: `rotate(-90 ${center} ${center})`,
			style: `filter:drop-shadow(0 0 5px ${rgbColor});`,
		}),
	);
	body.appendChild(arcSvg);

	// Indicator LED ring.
	for (let s = 0; s < count; s++) {
		const angle = -(span / 2) + (span / (count - 1)) * s;
		const litSeg = s < lit;
		body.appendChild(
			elc("div", {
				style:
					`position:absolute; top:50%; left:50%; width:${ledSize}px; height:${ledSize}px; border-radius:50%; ` +
					`background:${litSeg ? "#ffffff" : "#c6c8cc"}; ` +
					`box-shadow:${litSeg ? "0 0 7px rgba(255,255,255,0.85), inset 0 0 2px rgba(0,0,0,0.25)" : "inset 0 1px 1.5px rgba(0,0,0,0.35)"}; ` +
					`transform:translate(-50%,-50%) rotate(${angle}deg) translateY(-${ledRadius}px);`,
			}),
		);
	}

	// Cap (rubber knob + knurled top).
	const knobSize = p.knobSize ?? 53;
	const knob = elc("div", {
		style: `width:${knobSize}px; height:${knobSize}px; border-radius:50%; box-shadow:0 3px 7px rgba(0,0,0,0.65); position:relative; display:flex; align-items:center; justify-content:center;`,
	});
	body.appendChild(knob);
	knob.appendChild(
		buildCapTopSvg({
			size: knobSize,
			color: p.capColor ?? "#1f2126",
			innerScallopDia: p.capInnerDia ?? 7.5,
			baseDia: p.capBaseDia ?? 18.5,
			gripDiaBottom: p.capGripDiaBottom ?? 15,
			gripDiaTop: p.capGripDiaTop ?? 13.5,
			ribCount: p.capRibCount ?? 19,
			lightAngle: p.capLightAngle ?? 265,
			lightOffset: p.capLightOffset ?? 180,
			topFaceHex: hsvHex(p.capTopHue ?? 226, p.capTopSat ?? 5, p.capTopVal ?? 21),
			innerFaceHex: hsvHex(p.capInnerHue ?? 219, p.capInnerSat ?? 9, p.capInnerVal ?? 22),
		}),
	);

	if (showLabel) {
		outer.appendChild(
			elc("span", {
				style: "font-family:var(--twin-font-mono); font-size:10.5px; color:#8a92a3;",
				text: p.label ?? "",
			}),
		);
	}

	return outer;
}
