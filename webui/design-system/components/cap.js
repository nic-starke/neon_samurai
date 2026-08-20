// Ported from a Claude Design Canvas prototype ("ChromaCap.dc.html").

import { svgEl } from "./dom.js";
import { shift } from "./color-utils.js";

const MM_TO_UNITS = 10; // drawing units per mm, matching ChromaCap.dc.html's `U`

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

	// encoder.js rotates this whole SVG by knobRotation, since the physical
	// cap spins with the knob. The knurl lights and RGB reflection are
	// panel-fixed, so their angles are counter-rotated here to cancel that
	// and land back at the same panel-relative position.
	const flutes = ribCount;
	const step = (Math.PI * 2) / flutes;
	const knobRotation = p.knobRotation ?? 0;
	const rotationComp = (-knobRotation * Math.PI) / 180;
	const lightAngle = (((p.lightAngle ?? 315) - 90) * Math.PI) / 180 + rotationComp;
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

	// Reflection from the nearby lit RGB LED: radial gradient centred at
	// the bottom edge (where the LED physically sits), fading out within
	// a couple of flutes so it reads as ambient bounce, not backlighting.
	if (p.reflectionColor) {
		const gradId = `cap-reflect-${Math.random().toString(36).slice(2, 9)}`;
		const defs = svgEl("defs", {});
		const grad = svgEl("radialGradient", {
			id: gradId,
			cx: "50%",
			cy: "100%",
			r: "38.5%",
			// Pivot is in objectBoundingBox units (0-1), despite cx/cy above
			// reading as percentages - a pivot of "50 50" lands far outside
			// the box and silently blanks the gradient.
			gradientTransform: `rotate(${-knobRotation} 0.5 0.5)`,
		});
		const strength = p.reflectionStrength ?? 0.154;
		grad.appendChild(svgEl("stop", { offset: "0%", "stop-color": p.reflectionColor, "stop-opacity": strength }));
		grad.appendChild(svgEl("stop", { offset: "100%", "stop-color": p.reflectionColor, "stop-opacity": 0 }));
		defs.appendChild(grad);
		svg.appendChild(defs);
		svg.appendChild(
			svgEl("circle", {
				cx: topCX,
				cy: topCX,
				r: halfBase,
				fill: `url(#${gradId})`,
				style: "pointer-events:none;",
			}),
		);
	}

	return svg;
}
