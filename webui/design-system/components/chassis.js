// Faceplate/bevel are real hardware materials, not UI chrome - they stay
// neutral black/grey, not the Matrix palette tokens.css defines
// elsewhere. Only the page background around the device carries the
// green gradient (see live-twin.css/twin.css's `main`).

import { elc } from "./dom.js";

export function buildChassis(p) {
	const { size, cornerRadius, bevelWidth } = p;

	const outer = elc("div", {
		style: `position:relative; width:${size}px; height:${size}px; flex-shrink:0; margin:0 auto;`,
	});

	const bevel = elc("div", {
		style:
			`position:absolute; inset:0; border-radius:${cornerRadius}px; ` +
			"background:linear-gradient(158deg,#2c2e34 0%,#191a1e 38%,#121316 68%,#23252a 100%); " +
			"box-shadow:0 34px 70px rgba(0,0,0,0.6), inset 0 1px 0 rgba(255,255,255,0.06); " +
			`padding:${bevelWidth}px; box-sizing:border-box;`,
	});
	outer.appendChild(bevel);

	const faceRadius = Math.max(2, cornerRadius - bevelWidth);
	const face = elc("div", {
		style:
			`width:100%; height:100%; border-radius:${faceRadius}px; ` +
			"background:linear-gradient(168deg,#1e2024 0%,#15161a 46%,#0f1012 78%,#17181c 100%); " +
			"box-shadow:inset 0 1px 0 rgba(255,255,255,0.05), 0 1px 3px rgba(0,0,0,0.6); " +
			"box-sizing:border-box;",
	});
	bevel.appendChild(face);

	return { outer, bevel, face };
}
