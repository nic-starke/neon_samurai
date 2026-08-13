// twin.js - state and control panel for webui/twin.html, the "digital
// twin" device-geometry preview. Ported from a Claude Design Canvas
// prototype ("Twister Digital Twin.dc.html"); see twin-render.js for the
// rendering primitives this drives.
//
// This page is a standalone visual/design tool - it renders a plausible
// demo chassis and lets you tune its geometry, it does not talk to a real
// device or the config GUI's device model (see webui/js/device-model.js).
// Rebuilds the whole view on every change, same "clear and repopulate"
// pattern webui/js/ui.js uses elsewhere in this app, batched through
// requestAnimationFrame so dragging a slider doesn't rebuild sixteen
// encoders per pointermove event.

import { elc, hsvHex, buildEncoder } from "./twin-render.js";

const NUM_ENCODERS = 16;

// Fixed hardware-derived geometry the design was tuned against. "Reset to
// spec" restores exactly these fields (and only these - colour/selection
// state is untouched, matching the original design tool's behaviour).
const SPEC = {
	pitch: 136,
	edgeFirst: 92,
	cornerRadius: 36,
	bevelWidth: 14,
	bodySize: 112,
	knobSize: 71,
	capBaseDia: 18.5,
	capGripDiaBottom: 15,
	capGripDiaTop: 13.5,
	capRibCount: 19,
	capInnerDia: 11.3,
	lightX1: -35,
	lightY1: -45,
	lightX2: 130,
	lightY2: 145,
	capTopHue: 226,
	capTopSat: 5,
	capTopVal: 22,
	capInnerHue: 219,
	capInnerSat: 9,
	capInnerVal: 18,
	ledCount: 11,
	ledRadius: 46,
	ledSize: 10,
	ledArcSpan: 270,
	arcRadius: 46,
	arcWidth: 10,
	arcLength: 32,
	sideBtnW: 6,
	sideBtnH: 39,
	sideBtnSpacing: 76,
	sideBtnOffsetY: 0,
};

const PALETTE = ["#3bd6ff", "#ff3b6b", "#3bff8f", "#b23bff"];
const RAINBOW = ["#ff3b6b", "#ff9f3b", "#f4e04d", "#3bff8f", "#3bd6ff", "#5d7bff", "#b23bff", "#ff3bd6"];
const CAP_PRESETS = [
	[220, 15, 15],
	[225, 10, 26],
	[220, 6, 80],
	[345, 77, 100],
	[192, 77, 100],
];

const state = {
	...SPEC,
	accent: "#3bd6ff",
	capH: 220,
	capS: 15,
	capV: 15,
	uniform: true,
	rgbOff: false,
	sel: 5,
	selSide: -1,
};

const el = {
	chassis: document.getElementById("twin-chassis"),
	readouts: document.getElementById("twin-readouts"),
	scaleNote: document.getElementById("twin-scale-note"),
	sidebar: document.getElementById("twin-sidebar"),
};

function mm(px) {
	return Math.round((px / 4) * 100) / 100 + " mm";
}

let renderQueued = false;
function setState(patch) {
	Object.assign(state, patch);
	if (renderQueued) return;
	renderQueued = true;
	requestAnimationFrame(() => {
		renderQueued = false;
		render();
	});
}

// --- Control descriptors -------------------------------------------------

function ctl(field, label, min, max, step, unit) {
	const value = state[field];
	if (unit === "px") {
		return {
			label,
			min: min / 4,
			max: max / 4,
			step: 0.25,
			value: value / 4,
			display: mm(value),
			onInput: (v) => setState({ [field]: Number(v) * 4 }),
		};
	}
	return {
		label,
		min,
		max,
		step,
		value,
		display: value + (unit || ""),
		onInput: (v) => setState({ [field]: Number(v) }),
	};
}

