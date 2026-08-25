// State and control panel for webui/twin.html, the standalone geometry-
// tuning preview - demo/design tool, doesn't talk to a real device. See
// js/editor.js for the real-device render using the same components.

import { signal, effect, batch } from "./vendor/signals-core.js";
import { GEOMETRY } from "../design-system/geometry.js";
import {
  elc,
  hsvHex,
  buildDeviceChassis,
  computeLitMask,
  computeLedBrightness,
  computeDetentColorOverride,
  LedDisplayMode,
  ENC_MAX,
} from "../design-system/components/index.js";

// "Reset to spec" restores exactly these fields - colour/selection state is
// untouched. The shared chassis geometry comes from design-system/
// geometry.js, which the live view uses too; only the cap-colour fields
// below are specific to this tuning tool.
const SPEC = {
  ...GEOMETRY,
  capTopHue: 220,
  capTopSat: 13,
  capTopVal: 17,
  capInnerHue: 219,
  capInnerSat: 9,
  capInnerVal: 18,
};

const PALETTE = ["#3bd6ff", "#ff3b6b", "#3bff8f", "#b23bff"];
const RAINBOW = [
  "#ff3b6b",
  "#ff9f3b",
  "#f4e04d",
  "#3bff8f",
  "#3bd6ff",
  "#5d7bff",
  "#b23bff",
  "#ff3bd6",
];
const CAP_PRESETS = [
  [220, 13, 17], // default - matte dark plastic, #26282B
  [225, 10, 26],
  [220, 6, 80],
  [345, 77, 100],
  [192, 77, 100],
];

// One signal per field rather than one big object signal; render() does a
// full rebuild regardless, but is a single effect over all fields,
// batched so a multi-field change (e.g. "Reset to spec") triggers one
// rebuild, not one per field.
const state = {};
for (const [k, v] of Object.entries({
  ...SPEC,
  accent: "#3bd6ff",
  capH: 220,
  capS: 13,
  capV: 17,
  uniform: true,
  rgbOff: false,
  sel: 5,
  selSide: -1,
  // Demo indicator/rotation state - shared across all 16 encoders in
  // this preview (the live twin drives these per-encoder from real
  // device state instead - see js/editor.js).
  demoPosition: 160,
  demoDisplayMode: LedDisplayMode.SINGLE,
  demoDetent: false,
  demoRbRed: 255,
  demoRbBlue: 0,
  demoVmapActive: 0,
})) {
  state[k] = signal(v);
}

function setState(patch) {
  batch(() => {
    for (const [k, v] of Object.entries(patch)) state[k].value = v;
  });
}

const el = {
  chassis: document.getElementById("twin-chassis"),
  readouts: document.getElementById("twin-readouts"),
  scaleNote: document.getElementById("twin-scale-note"),
  sidebar: document.getElementById("twin-sidebar"),
};

function mm(px) {
  return Math.round((px / 4) * 100) / 100 + " mm";
}

// --- Control descriptors -------------------------------------------------

