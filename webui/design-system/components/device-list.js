import { elc } from "./dom.js";

const STATES = {
	detected: { level: "off", label: "Not connected", pill: "OFFLINE" },
	identifying: { level: "identifying", label: "Connecting", pill: "CONNECTING" },
	connected: { level: "ok", label: "Connected", pill: "CONNECTED" },
	busy: { level: "warn", label: "In use by another application", pill: "IN USE" },
	failed: { level: "danger", label: "Could not be opened", pill: "FAILED" },
	bootloader: { level: "warn", label: "Bootloader", pill: "BOOTLOADER" },
	djtt: { level: "warn", label: "Stock DJTT firmware", pill: "STOCK FIRMWARE" },
	incompatible: { level: "warn", label: "Incompatible firmware", pill: "INCOMPATIBLE" },
};

const CONNECTABLE = new Set(["detected", "busy", "failed"]);
const EXPANDABLE = new Set(["connected", "bootloader"]);

const LEVEL_COLOR = {
	off: "var(--ds-text-faint)",
	ok: "var(--ds-accent)",
	warn: "var(--ds-amber)",
	danger: "var(--ds-danger)",
	identifying: "var(--ds-text-faint)",
};

function stateBar(level, updateAvailable) {
	const effectiveLevel = updateAvailable ? "warn" : level;
	const modifier = effectiveLevel === "off" ? "" : ` ds-unit-bar--${effectiveLevel}`;
	return elc("span", {
		class: `ds-unit-bar${modifier}`,
		attrs: { "aria-hidden": "true" },
	});
}

function nameSpan(unit, color) {
	return elc("span", {
		style:
			"flex:1; min-width:0; font:var(--ds-text-title); letter-spacing:0.04em; " +
			`color:${color}; overflow:hidden; text-overflow:ellipsis; white-space:nowrap;`,
		text: unit.name,
	});
}

function connectSlot(unit, isConnecting, onSelect) {
	if (isConnecting) {
		const fraction = unit.progress ?? 0;
		return elc("button", {
			class: "ds-unit-actions__link ds-unit-actions__link--connect",
			attrs: { type: "button", "aria-label": `Connecting ${unit.name}`, "aria-disabled": "true" },
			children: [
				elc("span", {
					class: "ds-unit-actions__link-fill",
					attrs: { "data-progress": unit.id },
					style: `width:${Math.round(fraction * 100)}%;`,
				}),
			],
		});
	}

	return elc("button", {
		class: "ds-unit-actions__link ds-unit-actions__link--connect",
		attrs: { type: "button", "aria-label": `Connect ${unit.name}` },
		text: "CONNECT",
		onClick: () => onSelect?.(unit.id),
	});
}

function fact(key, value, unit, onUpdate) {
	const children = [
		elc("span", { class: "ds-unit-fact__key", text: key }),
		elc("span", { class: "ds-unit-fact__value", text: value }),
	];

	if (unit?.updateAvailable) {
		children.push(
			elc("button", {
				class: "ds-unit-fact__update",
				attrs: { type: "button" },
				text: "UPDATE",
				onClick: (ev) => {
					ev.stopPropagation();
					onUpdate?.();
				},
			}),
		);
	}

	return elc("div", { class: "ds-unit-fact", children });
}

function unitDetail(unit, p) {
	const isConnected = unit.state === "connected";
	const children = [];

	if (unit.firmwareVersion || unit.updateAvailable) {
		children.push(fact("FIRMWARE", unit.firmwareVersion ?? "—", unit, p.onUpdate));
	}

	if (unit.state === "bootloader") {
		children.push(
			elc("p", {
				style: "margin:5px 0 0; font:var(--ds-text-body); color:var(--ds-text-dim);",
				text: "No MIDI interface - recognised only by its bootloader record.",
			}),
		);
	}

	if (isConnected) {
		children.push(
			elc("div", {
				class: "ds-unit-actions",
				children: [
					elc("button", {
						class: "ds-unit-actions__link ds-unit-actions__link--disconnect",
						attrs: { type: "button" },
						text: "DISCONNECT",
						onClick: (ev) => {
							ev.stopPropagation();
							p.onSelect?.(unit.id);
						},
					}),
				],
			}),
		);
	}

	return elc("div", {
		class: "ds-unit-detail",
		attrs: { id: `device-detail-${unit.id}`, role: "region", "aria-label": `${unit.name} details` },
		children,
	});
}

