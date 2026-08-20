import { elc } from "./dom.js";

export function buildSideSwitch(p) {
	const { side, width, height, top, pressed, selected, onSelect } = p;
	// Idle colour is neutral black/grey (real hardware plastic); pressed/
	// selected use the accent colour deliberately - that's live/UI signal,
	// not a passive material finish.
	return elc("button", {
		class: "ds-side-switch",
		attrs: { type: "button", "aria-label": `${side === "L" ? "Left" : "Right"} side switch ${p.index + 1}` },
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
						: "linear-gradient(180deg,#3a3f4a,#1a1d22)"
			}; ` +
			`box-shadow:inset 0 1px 0 rgba(255,255,255,0.12), ${side === "L" ? "-2px" : "2px"} 2px 5px rgba(0,0,0,0.55);`,
		onClick: onSelect,
	});
}