function ctl(field, label, min, max, step, unit) {
  const value = state[field].value;
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
    {
      title: "PLASTIC RING (BODY)",
      controls: [ctl("bodySize", "Ring diameter", 80, 160, 1, "px")],
    },
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
  const s = {};
  for (const k of Object.keys(state)) s[k] = state[k].value;

  const gridGap = s.pitch - s.bodySize;
  const chassisPad = s.edgeFirst - s.bodySize / 2;
  const capColor = hsvHex(s.capH, s.capS, s.capV);

  // All 16 encoders in this demo share one position/mode/detent, driven
  // by the sidebar controls - the live twin drives these per-encoder
  // from real device state instead (see js/editor.js).
  const maskArgs = {
    position: s.demoPosition,
    displayMode: s.demoDisplayMode,
    detent: s.demoDetent,
  };
  const litMask = computeLitMask(maskArgs);
  const ledBrightness =
    s.demoDisplayMode === LedDisplayMode.MULTI_PWM
      ? computeLedBrightness(maskArgs)
      : undefined;
  const ledColorOverride = computeDetentColorOverride({
    position: s.demoPosition,
    detent: s.demoDetent,
    rb: { r: s.demoRbRed, b: s.demoRbBlue },
  });
  const knobRotation =
    -(s.ledArcSpan / 2) + (s.demoPosition / ENC_MAX) * s.ledArcSpan;

  const { el: chassisEl, chassisSize } = buildDeviceChassis(
    s,
    (i, knurlLight) => ({
      bodySize: s.bodySize,
      knobSize: s.knobSize,
      capColor,
      capInnerDia: s.capInnerDia,
      capBaseDia: s.capBaseDia,
      capGripDiaBottom: s.capGripDiaBottom,
      capGripDiaTop: s.capGripDiaTop,
      capRibCount: s.capRibCount,
      capLightAngle: knurlLight.angle,
      capLightOffset: knurlLight.offset,
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
      knobRotation,
      litMask,
      ledBrightness,
      ledColorOverride,
      vmapCount: 2,
      vmapActive: s.demoVmapActive,
      rgbColor: s.uniform ? s.accent : RAINBOW[i % 8],
      rgbOff: s.rgbOff,
      selected: s.sel === i,
      showLabel: false,
      onSelect: () => setState({ sel: i, selSide: -1 }),
    }),
    (side, i) => {
      const idx = (side === "L" ? 0 : 3) + i;
      return {
        selected: s.selSide === idx,
        onSelect: () => setState({ selSide: idx, sel: -1 }),
      };
    }
  );

  el.chassis.replaceChildren(chassisEl);
  el.scaleNote.textContent = `${mm(chassisSize)} × ${mm(
    chassisSize
  )} · drawn at 4× scale (1 mm = 4 px)`;

  const mid = chassisSize / 2 + s.sideBtnOffsetY;
  const centres = [mid - s.sideBtnSpacing, mid, mid + s.sideBtnSpacing];
  el.readouts.replaceChildren(
    elc("span", {
      text: `chassis ${mm(chassisSize)} · r ${mm(s.cornerRadius)}`,
    }),
    elc("span", { text: `pitch ${mm(s.pitch)}` }),
    elc("span", { text: `cell ⌀${mm(s.bodySize)} · gap ${mm(gridGap)}` }),
    elc("span", { text: `edge → 1st centre ${mm(s.edgeFirst)}` }),
    elc("span", { text: `side y ${centres.map((c) => mm(c)).join(" / ")}` })
  );
}

// --- Sidebar ---------------------------------------------------------------

function swatchButton({ color, selected, onClick, size = 28 }) {
  return elc("button", {
    style: `width:${size}px; height:${size}px; border-radius:50%; border:2px solid ${
      selected ? "var(--ds-accent-bright)" : "transparent"
    }; background:${color}; cursor:pointer; padding:0;`,
    onClick,
  });
}

