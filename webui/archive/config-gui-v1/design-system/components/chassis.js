// chassis.js - the plastic faceplate and rubber bevel: a static backdrop
// component with no sysex-driven state, per the "plastic faceplate and
// rubber bevel are a component, no variables" design decision (see
// webui/design-system/README.md). Only layout/geometry props (sizes come
// from the caller, which owns the tuning state) - never colour/on-off
// state, on purpose.
//
// buildChassis() returns the two nested wrapper elements (bevel, then
// face inside it) already appended to each other and to a size-fixed
// outer container; the caller appends its encoder grid and side switches
// into the returned `.face` element.

import { elc } from "./dom.js";

/**
 * @param {object} p
 * @param {number} p.size - outer chassis width/height, px
 * @param {number} p.cornerRadius
 * @param {number} p.bevelWidth
 * @returns {{outer: HTMLElement, bevel: HTMLElement, face: HTMLElement}}
 */
export function buildChassis(p) {
	const { size, cornerRadius, bevelWidth } = p;

	const outer = elc("div", {
		style: `position:relative; width:${size}px; height:${size}px; flex-shrink:0; margin:0 auto;`,
	});

	const bevel = elc("div", {
		style:
			`position:absolute; inset:0; border-radius:${cornerRadius}px; ` +
			"background:linear-gradient(158deg,#132420 0%,#0a1210 38%,#06090a 68%,#0f1c18 100%); " +
			"box-shadow:0 34px 70px rgba(0,0,0,0.6), inset 0 1px 0 rgba(157,255,219,0.05); " +
			`padding:${bevelWidth}px; box-sizing:border-box;`,
	});
	outer.appendChild(bevel);

	const faceRadius = Math.max(2, cornerRadius - bevelWidth);
	const face = elc("div", {
		style:
			`width:100%; height:100%; border-radius:${faceRadius}px; ` +
			"background:linear-gradient(168deg,#0d1815 0%,#080e0c 46%,#050706 78%,#0a1210 100%); " +
			"box-shadow:inset 0 1px 0 rgba(157,255,219,0.04), 0 1px 3px rgba(0,0,0,0.6); " +
			"box-sizing:border-box;",
	});
	bevel.appendChild(face);

	return { outer, bevel, face };
}
