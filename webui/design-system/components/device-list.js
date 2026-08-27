// The list of connected devices, down the left of the editor.
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
import { buildMiniDevice } from "./mini-device.js";

/** Shared with the editor, which replaces this element as the device moves. */
export const MINI_SIZE = 78;

// State determines the row's accent and its trailing glyph. Anything the
// editor cannot configure reads as amber, so "needs attention" is one colour.
// `label` is the accessible name and tooltip; `meta` is the row's second line,
// used when the device has nothing better to say there (a connected one shows
// its firmware version instead).
const STATES = {
	detected: { color: "var(--ds-border)", icon: "", label: "Detected - click to connect", meta: "not connected" },
	identifying: { color: "var(--ds-warning)", icon: "…", label: "Connecting", meta: "connecting…" },
	connected: { color: "var(--ds-accent)", icon: "", label: "Connected - click to release", meta: "connected" },
	// The device is there and something else is holding its port. Distinct
	// from an error: nothing is broken and the fix is elsewhere.
	busy: { color: "var(--ds-amber)", icon: "⊘", label: "In use by another application", meta: "in use elsewhere" },
	failed: { color: "var(--ds-danger)", icon: "✕", label: "Could not be opened", meta: "could not be opened" },
	bootloader: { color: "var(--ds-amber)", icon: "⚑", label: "Bootloader", meta: "bootloader" },
	djtt: { color: "var(--ds-text-dim)", icon: "◈", label: "Stock DJTT firmware", meta: "stock firmware" },
	incompatible: { color: "var(--ds-amber)", icon: "!", label: "Incompatible firmware", meta: "incompatible" },
};

function unitRow(unit, selected, onSelect) {
	const state = STATES[unit.state] ?? STATES.detected;
	const isSelected = selected === unit.id;

	const name = elc("span", {
		style:
			"font:600 12px/1 var(--ds-font-mono); letter-spacing:0.04em; " +
			`color:${isSelected ? "var(--ds-text)" : "var(--ds-text-dim)"}; ` +
			"overflow:hidden; text-overflow:ellipsis; white-space:nowrap;",
		text: unit.name,
	});

	const meta = elc("span", {
		style:
			"font:400 10px/1 var(--ds-font-mono); letter-spacing:0.05em; " +
			"color:var(--ds-text-faint); overflow:hidden; text-overflow:ellipsis; white-space:nowrap;",
		text: unit.meta ?? state.meta,
	});

	const label = elc("span", {
		style: "flex:1; min-width:0; display:flex; flex-direction:column; gap:4px; text-align:left;",
		children: [name, meta],
	});

	const children = [
		buildMiniDevice({
			size: MINI_SIZE,
			key: unit.id,
			encoders: unit.encoders,
			accent: isSelected ? state.color : undefined,
		}),
		label,
	];

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
		title: state.label,
		attrs: {
			type: "button",
			"aria-pressed": String(isSelected),
			"aria-label": `${unit.name} - ${state.label}`,
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
		text: "No devices connected.",
	});
}

/**
 * @param p.units     [{id, name, state, meta, encoders}]
 * @param p.selected  id of the selected unit, or null
 * @param p.onSelect  called with a unit id
 */
export function buildDeviceList(p) {
	const units = p.units ?? [];

	const heading = elc("div", {
		style:
			"display:flex; align-items:center; justify-content:space-between; " +
			"flex:none; padding:12px 13px 9px;",
		children: [
			elc("span", {
				style:
					"font:600 10px/1 var(--ds-font-mono); letter-spacing:0.16em; color:var(--ds-text-dim);",
				text: "DEVICES",
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
