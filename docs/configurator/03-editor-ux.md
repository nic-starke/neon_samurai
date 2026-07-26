# 03 — Editor UX

What the thing looks like and how it behaves.

## What the good editors get right

I looked at the tools people actually enjoy using, and the pattern is consistent.

**VIA / Vial** (keyboards). No upload button. You click a key, pick a function,
and the keyboard changes *now*. Writing to flash is a separate deliberate action.
This is the single biggest UX decision in the category and almost every older
editor gets it wrong.

**Intech Studio Grid Editor**. Layered depth: presets → visual action blocks →
raw Lua, in the same window. A beginner is never shown the scripting pane, and an
expert is never trapped behind a wizard. It also treats the MIDI monitor and the
debug monitor as permanent furniture, not a hidden diagnostic.

**Novation Components**. Browser-first, no install, a device picture you interact
with directly rather than a table of parameters, and a shared preset library.

**Elektron Transfer / Overbridge**. Honest about device state — you always know
what is on the device versus what is in the app, and transfers are visible
operations with progress, not silent magic.

**Ableton/Bitwig device panels**. Dense without being cramped; every control
shows its current value inline; alt-drag duplicates; everything has a keyboard
path.

**What the stock Midi Fighter Utility gets wrong** (and we should not repeat): a
Windows/macOS-only binary, a grid of dropdowns with no visual feedback, a modal
"send to device" step, and no way to see what the device is actually emitting.

## Principles

1. **Live by default.** Every edit is a `PARAM_SET` to RAM, immediately. The
   device *is* the preview. `STORE` writes EEPROM and is an explicit button with
   a dirty indicator next to it.
2. **The device is the source of truth.** On connect, `SNAPSHOT_GET` and render
   what is actually there. Never assume; never silently overwrite.
3. **One selection, one inspector.** Selecting encoder 7 in the 3D view, the 2D
   grid, or the table is the same selection. The right pane always shows
   everything about the current selection and nothing else.
4. **Progressive disclosure.** Basic / Advanced / Expert. Basic is MIDI mode,
   channel, CC, colour. Expert exposes virtmap ranges and positions, dead time,
   acceleration, display modes.
5. **Monitors are furniture.** A bottom drawer with MIDI, protocol, console, and
   scope tabs. Always one keystroke away. This is a firmware project; debugging
   *is* the product.
6. **Nothing is unreachable by keyboard.** Command palette (`Cmd/Ctrl-K`), arrow
   keys to move between encoders, `Cmd-C/V` to copy an encoder's whole config,
   multi-select with shift/ctrl to edit sixteen at once.
7. **Reversible.** Full undo/redo over the config, and a live diff against both
   the device's stored EEPROM state and the loaded profile.

