# 06 — Roadmap

Ordered so that something real is usable early, and so the riskiest unknowns get
tested before much is built on top of them.

## Sequencing logic

Two things gate everything else and should be proven first:

1. **Can the firmware carry an NSP frame at all?** Until a `HELLO` round-trips,
   every estimate downstream is a guess.
2. **Does the HID interface work on the XMega under LUFA?** If it does not, the
   whole plan falls back to SysEx and the telemetry budget shrinks — better to
   know in week one than month three.

The 3D twin, despite being the headline, is deliberately *not* first. It depends
on telemetry, which depends on the protocol. Building it against mock data first
would mean building it twice.

---

## Phase 0 — Protocol foundation

**Goal:** `HELLO` round-trips over SysEx from a browser console.

- `protocol/nsp.schema.yaml` covering the current config surface
- Generator → C header/source, TS module, golden `vectors.json`, reference docs
- `nsp/` liftable C library: framing, CRC16, session, 8-in-7 codec
- Firmware: NSP handler under manufacturer ID `0x7D`, alongside the existing
  handler. `HELLO`, `PING`, `PARAM_GET`, `PARAM_SET`
- Tests: round-trip fuzz in `tests/` (C) and Vitest (TS), both asserting against
  the shared vectors

**Done when:** a browser reads and writes a parameter, and — the acceptance test
that matters — **sets an encoder colour correctly**, which the current scheme
cannot do at all ([02](02-protocol.md#what-is-wrong-with-the-current-scheme)).

## Phase 1 — Bulk config + fast transport

**Goal:** the whole device state moves in one operation, quickly.

- `SNAPSHOT_GET` / `SNAPSHOT_SET` with fragmentation
- `STORE` / `FACTORY_RESET`
- Vendor HID interface in the LUFA descriptors + report handler
- TS `Transport` implementations: HID and MIDI-SysEx, with automatic preference
  and fallback
- The **device simulator**

**Done when:** full config read completes in under 200 ms over HID, and the
editor works end-to-end against the simulator with no hardware.

*Risk:* HID descriptor and endpoint budget on the XMega. If it does not fit,
fall back to Web Serial over the existing CDC interface
([01 § D](01-transport.md)) rather than to WebUSB.

## Phase 2 — Editor core

**Goal:** it replaces the stock utility.

- App shell: status bar, left rail, centre, inspector, monitor drawer
- 2D grid view and table view (3D comes later)
- Schema-driven inspector
- Live-edit model, dirty state, explicit `STORE`
- Undo/redo, conflict detection
- MIDI monitor, protocol monitor, CDC console pane

**Done when:** every parameter reachable in the DJTT Midi Fighter Utility is
reachable here, and the table view does bulk editing the stock utility cannot.

## Phase 3 — Profiles

- JSON profile format, versioned by `schema_hash`
- Save/load/import/export, drag-drop, library pane
- Diff view: profile vs. device, per-field resolution
- **Stock Midi Fighter Utility preset importer**

## Phase 4 — Telemetry + the 3D twin

**Goal:** the headline feature.

- Firmware: `SUBSCRIBE`, encoder/switch/LED event streams with self-rate-limiting
- Gamma LUT + HSV→RGB export to TS via codegen
  ([04 § Colour fidelity](04-device-twin-3d.md#colour-fidelity))
- Parametric geometry script → glTF build step
- r3f scene, instanced emitters, per-instance colour buffers
- Materials, bloom, HDRI, contact shadows
- Interaction: pick, drag-to-turn, hover-identify, camera presets

Ship after the blockout ([04 § Scope control](04-device-twin-3d.md#scope-control));
materials and detail land incrementally.

*Prerequisite worth starting early:* the photogrammetry capture, if you have the
hardware. It is independent of all the software work and can happen in parallel
from day one.

## Phase 5 — Firmware update

- Release manifest published with GitHub releases
- Atmel DFU over WebUSB (port [AVRFlashOnWeb](https://github.com/tmk/AVRFlashOnWeb))
- Auto-backup → flash → verify → restore flow
- Windows Zadig guidance path
- Stock DJTT firmware as a first-class restore option

## Phase 6 — Debug and depth

- Scope view: encoder velocity/acceleration over time
- Firmware feature work the editor unlocks: **LFO** (the struct exists and is
  unwired — an editor with a waveform preview is what makes it usable),
  virtual banks, standalone config
- Tauri shell for Safari/Firefox users and driverless Windows DFU

---

## Parallelisable

- Photogrammetry / geometry authoring — independent of everything, start now
- Visual design system — independent of the protocol
- The simulator — unblocks all UI work before firmware is ready

## Ordering constraints

```
Phase 0 ──► Phase 1 ──► Phase 2 ──► Phase 3
                │            │
                └──► Phase 4 (telemetry) ──► 3D twin
                             │
                             └──► Phase 5
   geometry authoring ───────┘
```

## What I would cut under pressure

In order: Phase 6 depth, the profile diff view, the stock-preset importer, the
3D materials pass (blockout still ships), Windows DFU. What I would not cut: the
protocol codegen and its shared test vectors — the drift they prevent is the
thing that would otherwise quietly consume the project.
