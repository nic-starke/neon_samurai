// A labelled on/off switch.
//
// A real checkbox underneath, visually replaced - so it is focusable, keyboard
// operable and announced as a checkbox without any ARIA of its own.

import { elc } from "./dom.js";

/**
 * @param p.label        what the setting is
 * @param p.description  why it matters; optional, shown beneath
 * @param p.checked      current value
 * @param p.onChange     called with the new boolean
 */
export function buildToggle(p) {
	const input = elc("input", {
		class: "ds-toggle__input",
		attrs: { type: "checkbox", ...(p.checked ? { checked: "" } : {}) },
	});

	input.addEventListener("change", () => p.onChange?.(input.checked));

	const track = elc("span", { class: "ds-toggle__track", attrs: { "aria-hidden": "true" } });
	track.appendChild(elc("span", { class: "ds-toggle__knob" }));

	const label = elc("label", {
		class: "ds-toggle",
		children: [
			input,
			track,
			elc("span", {
				style:
					"font:400 11.5px/1.4 var(--ds-font); color:var(--ds-text);",
				text: p.label,
			}),
		],
	});

	if (!p.description) return label;

	return elc("div", {
		children: [
			label,
			elc("p", {
				style:
					"margin:5px 0 0 44px; font:400 10.5px/1.6 var(--ds-font); color:var(--ds-text-faint);",
				text: p.description,
			}),
		],
	});
}