function sections() {
	return [
		{
			title: "CHASSIS & GRID",
			controls: [
				ctl("pitch", "Knob pitch (centre→centre)", 100, 200, 1, "px"),
				ctl("edgeFirst", "Edge → first centre", 40, 120, 1, "px"),
				ctl("cornerRadius", "Corner radius", 0, 60, 1, "px"),
				ctl("bevelWidth", "Rubber bevel width", 4, 40, 1, "px"),
			],
		},
		{ title: "PLASTIC RING (BODY)", controls: [ctl("bodySize", "Ring diameter", 80, 160, 1, "px")] },
		{
			title: "ENCODER CAP (CHROMA CAP)",
			controls: [
				ctl("knobSize", "Cap footprint on panel", 40, 120, 1, "px"),
				ctl("capBaseDia", "Base diameter", 8, 24, 0.1, "mm"),
				ctl("capGripDiaBottom", "Grip diameter — bottom", 6, 24, 0.1, "mm"),
				ctl("capGripDiaTop", "Grip diameter — top", 6, 24, 0.1, "mm"),
				ctl("capInnerDia", "Inner recess diameter", 4, 20, 0.1, "mm"),
				ctl("capTopVal", "Top face brightness", 0, 100, 1, "%"),
				ctl("capInnerVal", "Inner disc brightness", 0, 100, 1, "%"),
				ctl("capRibCount", "Knurl rib count", 9, 31, 2, ""),
			],
		},
		{
			title: "INDICATOR LED RING",
			controls: [
				ctl("ledCount", "LED count", 6, 16, 1, ""),
				ctl("ledRadius", "Ring radius", 30, 80, 0.5, "px"),
				ctl("ledSize", "LED size", 3, 14, 0.5, "px"),
				ctl("ledArcSpan", "Arc span", 200, 330, 5, "°"),
			],
		},
		{
			title: "RGB INDICATOR",
			controls: [
				ctl("arcRadius", "Arc radius", 30, 80, 0.5, "px"),
				ctl("arcWidth", "Arc thickness", 3, 20, 0.5, "px"),
				ctl("arcLength", "Arc length", 8, 70, 1, "px"),
			],
		},
		{
			title: "PANEL LIGHTING (GLOBAL)",
			controls: [
				ctl("lightX1", "Light 1 — X", -60, 160, 1, "%"),
				ctl("lightY1", "Light 1 — Y", -60, 160, 1, "%"),
				ctl("lightX2", "Light 2 — X", -60, 160, 1, "%"),
				ctl("lightY2", "Light 2 — Y", -60, 160, 1, "%"),
			],
		},
		{
			title: "SIDE BUTTONS",
			controls: [
				ctl("sideBtnW", "Width", 6, 30, 1, "px"),
				ctl("sideBtnH", "Height", 16, 70, 1, "px"),
				ctl("sideBtnSpacing", "Spacing from centre", 40, 260, 1, "px"),
				ctl("sideBtnOffsetY", "Group offset (Y)", -80, 80, 1, "px"),
			],
		},
	];
}

// --- Stage (chassis + encoders) ------------------------------------------

