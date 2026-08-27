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
import { buildToggle } from "./toggle.js";

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

const TABS = [
	{ id: "device", label: "DEVICE" },
	{ id: "settings", label: "SETTINGS" },
];

function tabStrip(active, onSelect) {
	return elc("div", {
		attrs: { role: "tablist" },
		style:
			"display:flex; gap:2px; flex:none; padding:10px 11px 0; " +
			"border-bottom:1px solid var(--ds-border);",
		children: TABS.map((t) => {
			const isActive = t.id === active;
			return elc("button", {
				attrs: { type: "button", role: "tab", "aria-selected": String(isActive) },
				style:
					"padding:8px 12px; border:none; border-bottom:2px solid " +
					`${isActive ? "var(--ds-accent)" : "transparent"}; ` +
					"background:none; cursor:pointer; " +
					"font:600 10px/1 var(--ds-font-mono); letter-spacing:0.12em; " +
					`color:${isActive ? "var(--ds-text)" : "var(--ds-text-faint)"};`,
				text: t.label,
				onClick: () => onSelect?.(t.id),
			});
		}),
	});
}

function section(title, children) {
	return elc("div", {
		style: "margin-top:17px;",
		children: [
			elc("div", {
				style:
					"font:600 10px/1 var(--ds-font-mono); letter-spacing:0.16em; color:var(--ds-accent);",
				text: title,
			}),
			elc("div", { style: "margin-top:11px;", children }),
		],
	});
}

/**
 * Settings for the editor, not the device.
 *
 * Reachable whether or not anything is connected - the auto-connect setting in
 * particular is one someone will want to change while nothing is attached.
 */
function settingsTab(p) {
	return elc("div", {
		style: "padding:4px 13px 24px;",
		children: [
			section("CONNECTION", [
				buildToggle({
					label: "Connect automatically",
					description:
						"Open a device as soon as it is found, when only one is attached. " +
						"Opening it takes exclusive use of its MIDI port, so anything else " +
						"already using the device will lose it.",
					checked: p.settings?.autoConnect ?? false,
					onChange: (value) => p.onSetting?.("autoConnect", value),
				}),
			]),
		],
	});
}

function deviceTab(p) {
	if (!p.connected || !p.deviceInfo) {
		return elc("div", {
			style: "padding:26px 19px;",
			children: [
				elc("p", {
					style: "margin:0; font:400 11.5px/1.7 var(--ds-font); color:var(--ds-text-faint);",
					text: "Connect a device to see what it reports about itself.",
				}),
			],
		});
	}

	return deviceSummary(p);
}

/**
 * @param p.tab         which tab is showing
 * @param p.onTab       called with a tab id
 * @param p.connected   whether a device is attached
 * @param p.deviceInfo  the sysex device-info reply, when there is one
 * @param p.bank        zero-based bank being viewed
 * @param p.settings    current editor settings
 * @param p.onSetting   called with (name, value)
 */
export function buildInspector(p) {
	const tab = p.tab ?? "device";

	return elc("div", {
		style: "display:flex; flex-direction:column; height:100%; min-height:0;",
		children: [
			tabStrip(tab, p.onTab),
			elc("div", {
				style: "flex:1; min-height:0; overflow-y:auto;",
				children: [tab === "settings" ? settingsTab(p) : deviceTab(p)],
			}),
		],
	});
}

function deviceSummary(p) {

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
