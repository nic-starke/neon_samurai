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
import { buildTooltip } from "./tooltip.js";

/** Shared with the editor, which replaces this element as the device moves. */
export const MINI_SIZE = 78;

// How long a connected row must be held before it releases. Long enough that
// brushing the row does not disconnect it by accident; short enough that
// doing it on purpose does not feel like a fight with the UI.
const HOLD_TO_DISCONNECT_MS = 500;

// `label` is the accessible name and the native title (a plain-text fallback
// for anything the custom banner below cannot reach - screen readers, a
// browser tooltip on long hover). `meta` is the row's second line, used when
// the device has nothing better to say there (a connected one shows its
// firmware version instead). `dot` is the connection-status glyph in the
// corner - "ok" (green, pulsing: a live connection), "warn" (amber, marked
// "!"), or "off" (grey, flat: nothing yet to report).
const STATES = {
	detected: { dot: "off", label: "click to connect", meta: "click to connect..." },
	identifying: { dot: "off", label: "Connecting", meta: "connecting…" },
	connected: { dot: "ok", label: "Connected - hold to release", meta: "connected" },
	// The device is there and something else is holding its port. Distinct
	// from an error: nothing is broken and the fix is elsewhere.
	busy: { dot: "warn", label: "In use by another application", meta: "in use elsewhere" },
	failed: { dot: "warn", label: "Could not be opened", meta: "could not be opened" },
	bootloader: { dot: "warn", label: "Bootloader", meta: "bootloader" },
	djtt: { dot: "warn", label: "Stock DJTT firmware", meta: "stock firmware" },
	incompatible: { dot: "warn", label: "Incompatible firmware", meta: "incompatible" },
};

// A single click retries the same connect a fresh device would get -
// registry.connect() does not care what the previous state was. Bootloader
// and DJTT need their own flows (spec 4.2, 8), not implemented yet, so they
// get the status dot and the informational banner but not a click.
const CONNECTABLE = new Set(["detected", "busy", "failed"]);

const DOT_COLOR = {
	ok: "var(--ds-accent)",
	warn: "var(--ds-amber)",
	off: "var(--ds-text-faint)",
};

function statusDot(state) {
	return elc("span", {
		class: `ds-status-dot ds-status-dot--${state.dot}`,
		attrs: { "aria-hidden": "true" },
		text: state.dot === "warn" ? "!" : "",
	});
}

/**
 * The one banner style used for everything a row has to say when hovered or
 * held - a rounded bar across the row's centre, not a small popup pinned to
 * whichever glyph triggered it. There used to be two of these (a pill by the
 * status dot, a circle over the mini device); a row only ever needs one
 * message at a time, so now there is only one banner.
 */
function rowBanner(text, color) {
	return elc("span", {
		class: "ds-unit-row__banner",
		attrs: { "aria-hidden": "true" },
		children: [buildTooltip({ text, color })],
	});
}

/**
 * Wire up press-and-hold-to-disconnect on a connected row.
 *
 * A plain click no longer disconnects - releasing a device is easy to do by
 * accident otherwise, sitting right next to selecting one. Holding uses the
 * same bar a connect fills, coloured for a destructive action instead, driven
 * by a CSS transition timed to match the real timeout exactly rather than by
 * polling.
 */
function attachHold(row, fill, onComplete) {
	let timer = null;

	const start = (ev) => {
		if (ev.button !== undefined && ev.button !== 0) return;
		fill.parentElement.classList.add("ds-unit-progress--active");
		fill.classList.add("ds-unit-progress__fill--danger", "ds-unit-progress__fill--hold");
		// Forces the 0% state to paint before the transition to 100% is
		// requested, so the bar always fills from empty rather than from
		// wherever a previous, released hold left it.
		fill.style.width = "0%";
		void fill.offsetWidth;
		fill.style.width = "100%";
		timer = setTimeout(() => {
			timer = null;
			onComplete();
		}, HOLD_TO_DISCONNECT_MS);
	};

	const cancel = () => {
		if (timer === null) return;
		clearTimeout(timer);
		timer = null;
		fill.parentElement.classList.remove("ds-unit-progress--active");
		fill.classList.remove("ds-unit-progress__fill--hold");
		fill.style.width = "0%";
	};

	row.addEventListener("pointerdown", start);
	row.addEventListener("pointerup", cancel);
	row.addEventListener("pointerleave", cancel);
	row.addEventListener("pointercancel", cancel);
}