function renderStage() {
	const s = state;
	const gridGap = s.pitch - s.bodySize;
	const chassisPad = s.edgeFirst - s.bodySize / 2;
	const chassisSize = 4 * s.bodySize + 3 * gridGap + 2 * chassisPad;

	// Two lights fixed to the panel, not to each cap: every encoder's
	// highlight angle is derived from its own position relative to the
	// two light points, so the knurl shading reads as one lit surface
	// rather than 16 identical stickers.
	const lx1 = (s.lightX1 / 100) * chassisSize;
	const ly1 = (s.lightY1 / 100) * chassisSize;
	const lx2 = (s.lightX2 / 100) * chassisSize;
	const ly2 = (s.lightY2 / 100) * chassisSize;
	const deg = (rad) => (rad * 180) / Math.PI;
	const capColor = hsvHex(s.capH, s.capS, s.capV);

	const chassis = elc("div", {
		style: `position:relative; width:${chassisSize}px; height:${chassisSize}px; flex-shrink:0; margin:0 auto;`,
	});

	const mid = chassisSize / 2 + s.sideBtnOffsetY;
	const centres = [mid - s.sideBtnSpacing, mid, mid + s.sideBtnSpacing];
	for (const side of ["L", "R"]) {
		centres.forEach((c, i) => {
			const idx = (side === "L" ? 0 : 3) + i;
			const btn = elc("button", {
				title: `${side === "L" ? "Left" : "Right"} Side ${i + 1}`,
				style:
					`position:absolute; ${side === "L" ? "left" : "right"}:-${s.sideBtnW}px; top:${c - s.sideBtnH / 2}px; ` +
					`width:${s.sideBtnW}px; height:${s.sideBtnH}px; border-radius:${side === "L" ? "6px 0 0 6px" : "0 6px 6px 0"}; ` +
					"border:none; padding:0; cursor:pointer; " +
					`background:${s.selSide === idx ? "linear-gradient(180deg,#5df0ff,#2b6f7c)" : "linear-gradient(180deg,#3a3f4a,#1a1d22)"}; ` +
					`box-shadow:inset 0 1px 0 rgba(255,255,255,0.12), ${side === "L" ? "-2px" : "2px"} 2px 5px rgba(0,0,0,0.55);`,
				onClick: () => setState({ selSide: idx, sel: -1 }),
			});
			chassis.appendChild(btn);
		});
	}

	const bevel = elc("div", {
		style: `position:absolute; inset:0; border-radius:${s.cornerRadius}px; background:linear-gradient(158deg,#2c2e34 0%,#191a1e 38%,#121316 68%,#23252a 100%); box-shadow:0 34px 70px rgba(0,0,0,0.6), inset 0 1px 0 rgba(255,255,255,0.06); padding:${s.bevelWidth}px; box-sizing:border-box;`,
	});
	chassis.appendChild(bevel);

	const faceRadius = Math.max(2, s.cornerRadius - s.bevelWidth);
	const facePad = Math.max(0, chassisPad - s.bevelWidth);
	const face = elc("div", {
		style: `width:100%; height:100%; border-radius:${faceRadius}px; background:linear-gradient(168deg,#1e2024 0%,#15161a 46%,#0f1012 78%,#17181c 100%); box-shadow:inset 0 1px 0 rgba(255,255,255,0.05), 0 1px 3px rgba(0,0,0,0.6); padding:${facePad}px; box-sizing:border-box;`,
	});
	bevel.appendChild(face);

	const grid = elc("div", {
		style: `display:grid; grid-template-columns:repeat(4, ${s.bodySize}px); grid-template-rows:repeat(4, ${s.bodySize}px); gap:${gridGap}px;`,
	});
	face.appendChild(grid);

	for (let i = 0; i < NUM_ENCODERS; i++) {
		const cx = chassisPad + s.bodySize / 2 + (i % 4) * s.pitch;
		const cy = chassisPad + s.bodySize / 2 + Math.floor(i / 4) * s.pitch;
		const a1 = deg(Math.atan2(cy - ly1, cx - lx1));
		const a2 = deg(Math.atan2(cy - ly2, cx - lx2));

		grid.appendChild(
			buildEncoder({
				bodySize: s.bodySize,
				knobSize: s.knobSize,
				capColor,
				capInnerDia: s.capInnerDia,
				capBaseDia: s.capBaseDia,
				capGripDiaBottom: s.capGripDiaBottom,
				capGripDiaTop: s.capGripDiaTop,
				capRibCount: s.capRibCount,
				capLightAngle: Math.round(a1 + 90),
				capLightOffset: Math.round(((a2 - a1) % 360 + 360) % 360),
				capTopHue: s.capTopHue,
				capTopSat: s.capTopSat,
				capTopVal: s.capTopVal,
				capInnerHue: s.capInnerHue,
				capInnerSat: s.capInnerSat,
				capInnerVal: s.capInnerVal,
				ledCount: s.ledCount,
				ledSize: s.ledSize,
				ledRadius: s.ledRadius,
				ledArcSpan: s.ledArcSpan,
				arcRadius: s.arcRadius,
				arcWidth: s.arcWidth,
				arcLength: s.arcLength,
				value: 12 + i * 7,
				max: 127,
				rgbColor: s.rgbOff ? "#c6c8cc" : s.uniform ? s.accent : RAINBOW[i % 8],
				selected: s.sel === i,
				showLabel: false,
				onSelect: () => setState({ sel: i, selSide: -1 }),
			}),
		);
	}

	el.chassis.replaceChildren(chassis);
	el.scaleNote.textContent = `${mm(chassisSize)} × ${mm(chassisSize)} · drawn at 4× scale (1 mm = 4 px)`;

	el.readouts.replaceChildren(
		elc("span", { text: `chassis ${mm(chassisSize)} · r ${mm(s.cornerRadius)}` }),
		elc("span", { text: `pitch ${mm(s.pitch)}` }),
		elc("span", { text: `cell ⌀${mm(s.bodySize)} · gap ${mm(gridGap)}` }),
		elc("span", { text: `edge → 1st centre ${mm(s.edgeFirst)}` }),
		elc("span", { text: `side y ${centres.map((c) => mm(c)).join(" / ")}` }),
	);
}

// --- Sidebar ---------------------------------------------------------------

function swatchButton({ color, selected, onClick, size = 28 }) {
	return elc("button", {
		style: `width:${size}px; height:${size}px; border-radius:50%; border:2px solid ${selected ? "#fff" : "transparent"}; background:${color}; cursor:pointer; padding:0;`,
		onClick,
	});
}

function toggleField(labelText, checked, onToggle) {
	return elc("label", {
		style: "display:flex; align-items:center; justify-content:space-between;",
		children: [
			elc("span", { style: "font-family:var(--twin-font-mono); font-size:10.5px; color:#9a92b0;", text: labelText }),
			elc("button", {
				style: `width:38px; height:21px; border-radius:11px; border:none; cursor:pointer; background:${checked ? "#3bff8f" : "#3a3f4a"}; position:relative;`,
				onClick: onToggle,
				children: [
					elc("span", {
						style: `position:absolute; top:2px; left:${checked ? "19px" : "2px"}; width:17px; height:17px; border-radius:50%; background:#fff;`,
					}),
				],
			}),
		],
	});
}

