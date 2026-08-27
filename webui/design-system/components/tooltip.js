// A small coloured pill: an icon, then a message. Not a native `title`
// tooltip - this is drawn and styled by the caller, so severity has a
// colour and a mark rather than plain hover text.
//
// Positioning is the caller's job. This returns the pill only; pairing it
// with a trigger that reveals it on hover is a couple of CSS rules, not
// anything this needs to know about.

import { elc } from "./dom.js";

/**
 * @param p.text   the message
 * @param p.color  CSS colour (or var()) for the border, background tint and text
 * @param p.icon   optional glyph shown to the left of the text
 */
export function buildTooltip(p) {
	const children = [];

	if (p.icon) {
		children.push(
			elc("span", { attrs: { "aria-hidden": "true" }, text: p.icon }),
		);
	}

	children.push(elc("span", { text: p.text }));

	return elc("div", {
		class: "ds-tooltip",
		attrs: { role: "status" },
		style:
			"display:flex; align-items:center; gap:6px; padding:6px 10px; " +
			"border-radius:999px; white-space:nowrap; " +
			"font:600 10px/1 var(--ds-font-mono); letter-spacing:0.05em; " +
			`border:1px solid ${p.color}; color:${p.color}; ` +
			`background:color-mix(in srgb, ${p.color} 16%, var(--ds-bg-inset));`,
		children,
	});
}
