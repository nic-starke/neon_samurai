// Small pill showing which vmap layer (A/B) is currently active on an
// encoder - one letter per layer with a divider between, the active one
// highlighted. NUM_VMAPS_PER_ENCODER is 2 on real hardware
// (device-model.js), but this renders however many `count` says, labelled
// A, B, C, ... in order.
//
// Two layouts: the default vertical stack (A over B, divider between) for
// placement beside the encoder ring, and `compact` - a small horizontal
// chip (A|B side by side) sized to sit directly on the cap's inner disc,
// where a tall vertical pill wouldn't fit.

import { elc } from "./dom.js";

const LETTERS = "ABCDEFGH";

// One colour per layer, not a shared highlight - a vmap's letter needs to
// read as "which one" at a glance, not just "the active one" (which
// --ds-accent already means elsewhere, e.g. selection rings). Cyan (cool)
// + amber (warm) is a deliberate two-colour pair, not arbitrary picks -
// see tokens.css's --ds-cyan/--ds-amber. Anything past B falls back to
// the neutral accent - real hardware only ever has 2 vmaps.
const LETTER_COLORS = ["var(--ds-cyan)", "var(--ds-amber)"];

export function buildVmapPill(p) {
	const count = p.count ?? 2;
	const active = p.active ?? 0;
	const powered = p.powered ?? true;
	const compact = p.compact ?? false;

	const borderColor = powered ? "var(--ds-border-bright)" : "var(--ds-border)";
	const pill = elc("div", {
		style:
			`display:flex; flex-direction:${compact ? "row" : "column"}; border-radius:999px; overflow:hidden; ` +
			`border:1px solid ${borderColor}; background:var(--ds-bg-inset); ` +
			(compact ? "" : "width:16px;"),
	});

	for (let i = 0; i < count; i++) {
		const isActive = powered && i === active;
		const color = LETTER_COLORS[i] ?? "var(--ds-accent)";
		if (i > 0) {
			pill.appendChild(
				elc("div", {
					style: compact ? `width:1px; background:${borderColor};` : `height:1px; background:${borderColor};`,
				}),
			);
		}
		pill.appendChild(
			elc("span", {
				style:
					`display:flex; align-items:center; justify-content:center; ${compact ? "width:9px; height:9px;" : "height:14px;"} ` +
					`font-family:var(--ds-font-mono); font-size:${compact ? 6.5 : 9}px; font-weight:600; line-height:1; ` +
					`background:${isActive ? color : "transparent"}; ` +
					`color:${isActive ? "var(--ds-bg)" : powered ? "var(--ds-text-dim)" : "var(--ds-text-faint)"};`,
				text: LETTERS[i] ?? "?",
			}),
		);
	}

	return pill;
}