function unitRow(unit, selected, onSelect) {
	const state = STATES[unit.state] ?? STATES.detected;
	const isSelected = selected === unit.id;
	const connectable = CONNECTABLE.has(unit.state);
	const isConnected = unit.state === "connected";

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
		style: "display:flex; flex-direction:column; gap:4px; text-align:left;",
		children: [name, meta],
	});

	// Keyed so the editor can update its width directly while connecting,
	// instead of rebuilding the row on every progress tick - loadFromDevice()
	// reports progress once per request, easily 100+ times over one connect,
	// which was tearing this element down and restarting its stripe
	// animation from frame zero before a single cycle could complete.
	const progressFill = elc("span", {
		class: "ds-unit-progress__fill",
		attrs: { "data-progress": unit.id },
		style:
			unit.progress === null || unit.progress === undefined
				? "width:0;"
				: `width:${Math.round(unit.progress * 100)}%;`,
	});
	// Visible only while something is actually happening - either connecting
	// (unit.progress is set) or mid hold-to-disconnect, toggled directly by
	// attachHold() below. An always-on empty track under every idle row read
	// as clutter.
	const progressActive = unit.progress !== null && unit.progress !== undefined;
	const progress = elc("span", {
		class: `ds-unit-progress${progressActive ? " ds-unit-progress--active" : ""}`,
		children: [progressFill],
	});

	// name/meta at the top, the bar pushed to the bottom by margin-top:auto -
	// both confined to this column, which is also all the banner below
	// covers, so neither ever draws over the mini device.
	const right = elc("span", {
		class: "ds-unit-right",
		style: "flex:1; min-width:0; align-self:stretch; display:flex; flex-direction:column; position:relative;",
		children: [label, progress],
	});

	// Not shown mid-connect: the bar already says that, and a row cannot
	// usefully be clicked or held while it is still identifying.
	if (connectable) right.appendChild(rowBanner("CONNECT", "var(--ds-accent)"));
	else if (isConnected) right.appendChild(rowBanner("HOLD TO DISCONNECT", "var(--ds-danger)"));
	else if (unit.state !== "identifying") right.appendChild(rowBanner(state.label, DOT_COLOR[state.dot]));

	const mini = elc("span", {
		class: "ds-unit-mini",
		children: [
			buildMiniDevice({
				size: MINI_SIZE,
				key: unit.id,
				encoders: unit.encoders,
				// A light neutral ring, not the accent, and drawn outside the
				// mini's own edge rather than straddling it - see mini-device.js.
				outerStroke: isConnected ? "var(--ds-mini-connected-ring)" : undefined,
			}),
		],
	});

	const children = [mini, right, statusDot(state)];

	const row = elc("button", {
		class: `ds-unit-row${connectable ? " ds-unit-row--connectable" : ""}`,
		title: state.label,
		attrs: {
			type: "button",
			"aria-pressed": String(isSelected),
			"aria-label": `${unit.name} - ${state.label}`,
		},
		style:
			"position:relative; display:flex; align-items:center; gap:9px; width:100%; padding:10px 9px 12px; " +
			"border-radius:6px; cursor:pointer; text-align:left; font:inherit; " +
			`border:1px solid ${isSelected ? DOT_COLOR[state.dot] : "var(--ds-border)"}; ` +
			`background:${isSelected ? "var(--ds-bg-panel)" : "var(--ds-bg-raised)"};`,
		// A connected row disconnects only via the hold gesture wired below -
		// see attachHold(). Every other clickable state still connects on a
		// plain click, unchanged.
		onClick: connectable && onSelect ? () => onSelect(unit.id) : undefined,
		children,
	});

	if (isConnected && onSelect) attachHold(row, progressFill, () => onSelect(unit.id));

	return row;
}

function emptyState() {
	return elc("p", {
		style:
			"margin:0; padding:18px 4px; font:400 11px/1.7 var(--ds-font); " +
			"color:var(--ds-text-faint); text-align:left;",
		text: "No devices detected.",
	});
}

/**
 * @param p.units     [{id, name, state, meta, encoders, progress}]
 *                     progress is 0-1 while connecting, otherwise null.
 * @param p.selected  id of the selected unit, or null
 * @param p.onSelect  called with a unit id - on click to connect, or once a
 *                     connected row's hold-to-disconnect gesture completes
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