function connectRow(unit, p) {
	const state = STATES[unit.state] ?? STATES.detected;
	const isConnecting = unit.state === "identifying";

	return elc("div", {
		class: "ds-unit-row",
		title: state.label,
		style:
			"position:relative; display:flex; align-items:center; gap:8px; width:100%; padding:10px 9px 10px 16px; " +
			"border-radius:6px; border:1px solid var(--ds-border); background:var(--ds-bg-raised);",
		children: [stateBar(state.level, unit.updateAvailable), nameSpan(unit, "var(--ds-text-dim)"), connectSlot(unit, isConnecting, p.onSelect)],
	});
}

function expandableRow(unit, p) {
	const state = STATES[unit.state] ?? STATES.detected;
	const isExpanded = p.expandedId === unit.id;
	const borderColor = isExpanded ? (unit.updateAvailable ? "var(--ds-amber)" : LEVEL_COLOR[state.level]) : "var(--ds-border)";
	const title = unit.updateAvailable ? `${state.label} - firmware update available` : state.label;

	const header = elc("button", {
		class: "ds-unit-row-header",
		title,
		attrs: {
			type: "button",
			"aria-expanded": String(isExpanded),
			"aria-controls": `device-detail-${unit.id}`,
			"aria-label": `${unit.name} - ${title}`,
		},
		style:
			"position:relative; display:flex; align-items:center; gap:8px; width:100%; padding:10px 9px 10px 16px; " +
			"border:0; border-radius:6px; text-align:left; font:inherit; cursor:pointer;",
		onClick: () => p.onExpand?.(unit.id),
		children: [stateBar(state.level, unit.updateAvailable), nameSpan(unit, isExpanded ? "var(--ds-text)" : "var(--ds-text-dim)")],
	});

	return elc("div", {
		class: "ds-unit-row",
		style: `border-radius:6px; border:1px solid ${borderColor}; background:${isExpanded ? "var(--ds-bg-selected)" : "var(--ds-bg-raised)"};`,
		children: isExpanded ? [header, unitDetail(unit, p)] : [header],
	});
}

function staticRow(unit) {
	const state = STATES[unit.state] ?? STATES.detected;

	return elc("div", {
		class: "ds-unit-row",
		title: state.label,
		style:
			"position:relative; display:flex; align-items:center; gap:8px; width:100%; padding:10px 9px 10px 16px; " +
			"border-radius:6px; border:1px solid var(--ds-border); background:var(--ds-bg-raised);",
		children: [stateBar(state.level, unit.updateAvailable), nameSpan(unit, "var(--ds-text-dim)")],
	});
}

function unitRow(unit, p) {
	if (unit.state === "identifying" || CONNECTABLE.has(unit.state)) return connectRow(unit, p);
	if (EXPANDABLE.has(unit.state)) return expandableRow(unit, p);
	return staticRow(unit);
}

function emptyState() {
	return elc("p", {
		style: "margin:0; padding:18px 4px; font:var(--ds-text-body); color:var(--ds-text-faint); text-align:left;",
		text: "No devices detected.",
	});
}

function attentionCount(units) {
	return units.filter((u) => u.state === "failed" || u.state === "busy" || u.state === "bootloader" || u.updateAvailable).length;
}

function header(units, onRescan) {
	const count = attentionCount(units);

	const children = [
		elc("span", {
			style: "font:var(--ds-text-heading); letter-spacing:0.16em; color:var(--ds-text-dim);",
			text: "DEVICES",
		}),
	];
	if (count) children.push(elc("span", { class: "ds-unit-attn", text: String(count) }));
	children.push(elc("span", { style: "flex:1;" }));
	if (onRescan) {
		children.push(
			elc("button", {
				class: "ds-unit-rescan",
				attrs: { type: "button", title: "Look for devices again" },
				text: "RESCAN",
				onClick: onRescan,
			}),
		);
	}

	return elc("div", {
		style: "display:flex; align-items:center; gap:8px; flex:none; padding:12px 13px 9px;",
		children,
	});
}

export function buildDeviceList(p) {
	const units = p.units ?? [];

	const list = elc("div", {
		style: "display:flex; flex-direction:column; gap:5px;",
		children: units.length ? units.map((u) => unitRow(u, p)) : [emptyState()],
	});

	return elc("div", {
		style: "display:flex; flex-direction:column; min-height:0;",
		children: [
			header(units, p.onRescan),
			elc("div", {
				style: "min-height:0; overflow-y:auto; padding:0 8px 12px;",
				children: [list],
			}),
			elc("div", { class: "ds-unit-legend", text: "BAR = STATE · AMBER BORDER = NEEDS YOU" }),
		],
	});
}