function toggleField(labelText, checked, onToggle) {
  return elc("label", {
    style: "display:flex; align-items:center; justify-content:space-between;",
    children: [
      elc("span", {
        style:
          "font-family:var(--ds-font-mono); font-size:10.5px; color:var(--ds-text-dim);",
        text: labelText,
      }),
      elc("button", {
        style: `width:38px; height:21px; border-radius:11px; border:none; cursor:pointer; background:${
          checked ? "var(--ds-accent)" : "#16241f"
        }; position:relative;`,
        onClick: onToggle,
        children: [
          elc("span", {
            style: `position:absolute; top:2px; left:${
              checked ? "19px" : "2px"
            }; width:17px; height:17px; border-radius:50%; background:#06090a;`,
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
        style:
          "display:flex; align-items:baseline; justify-content:space-between; font-family:var(--ds-font-mono); font-size:11px; color:var(--ds-text);",
        children: [
          elc("span", { text: c.label }),
          elc("span", {
            style: "color:var(--ds-cyan);",
            text: String(c.display),
          }),
        ],
      }),
      (() => {
        const input = elc("input", {
          attrs: {
            type: "range",
            min: c.min,
            max: c.max,
            step: c.step,
            value: c.value,
          },
        });
        input.addEventListener("input", () => c.onInput(input.value));
        return input;
      })(),
    ],
  });
}

function renderSidebar() {
  const s = {};
  for (const k of Object.keys(state)) s[k] = state[k].value;
  const sidebar = elc("div", {
    style: "display:flex; flex-direction:column; gap:24px;",
  });

  sidebar.appendChild(
    elc("div", {
      style:
        "display:flex; align-items:baseline; justify-content:space-between;",
      children: [
        elc("span", { class: "twin-heading", text: "Geometry" }),
        elc("button", {
          class: "twin-reset",
          text: "Reset to spec",
          onClick: () => setState({ ...SPEC }),
        }),
      ],
    })
  );

  // RGB colour (demo only - the real config GUI drives this per-encoder
  // from the device's own vmap HSV, see webui/js/editor.js).
  sidebar.appendChild(
    elc("div", {
      style: "display:flex; flex-direction:column; gap:8px;",
      children: [
        elc("span", { class: "twin-label", text: "RGB COLOUR (DEMO)" }),
        elc("div", {
          style: "display:flex; gap:9px;",
          children: PALETTE.map((color) =>
            swatchButton({
              color,
              selected: s.accent === color,
              onClick: () => setState({ accent: color }),
            })
          ),
        }),
        toggleField("RGB LEDs off (unlit)", s.rgbOff, () =>
          setState({ rgbOff: !s.rgbOff })
        ),
        toggleField("Uniform colour", s.uniform, () =>
          setState({ uniform: !s.uniform })
        ),
      ],
    })
  );

  // Indicator LED demo (position/display mode/detent/RB) - drives all 16
  // encoders identically, previewing the same computeLitMask()/
  // computeLedBrightness()/computeDetentColorOverride() pipeline the
  // live twin uses per-encoder from real device state.
  sidebar.appendChild(
    elc("div", {
      style: "display:flex; flex-direction:column; gap:8px;",
      children: [
        elc("span", { class: "twin-label", text: "INDICATOR LEDS (DEMO)" }),
        sliderField({
          label: "Position",
          min: 0,
          max: ENC_MAX,
          step: 1,
          value: s.demoPosition,
          display: s.demoPosition,
          onInput: (v) => setState({ demoPosition: Number(v) }),
        }),
        elc("div", {
          style: "display:flex; gap:9px;",
          children: [
            { mode: LedDisplayMode.SINGLE, label: "Dot" },
            { mode: LedDisplayMode.MULTI, label: "Bar" },
            { mode: LedDisplayMode.MULTI_PWM, label: "Blended Bar" },
          ].map(({ mode, label }) =>
            elc("button", {
              class: "twin-reset",
              style:
                s.demoDisplayMode === mode
                  ? "border-color:var(--ds-accent-bright); color:var(--ds-accent-bright);"
                  : "",
              text: label,
              onClick: () => setState({ demoDisplayMode: mode }),
            })
          ),
        }),
        toggleField("Detent mode", s.demoDetent, () =>
          setState({ demoDetent: !s.demoDetent })
        ),
        sliderField({
          label: "Detent red LED",
          min: 0,
          max: 255,
          step: 1,
          value: s.demoRbRed,
          display: s.demoRbRed,
          onInput: (v) => setState({ demoRbRed: Number(v) }),
        }),
        sliderField({
          label: "Detent blue LED",
          min: 0,
          max: 255,
          step: 1,
          value: s.demoRbBlue,
          display: s.demoRbBlue,
          onInput: (v) => setState({ demoRbBlue: Number(v) }),
        }),
        elc("span", {
          style:
            "font-family:var(--ds-font-mono); font-size:9.5px; color:var(--ds-text-faint);",
          text: "Red/blue detent LEDs only show at dead-centre position (127), and share the centre indicator's slot.",
        }),
        toggleField("Active vmap: B", s.demoVmapActive === 1, () =>
          setState({ demoVmapActive: s.demoVmapActive === 1 ? 0 : 1 })
        ),
      ],
    })
  );

  const capHex = hsvHex(s.capH, s.capS, s.capV).toUpperCase();
  sidebar.appendChild(
    elc("div", {
      style:
        "display:flex; flex-direction:column; gap:8px; border-top:1px solid var(--ds-border); padding-top:18px;",
      children: [
        elc("div", {
          style:
            "display:flex; align-items:center; justify-content:space-between;",
          children: [
            elc("span", { class: "twin-label", text: "ENCODER CAP COLOUR" }),
            elc("span", {
              style: "display:flex; align-items:center; gap:8px;",
              children: [
                elc("span", {
                  style: `width:20px; height:20px; border-radius:5px; background:${capHex}; border:1px solid var(--ds-border-bright);`,
                }),
                elc("span", {
                  style:
                    "font-family:var(--ds-font-mono); font-size:10.5px; color:var(--ds-cyan);",
                  text: capHex,
                }),
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
            return swatchButton({
              color,
              selected,
              size: 26,
              onClick: () => setState({ capH: h, capS: sat, capV: v }),
            });
          }),
        }),
        elc("span", {
          style:
            "font-family:var(--ds-font-mono); font-size:9.5px; color:var(--ds-text-faint);",
          text: "Base, midsection and knurl tones derive from this one colour.",
        }),
      ],
    })
  );

  for (const sec of sections()) {
    sidebar.appendChild(
      elc("div", {
        style:
          "display:flex; flex-direction:column; gap:14px; border-top:1px solid var(--ds-border); padding-top:18px;",
        children: [
          elc("span", { class: "twin-label", text: sec.title }),
          ...sec.controls.map(sliderField),
        ],
      })
    );
  }

  el.sidebar.replaceChildren(sidebar);
}

// --- Entry -----------------------------------------------------------------

effect(() => {
  for (const k of Object.keys(state)) void state[k].value; // establish deps
  renderStage();
  renderSidebar();
});