function sliderField(c) {
	return elc("label", {
		style: "display:flex; flex-direction:column; gap:6px;",
		children: [
			elc("span", {
				style: "display:flex; align-items:baseline; justify-content:space-between; font-family:var(--twin-font-mono); font-size:11px; color:#c9c2dd;",
				children: [
					elc("span", { text: c.label }),
					elc("span", { style: "color:#5df0ff;", text: String(c.display) }),
				],
			}),
			(() => {
				const input = elc("input", {
					attrs: { type: "range", min: c.min, max: c.max, step: c.step, value: c.value },
				});
				input.addEventListener("input", () => c.onInput(input.value));
				return input;
			})(),
		],
	});
}

function renderSidebar() {
	const s = state;
	const sidebar = elc("div", { style: "display:flex; flex-direction:column; gap:24px;" });

	sidebar.appendChild(
		elc("div", {
			style: "display:flex; align-items:baseline; justify-content:space-between;",
			children: [
				elc("span", { class: "twin-heading", text: "Geometry" }),
				elc("button", { class: "twin-reset", text: "Reset to spec", onClick: () => setState({ ...SPEC }) }),
			],
		}),
	);

	// RGB colour (demo only - the real config GUI drives this per-encoder
	// from the device's own vmap HSV, see webui/js/ui.js).
	sidebar.appendChild(
		elc("div", {
			style: "display:flex; flex-direction:column; gap:8px;",
			children: [
				elc("span", { class: "twin-label", text: "RGB COLOUR (DEMO)" }),
				elc("div", {
					style: "display:flex; gap:9px;",
					children: PALETTE.map((color) =>
						swatchButton({ color, selected: s.accent === color, onClick: () => setState({ accent: color }) }),
					),
				}),
				toggleField("RGB LEDs off (unlit grey)", s.rgbOff, () => setState({ rgbOff: !s.rgbOff })),
				toggleField("Uniform colour", s.uniform, () => setState({ uniform: !s.uniform })),
			],
		}),
	);

	// Cap colour.
	const capHex = hsvHex(s.capH, s.capS, s.capV).toUpperCase();
	sidebar.appendChild(
		elc("div", {
			style: "display:flex; flex-direction:column; gap:8px; border-top:1px solid rgba(255,255,255,0.08); padding-top:18px;",
			children: [
				elc("div", {
					style: "display:flex; align-items:center; justify-content:space-between;",
					children: [
						elc("span", { class: "twin-label", text: "ENCODER CAP COLOUR" }),
						elc("span", {
							style: "display:flex; align-items:center; gap:8px;",
							children: [
								elc("span", {
									style: `width:20px; height:20px; border-radius:5px; background:${capHex}; border:1px solid rgba(255,255,255,0.18);`,
								}),
								elc("span", { style: "font-family:var(--twin-font-mono); font-size:10.5px; color:#5df0ff;", text: capHex }),
							],
						}),
					],
				}),
				sliderField({
					label: "Hue",
					min: 0,
					max: 360,
					step: 1,
					value: s.capH,
					display: s.capH + "°",
					onInput: (v) => setState({ capH: Number(v) }),
				}),
				sliderField({
					label: "Saturation",
					min: 0,
					max: 100,
					step: 1,
					value: s.capS,
					display: s.capS + "%",
					onInput: (v) => setState({ capS: Number(v) }),
				}),
				sliderField({
					label: "Value",
					min: 0,
					max: 100,
					step: 1,
					value: s.capV,
					display: s.capV + "%",
					onInput: (v) => setState({ capV: Number(v) }),
				}),
				elc("div", {
					style: "display:flex; gap:9px; margin-top:4px;",
					children: CAP_PRESETS.map(([h, sat, v]) => {
						const color = hsvHex(h, sat, v);
						const selected = s.capH === h && s.capS === sat && s.capV === v;
						return swatchButton({ color, selected, size: 26, onClick: () => setState({ capH: h, capS: sat, capV: v }) });
					}),
				}),
				elc("span", {
					style: "font-family:var(--twin-font-mono); font-size:9.5px; color:#57506d;",
					text: "Base, midsection and knurl tones derive from this one colour.",
				}),
			],
		}),
	);

	for (const sec of sections()) {
		sidebar.appendChild(
			elc("div", {
				style: "display:flex; flex-direction:column; gap:14px; border-top:1px solid rgba(255,255,255,0.08); padding-top:18px;",
				children: [
					elc("span", { class: "twin-label", text: sec.title }),
					...sec.controls.map(sliderField),
				],
			}),
		);
	}

	el.sidebar.replaceChildren(sidebar);
}

// --- Entry -----------------------------------------------------------------

function render() {
	renderStage();
	renderSidebar();
}

render();
