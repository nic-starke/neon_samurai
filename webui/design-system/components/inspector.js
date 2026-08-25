// The properties panel down the right of the editor.
//
// It has nothing to edit yet - the protocol is read-only - so it shows the
// device's own description of itself. Controls arrive with the write path;
// until then the panel says what is true rather than showing fields that
// cannot be changed.
//
// With nothing attached it renders nothing at all: the sidebar already says
// there are no devices, and saying it twice on one screen is noise.

import { elc } from "./dom.js";

function heading(text) {
	return elc("div", {
		style:
			"margin-top:17px; font:600 10px/1 var(--ds-font-mono); letter-spacing:0.16em; " +
			"color:var(--ds-accent);",
		text,
	});
}

function row(key, value) {
	return elc("div", {
		style:
			"display:flex; align-items:baseline; justify-content:space-between; gap:10px; padding:4px 0;",
		children: [
			elc("span", {
				style:
					"font:400 10px/1.5 var(--ds-font-mono); letter-spacing:0.1em; color:var(--ds-text-dim);",
				text: key,
			}),
			elc("span", {
				style:
					"font:400 11px/1.5 var(--ds-font-mono); color:var(--ds-text); text-align:right;",
				text: value,
			}),
		],
	});
}

function paragraph(text) {
	return elc("p", {
		style:
			"margin:11px 0 0; font:400 11.5px/1.7 var(--ds-font); color:var(--ds-text-dim);",
		text,
	});
}

function guidance() {
	return elc("div", {
		style: "padding:26px 6px;",
		children: [
			elc("div", {
				style:
					"font:600 11px/1.4 var(--ds-font-mono); letter-spacing:0.1em; color:var(--ds-text-dim);",
				text: "NOTHING SELECTED",
			}),
			paragraph(
				"The grid shows what the device is doing right now - turn an encoder and " +
					"it moves here too.",
			),
			paragraph(
				"Selecting and editing are not wired up yet, so nothing here can be " +
					"changed from the editor.",
			),
		],
	});
}

/**
 * @param p.connected   whether a unit is attached
 * @param p.deviceInfo  the sysex device-info reply, when there is one
 * @param p.bank        zero-based bank being viewed
 */
export function buildInspector(p) {
	if (!p.connected || !p.deviceInfo) return elc("div");

	const info = p.deviceInfo;

	const summary = elc("div", {
		style:
			"padding:13px; border:1px solid var(--ds-border); border-radius:6px; " +
			"background:var(--ds-bg-inset);",
		children: [
			row("FIRMWARE", info.fwVersion ?? "unknown"),
			row("ENCODERS", String(info.numEncoders ?? "?")),
			row("BANKS", String(info.numBanks ?? "?")),
			row("VIEWING", `BANK ${(p.bank ?? 0) + 1}`),
		],
	});

	return elc("div", {
		style: "padding:15px 13px 24px;",
		children: [heading("DEVICE"), elc("div", { style: "margin-top:9px;", children: [summary] }), guidance()],
	});
}
