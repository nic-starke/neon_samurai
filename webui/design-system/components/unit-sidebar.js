// The list of connected units, down the left of the editor.
//
// One row per Twister, each showing a thumbnail of its current bank so a unit
// can be told apart at a glance without selecting it. Only one unit can be
// attached at a time today; the region exists because the layout is built
// around it, and because a device in the bootloader has to stay visible here -
// a half-flashed unit must not vanish from the tool that recovers it.
//
// This builds the list only. The connect, firmware and link chrome below it in
// the sidebar lives in index.html, because it acts on the editor rather than on
// any one row.

import { elc } from "./dom.js";

// State determines the row's accent and its trailing glyph. Anything the
// editor cannot configure reads as amber, so "needs attention" is one colour.
const STATES = {
	connected: { color: "var(--ds-accent)", icon: "", label: "Connected" },
	bootloader: { color: "var(--ds-amber)", icon: "⚑", label: "Bootloader" },
	djtt: { color: "var(--ds-text-dim)", icon: "◈", label: "Stock DJTT firmware" },
	incompatible: { color: "var(--ds-amber)", icon: "!", label: "Incompatible firmware" },
	unavailable: { color: "var(--ds-danger)", icon: "✕", label: "Port unavailable" },
};

function thumbnail(colors) {
	const wrap = elc("div", {
		style:
			"flex:none; display:grid; grid-template-columns:repeat(4,1fr); gap:2px; " +
			"width:34px; height:34px; padding:3px; border-radius:5px; " +
			"background:var(--ds-bg-inset); border:1px solid var(--ds-border);",
	});

	// Sixteen dots in bank order. Without a connection there is nothing to
	// show, so they sit at the unlit colour rather than being hidden - the
	// shape of the device is the point.
	for (let i = 0; i < 16; i++) {
		wrap.appendChild(
			elc("span", {
				style:
					`border-radius:50%; background:${colors?.[i] ?? "var(--ds-led-powered-off)"};`,
			}),
		);
	}

	return wrap;
}

function unitRow(unit, selected, onSelect) {
	const state = STATES[unit.state] ?? STATES.connected;
	const isSelected = selected === unit.id;

	const name = elc("span", {
		style:
			"font:600 12px/1 var(--ds-font-mono); letter-spacing:0.04em; " +
			`color:${isSelected ? "var(--ds-text)" : "var(--ds-text-dim)"}; ` +
			"overflow:hidden; text-overflow:ellipsis; white-space:nowrap;",
		text: unit.nickname,
	});

	const meta = elc("span", {
		style:
			"font:400 10px/1 var(--ds-font-mono); letter-spacing:0.05em; " +
			"color:var(--ds-text-faint); overflow:hidden; text-overflow:ellipsis; white-space:nowrap;",
		text: unit.meta,
	});

	const label = elc("span", {
		style: "flex:1; min-width:0; display:flex; flex-direction:column; gap:4px; text-align:left;",
		children: [name, meta],
	});

	const children = [thumbnail(unit.colors), label];

	if (state.icon) {
		children.push(
			elc("span", {
				style: `flex:none; font:400 12px/1 var(--ds-font-mono); color:${state.color};`,
				text: state.icon,
				attrs: { "aria-hidden": "true" },
			}),
		);
	}

	return elc("button", {
		class: "ds-unit-row",
		attrs: {
			type: "button",
			"aria-pressed": String(isSelected),
			"aria-label": `${unit.nickname} - ${state.label}`,
		},
		style:
			"display:flex; align-items:center; gap:9px; width:100%; padding:8px 9px; " +
			"border-radius:6px; cursor:pointer; text-align:left; font:inherit; " +
			`border:1px solid ${isSelected ? state.color : "var(--ds-border)"}; ` +
			`background:${isSelected ? "var(--ds-bg-panel)" : "var(--ds-bg-raised)"};`,
		onClick: onSelect ? () => onSelect(unit.id) : undefined,
		children,
	});
}

function emptyState() {
	return elc("p", {
		style:
			"margin:0; padding:18px 4px; font:400 11px/1.7 var(--ds-font); " +
			"color:var(--ds-text-faint); text-align:left;",
		text: "No unit connected.",
	});
}

/**
 * @param p.units     [{id, nickname, meta, state, colors}]
 * @param p.selected  id of the selected unit, or null
 * @param p.onSelect  called with a unit id
 */
export function buildUnitSidebar(p) {
	const units = p.units ?? [];

	const heading = elc("div", {
		style:
			"display:flex; align-items:center; justify-content:space-between; " +
			"flex:none; padding:12px 13px 9px;",
		children: [
			elc("span", {
				style:
					"font:600 10px/1 var(--ds-font-mono); letter-spacing:0.16em; color:var(--ds-text-dim);",
				text: "UNITS",
			}),
			elc("span", {
				style: "font:400 10px/1 var(--ds-font-mono); color:var(--ds-text-faint);",
				text: String(units.length),
			}),
		],
	});

	const list = elc("div", {
		style: "display:flex; flex-direction:column; gap:5px;",
		children: units.length
			? units.map((u) => unitRow(u, p.selected, p.onSelect))
			: [emptyState()],
	});

	return elc("div", {
		style: "display:flex; flex-direction:column; min-height:0;",
		children: [
			heading,
			elc("div", {
				style: "min-height:0; overflow-y:auto; padding:0 8px 12px;",
				children: [list],
			}),
		],
	});
}
