// "BANK 1 2 3" label, active number highlighted - deliberately a different
// visual language from vmap-pill.js's filled chip (no background shapes,
// just a text label and numbers). Positioned by its caller, not here.
//
// Each number is a real <button> with aria-pressed, so the one interactive
// control on the live page is keyboard-reachable and announces its state.
// `pending` marks a bank the user has selected but the device has not yet
// confirmed - see editor.js's switchBank().

import { elc } from "./dom.js";

export function buildBankSelector(p) {
	const count = p.count ?? 3;
	const active = p.active ?? 0;
	const pending = p.pending ?? null;
	const onSelect = p.onSelect;

	const wrap = elc("div", {
		attrs: { role: "group", "aria-label": "Active bank" },
		style:
			"display:flex; align-items:baseline; gap:10px; font-family:var(--ds-font-mono); " +
			"letter-spacing:0.08em;",
	});

	wrap.appendChild(
		elc("span", {
			style: "font-size:11px; color:var(--ds-text-faint); font-weight:600;",
			text: "BANK",
		}),
	);

	const numbers = elc("div", { style: "display:flex; gap:8px;" });
	for (let i = 0; i < count; i++) {
		const isActive = i === active;
		const isPending = pending === i && !isActive;
		const color = isActive
			? "var(--ds-accent-bright)"
			: isPending
				? "var(--ds-amber)"
				: "var(--ds-text-faint)";

		const attrs = {
			type: "button",
			"aria-pressed": String(isActive),
			"aria-label": `Bank ${i + 1}`,
		};
		if (!onSelect) attrs.disabled = "";
		if (isPending) attrs["aria-busy"] = "true";

		numbers.appendChild(
			elc("button", {
				class: "ds-bank-number",
				attrs,
				style:
					`font:inherit; background:none; border:none; padding:2px 4px; margin:0; ` +
					`font-size:${isActive ? 15 : 13}px; font-weight:${isActive ? 700 : 500}; line-height:1; ` +
					`color:${color}; ` +
					`text-shadow:${isActive ? "var(--ds-glow-soft)" : "none"}; ` +
					`opacity:${!onSelect && !isActive ? 0.55 : 1}; ` +
					`cursor:${onSelect ? "pointer" : "default"};`,
				text: String(i + 1),
				onClick: onSelect ? () => onSelect(i) : undefined,
			}),
		);
	}
	wrap.appendChild(numbers);

	return wrap;
}
