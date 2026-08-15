// side-switch.js - one physical side switch (6 on real hardware, left and
// right of the encoder grid). No LED/colour state - pressed/unpressed
// only, per the "side switch is its own component" design decision (see
// webui/design-system/README.md). Geometry ported from Twister Digital
// Twin.dc.html's side-button loop.

import { elc } from "./dom.js";

/**
 * @param {object} p
 * @param {"L"|"R"} p.side
 * @param {number} p.index - 0-2 within this side
 * @param {number} p.width
 * @param {number} p.height
 * @param {number} p.top - top offset, px, within the parent's positioning context
 * @param {boolean} [p.pressed]
 * @param {boolean} [p.selected] - browsing/focus state in the tuning UI, distinct from `pressed` (the physical state)
 * @param {() => void} [p.onSelect]
 */
export function buildSideSwitch(p) {
	const { side, width, height, top, pressed, selected, onSelect } = p;
	return elc("button", {
		title: `${side === "L" ? "Left" : "Right"} Side ${p.index + 1}`,
		style:
			`position:absolute; ${side === "L" ? "left" : "right"}:-${width}px; top:${top}px; ` +
			`width:${width}px; height:${height}px; border-radius:${side === "L" ? "6px 0 0 6px" : "0 6px 6px 0"}; ` +
			"border:none; padding:0; cursor:pointer; " +
			`background:${
				pressed
					? "linear-gradient(180deg,var(--ds-accent),var(--ds-accent-dim))"
					: selected
						? "linear-gradient(180deg,var(--ds-cyan),var(--ds-accent-dim))"
						: "linear-gradient(180deg,#16241f,#080b0a)"
			}; ` +
			`box-shadow:inset 0 1px 0 rgba(157,255,219,0.10), ${side === "L" ? "-2px" : "2px"} 2px 5px rgba(0,0,0,0.55);`,
		onClick: onSelect,
	});
}