## Layout

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ ● NEON_SAMURAI  fw 0.4.1  HID  •  Bank 2/3  •  ◆ 3 unsaved     [Store] [⋯]  │  status bar
├────────────┬────────────────────────────────────────────────┬────────────────┤
│            │                                                │                │
│  DEVICE    │                                                │   INSPECTOR    │
│  ▸ Bank 1  │                                                │                │
│  ▾ Bank 2  │            ┌──────────────────────┐            │  Encoder 07    │
│    enc 00  │            │                      │            │  ───────────   │
│    enc 01  │            │     3D  /  2D  /  ▤  │            │  Detent    ▢   │
│    ...     │            │                      │            │  Display  Multi│
│  ▸ Bank 3  │            │   the device twin    │            │                │
│  ▸ Side    │            │                      │            │  ▾ Virtmap A   │
│            │            │                      │            │   Mode    CC14 │
│  PROFILES  │            └──────────────────────┘            │   Ch      1    │
│  ▸ Library │                                                │   CC      21   │
│  ▸ Recent  │                                                │   Range 0–127  │
│            │                                                │   Colour  ████ │
│            │                                                │                │
├────────────┴────────────────────────────────────────────────┴────────────────┤
│ MIDI │ Protocol │ Console │ Scope                                       ▲ ▼  │  monitors
│ 12:04:41.221  CC  ch1  #21  →  94                                            │
└──────────────────────────────────────────────────────────────────────────────┘
```

**Left rail** — device tree (banks → encoders → virtmaps, side switches, global)
and the profile library. Doubles as a search target: type to filter, "ch 3" finds
every control on channel 3.

**Centre** — three interchangeable views over the same selection:

- **3D** ([04](04-device-twin-3d.md)) — the hero view. Live LEDs, live knob
  rotation, click to select, drag a knob to drive it.
- **2D grid** — a flat 4×4 of encoder tiles. Each tile shows colour, MIDI
  assignment, value arc, and a live-activity flash. Faster than 3D for bulk
  editing, and the accessible fallback.
- **Table** — every parameter of every encoder in the bank as a spreadsheet.
  Multi-cell select, paste a column, sort by CC to spot collisions. This is the
  view power users will live in and almost no editor in this category ships.

Split-view lets you keep 3D and table side by side.

**Right** — the inspector, driven entirely off the generated schema so a new
firmware field appears as a control automatically, with its declared range and
unit already enforced.

**Bottom** — the monitor drawer:

- **MIDI** — timestamped, filterable, colour-coded by channel; click a row to
  select the encoder that produced it. "Learn" mode: wiggle a control in your DAW,
  the editor captures the CC and offers to assign it.
- **Protocol** — raw NSP PDUs, decoded and as hex. Non-negotiable for firmware work.
- **Console** — the CDC console as a real terminal ([01 § D](01-transport.md)).
- **Scope** — encoder velocity and acceleration plotted over time. This is how
  you tune `enc_dead_time`, the acceleration curve, and the throttle, and it turns
  invisible firmware behaviour into something you can see.

## Interaction details worth specifying

- **Identify.** Hover an encoder in the UI → that encoder pulses on the physical
  device (via `LED_OVERRIDE`). Instant spatial grounding, costs almost nothing.
- **Learn.** Turn a physical encoder → the editor selects it. Bidirectional
  identify.
- **Colour picking** in HSV, because that is what the hardware stores. Show the
  gamma-corrected result, not the raw value — see
  [04 § Colour fidelity](04-device-twin-3d.md#colour-fidelity).
- **Conflict detection.** Two controls on the same channel + CC gets a warning
  badge in the tree, on the tile, and in the table. Cheap, and it is the mistake
  everyone makes.
- **Bulk ops** as first-class: select all 16, "assign CC ascending from 16",
  "spread hue across selection", "copy bank 1 to bank 3".
- **Drag a knob in the 3D/2D view** to send a value — lets you test a mapping in
  your DAW without touching the hardware.

## Profiles

Plain JSON, versioned by `schema_hash`, human-readable and diffable:

```json
{
  "nsp_schema": "0x8f21ac03",
  "name": "Traktor Layer",
  "device": { "product": "NEON_SAMURAI", "fw_min": "0.4.0" },
  "banks": [ { "encoders": [ { "detent": true, "vmaps": [ … ] } ] } ]
}
```

- Import/export as files; drag-drop onto the window.
- A library pane with tags and search.
- **Import stock Midi Fighter Utility presets** — a converter for DJTT's format
  is a strong adoption lever for anyone switching to this firmware.
- Diff view: profile vs. device, with per-field "take mine / take theirs".
- Optional git-friendliness: a profile is one file, stable key order, so a
  directory of them lives happily in a repo.

## Visual language

Take the cue from the project's name and logo without becoming a novelty.

- **Dark first**, near-black neutral background, a single saturated neon accent
  for interactive state. Light theme available and properly done, not an
  afterthought — a lot of people work in bright rooms.
- **Colour is data.** Encoder colours in the UI are the *device's actual LED
  colours*. That means the neutral chrome must stay strictly desaturated, or the
  UI competes with the thing it is displaying. This is the main visual
  constraint and it is a real one.
- Type: one geometric sans for UI, a mono for hex/monitors. Tabular numerals
  everywhere a value updates live, so nothing jitters.
- Motion: fast and functional. 120–160 ms transitions, and none at all on
  live-updating values. Respect `prefers-reduced-motion`, including reducing the
  3D view's idle camera drift to nothing.
- Density: closer to a DAW than to a marketing site. Compact rows, small
  controls, no giant hero spacing.

## Accessibility

The 3D view is a *presentation* of state, never the only route to it. Everything
reachable in 3D is reachable in the 2D grid and the table, both of which are
plain DOM, focusable, and screen-reader labelled. Colour is never the sole
carrier of meaning — conflict badges have icons, MIDI monitor rows have text
channel labels. Target WCAG AA contrast for chrome (the LED colours themselves
are data and exempt, but their *labels* are not).

## Stack

| | | Why |
| --- | --- | --- |
| Build | Vite + TypeScript | fast, static output, deploys to GitHub Pages |
| UI | React + Tailwind + Radix primitives | Radix gives correct a11y semantics free |
| State | Zustand | one store, transport/telemetry/config slices; cheap subscriptions matter at 60 Hz |
| 3D | react-three-fiber + drei + postprocessing | [04](04-device-twin-3d.md) |
| Validation | Zod, generated from the NSP schema | one source of truth for ranges |
| Test | Vitest + Playwright | plus the shared `vectors.json` from [02](02-protocol.md) |
| Package | static site; Tauri shell later | [01 § E](01-transport.md) |

**Build the device simulator first.** A TypeScript implementation of the firmware
config model behind the same `Transport` interface. It makes the whole editor
developable and testable without hardware, gives Playwright something
deterministic to drive, and doubles as the public "try it in your browser" demo
for people who do not own a Twister. It is a day or two of work that pays for
itself immediately.

## Sources

- [Intech Studio Editor](https://intech.studio/products/editor) ·
  [docs](https://docs.intech.studio/guides/grid/grid-basic/editor-110/)
- [Midi Fighter Utility](https://store.djtechtools.com/pages/midi-fighter-utility) ·
  [app guide](https://techtools.zendesk.com/hc/en-us/articles/7002462346893-Midi-Fighter-Twister-App-Guide)
- [Midi Fighter configuration guide](https://djtechtools.com/midi-fighter-setup/)
