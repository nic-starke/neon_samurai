# Digital-twin design system

A small, dependency-free component library for rendering the Midi Fighter
Twister chassis in the browser - used by both `webui/twin.html` (the
standalone geometry-tuning tool) and the live device view in
`webui/index.html`/`js/live-twin.js`. Same "no framework, no build step,
no `npm`/`node_modules`" constraint as the rest of `webui/` - see the
top-level [../README.md](../README.md).

Originally ported from a Claude Design Canvas prototype ("Twister Digital
Twin.dc.html", importing "Encoder.dc.html" and "ChromaCap.dc.html").

## Why not a framework

This app has no backend - `webui/` is static files served locally only to
satisfy Web MIDI's sysex permission requirement (see the top-level
README's "Running" section), and all state comes from either the browser
tab's own JS or live sysex/MIDI messages. Server-driven-HTML tools
(htmx and similar) have nothing to talk to here, and a full component
framework (React/Vue/etc.) would be a large dependency for a page that's
fundamentally "render some SVG/divs from a state object" - the codebase's
own stated preference (see `webui/README.md`) is to keep this dependency-
free wherever reasonable.

What's here instead:

- **Plain JS component modules** - one function per visual part, each
  taking a props object and returning a DOM/SVG node. No JSX, no virtual
  DOM, no lifecycle - a component is just a function.
- **`@preact/signals-core`** (vendored, see `../js/vendor/`) for reactive
  state - a `signal()` write triggers exactly the `effect()`s that read
  it, so a live device stream (sysex/MIDI messages arriving many times a
  second) doesn't require hand-written dirty-checking or a full
  "rebuild everything" pass on every message. ~1.2kB, MIT-licensed,
  vendored locally rather than CDN-imported so the app keeps working
  fully offline.
- **`tokens.css`** - the design tokens (colours, spacing, glow effects)
  every component's inline styles reference via `var(--ds-*)`, so the
  palette lives in one place.

## Aesthetic

Monochrome, bluey-green, dark - a "Matrix" phosphor-glow palette (see
`tokens.css`'s `--ds-accent`/`--ds-cyan`/`--ds-glow` tokens). This
governs the *chrome* - panel backgrounds, borders, UI text, unlit-LED
colour, selection outlines. It deliberately does **not** govern the
*device's own reported colours* - an encoder's RGB LED and indicator ring
render whatever colour the live HSV data (from sysex, or the tuning
sidebar's demo state) says they are. A device showing red should still
show red; only the surrounding interface commits to green/cyan.

There are three LED brightness tiers, not two - see `tokens.css`:

- **Lit** (`--ds-led-on` / a real `rgbColor`) - an LED actually on.
- **Off** (`--ds-led-off`) - an LED that's unlit while the device is
  live/connected (e.g. a colour layer with no colour configured, or an
  indicator outside the current bar-graph fill).
- **Powered off** (`--ds-led-powered-off`, dimmer still) - the *whole
  device* has no power/connection at all (see `live-twin.js`'s
  disconnected render). Deliberately distinct from "off" so a
  disconnected twin doesn't read as "every LED happens to be unlit" -
  see `rgb-arc.js`/`led-ring.js`/`encoder.js`'s `powered` prop.

## Components

| Module | Renders | Notable props |
|---|---|---|
| `chassis.js` | Plastic faceplate + rubber bevel. Static - no colour/on-off state, only geometry. | `size`, `cornerRadius`, `bevelWidth` |
| `device-chassis.js` | The full assembled device: `chassis.js` + the 4x4 encoder grid + 6 side switches + the two-light knurl-shading system, composed together. Shared by `twin.js` and `live-twin.js` so the grid/lighting geometry lives in one place. | `encoderProps(i, knurlLight)` and `sideSwitchProps(side, i)` callbacks - see its doc comment |
| `encoder.js` | One full encoder assembly: body, RGB arc, LED ring, and cap, composed together. | `rgbColor`/`rgbOff`, `litMask` (or `value`/`max`), `ledBrightness`, `ledColorOverride`, `knobRotation`, `capColor`, `selected`, `powered` |
| `cap.js` | The knurled cap top-view only (used internally by `encoder.js`, but also usable standalone for a cap-only preview). | `color`, `ribCount`, `lightAngle`/`lightOffset`, `reflectionColor` (subtle coloured bounce from the nearby lit RGB LED) |
| `led-ring.js` | The ring of discrete indicator LEDs (11 on real hardware). | `count`, `arcSpan`, `litMask`, `brightness` (per-LED BCM 0-31, for `DIS_MODE_MULTI_PWM`'s smooth fade - takes priority over `litMask`), `colorOverride` (recolors one LED - used for the detent red/blue LEDs, which share the center indicator's physical slot), `powered` |
| `rgb-arc.js` | The RGB backlight indicator arc. | `color`, `off` (suppresses glow entirely, not just colour - see inline comment), `powered` |
| `side-switch.js` | One side switch button. Pressed/unpressed only. | `pressed`, `selected` |
| `color-utils.js` | Shared cosmetic HSV/hex math (`shift`, `dim`, `hsvHex`) for material shading - NOT the firmware colour model, see `../js/color.js` for that and the note in this file. | - |
| `led-mask.js` | Firmware-accurate lit-LED/brightness/detent-colour math, not a renderer - ports `src/led/led.c`'s LUTs and BCM brightness calculation so a real position/display-mode/detent/rb tuple can drive `led-ring.js` exactly like the real device. | `computeLitMask()`, `computeLedBrightness()` (per-LED BCM for `DIS_MODE_MULTI_PWM`), `computeDetentColorOverride()` (red/blue detent LED colour, only shown at dead-centre) |
| `dom.js` | `elc()`/`svgEl()` - the only DOM-construction helpers every component uses. | - |

### Why "Encoder" is one component, not four

The knob, plastic ring, indicator LEDs, and RGB LED all update together
from the same sysex reads (`ENCODER_*`/`VMAP_*` params landing on one
`struct encoder`) and visually belong to one physical part - splitting
them into separate top-level components would mean threading the same
position/colour/detent state through four call sites for every render.
`encoder.js` composes the finer-grained `cap.js`/`led-ring.js`/
`rgb-arc.js` internally so those pieces stay independently testable/
reusable, but the thing callers (`twin.js`, `live-twin.js`) instantiate
per physical encoder is the one composed unit.

### Firmware-accurate rendering vs. demo/geometry preview

Components here are **prop-driven and state-agnostic** - they don't know
whether their `litMask`/`rgbColor`/`value` came from a live device, a
JSON preset, or a slider in the tuning sidebar. Two current callers:

- `webui/twin.html`/`js/twin.js` - standalone geometry/colour tuning tool,
  demo data only (no device connection).
- `webui/index.html`/`js/live-twin.js` - the live device view, which
  renders these components from `DeviceModel`'s real per-encoder HSV/
  detent/display-mode state (pulled from a connected Twister over sysex
  immediately on connect) plus `live-position.js`'s live knob rotation
  (pushed unsolicited by the firmware itself over sysex - see the
  top-level README's "How live tracking works").

If you add a third caller, keep state derivation (device model → props)
in that caller, not in these component modules.

## Adding a new component

1. New file in `components/`, exporting one `build*()` function that
   takes a props object and returns a DOM node (or `DocumentFragment` for
   something that's just a list of siblings, like `led-ring.js`).
2. Reference colours/spacing via `var(--ds-*)` tokens from `tokens.css`
   wherever the value is chrome, not live device data.
3. If it composes other components, import and call them directly (see
   `encoder.js`) rather than duplicating their markup.
4. Document it in the table above.
