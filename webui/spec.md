# NEOSAM Editor — UI/UX specification

**Product:** Browser-based configuration, preset and firmware tool for the DJ TechTools Midi Fighter Twister running NEOSAM custom firmware.
**Status:** Draft v0.1 — feature scope agreed, visual design not yet started.
**Audience:** This document is written to be handed to a design pass (layout, visual language, component design) and an implementation pass (state machine, protocol handling, behaviour). Sections are tagged accordingly.

---

## 1. Scope and constraints

### 1.1 What this is

A single-page web application that connects to one or more Midi Fighter Twisters over WebMIDI, reads and writes their configuration, manages presets, and flashes firmware.

### 1.2 Hard constraints

| Constraint | Consequence for design |
|---|---|
| Single fixed hardware target | No device manifests, no dynamic layout engine. The 4×4 grid, 4 banks and side switches are hardcoded. |
| Browser-based, WebMIDI + SysEx | Chromium-family only. Firefox and Safari must be gated with a clear message. |
| Must work logged-out | Login gates *writes to the community layer only* (votes, comments, bug reports). Editing, preset browsing and firmware updates never require an account. |
| Must work offline | GitHub is an enhancement, not a dependency. Local presets, local firmware binaries and the full editor work with no network. |
| Community open-source firmware | Firmware validation prevents *mistakes*, not attacks. ID + CRC, not signatures. |

### 1.3 Out of scope for v1

- Mobile/tablet layouts (desktop-first; degrade gracefully, don't optimise)
- Multi-user collaboration on a config
- Cloud-hosted user preset storage (local + GitHub only)

---

## 2. Core concepts

These terms are used consistently throughout the UI and the codebase. Define them once, in the tutorial and the glossary.

| Term | Meaning |
|---|---|
| **Unit** | One physically connected Twister. Identified by serial number, labelled by user-assigned nickname. |
| **Baseline** | The full configuration downloaded from the unit at connect time. The revert target. |
| **Working config** | The editor's current in-memory config for a unit. What the inspector shows. |
| **Device RAM** | The unit's live config. Every edit pushes here immediately, so changes are audible/visible on hardware at once. |
| **Device EEPROM** | Persistent storage. Only written by an explicit **Store** action. |
| **Dirty** | Working config differs from EEPROM. Tracked by the editor *and* by the firmware. |
| **Element** | One encoder or one side switch, within one bank. |
| **Bank** | One of four pages of 16 encoders. |

### 2.1 The three-copy model — the most important idea in the app

```
  Editor working config  ──edit──▶  Device RAM  ──Store──▶  Device EEPROM
          ▲                                                       │
          └──────────────── Baseline (captured on connect) ◀──────┘
```

Every UI decision follows from this:

- **Editing is instant and non-destructive.** Turn a knob's colour in the inspector, the hardware ring changes colour immediately. Nothing is permanent.
- **Store is the only commit.** Preset load does not store. Undo/redo does not store. Revert does not store. Factory reset does not store (it loads defaults into RAM; the user still confirms with Store).
- **Dirty state must be impossible to miss.** See §5.3.

---

## 3. Application shell

### 3.1 Layout regions

```
┌─────────────────────────────────────────────────────────────────────┐
│  A. Header bar                                                      │
├──────────────┬──────────────────────────────────────┬───────────────┤
│              │                                      │               │
│  B. Unit     │  D. Editor canvas                    │  E. Inspector │
│     sidebar  │     (bank tabs + 4×4 grid +          │               │
│              │      side switches)                  │               │
│              │                                      │               │
│              ├──────────────────────────────────────┤               │
│              │  F. Utility drawer                   │               │
│              │     (MIDI monitor / dev log)         │               │
└──────────────┴──────────────────────────────────────┴───────────────┘
```

### 3.2 Region contents

**A. Header bar** — Unit name and connection status · dirty indicator and **Store** button · undo/redo controls · firmware update badge when an update is available · links menu (GitHub repo, user manual, releases) · account state (Sign in / avatar).

**B. Unit sidebar** — List of discovered units, one row each. Empty state when nothing is connected. `[+] Scan` action at the bottom. Selecting a unit switches the entire application context to it.

**C.** *(reserved)*

**D. Editor canvas** — Bank tabs (1–4), the 4×4 encoder grid, and the side switches rendered in their true physical positions relative to the grid. This is the primary selection surface. Basic usage guidance lives here as quiet inline hints, not a permanent panel.

**E. Inspector** — Properties of the current selection. Handles single and multi-selection. Empty state when nothing is selected: show usage guidance rather than a blank panel.

**F. Utility drawer** — Collapsed by default. Tabs: MIDI monitor, developer log. Expandable to roughly half canvas height. Never covers the inspector.

### 3.3 Design notes

- The device grid is the visual anchor of the app. It should read as a recognisable Twister, not a generic 4×4 of squares — encoder rings, ring colour, indicator position and switch state are all live and should be legible at a glance.
- The header must accommodate a persistent, prominent dirty banner without reflowing the canvas.
- Design for a single unit first; multi-unit is a sidebar concern only.

---

## 4. Connection

### 4.1 Discovery and selection

- Units are auto-detected from connected USB MIDI devices on load. No manual scan should normally be needed; `[+] Scan` exists as a fallback.
- Each unit is individually connectable and disconnectable.
- Exactly one unit is *selected* at a time. All workflow actions target the selected unit.
- Units are distinguished by **serial number**. The user can assign a **nickname**, persisted locally against that serial.

### 4.2 Unit row states

| State | Sidebar treatment |
|---|---|
| Connected, compatible firmware | Normal. Nickname, firmware version. |
| Connected, dirty | Dirty marker on the row (so it's visible while another unit is selected). |
| Connected, running stock DJTT firmware | Distinct icon. Selecting it opens the migration dialog (§8.3). |
| Connected, firmware incompatible with this editor | Warning icon, editing disabled, firmware update offered. |
| In bootloader mode / no identity | "Unrecognised device (bootloader)". Routes only to firmware recovery. **Must remain visible** — a half-flashed unit must not disappear from the tool that recovers it. |
| Port unavailable | See §4.4. |

### 4.3 On connect

1. Query firmware version and serial.
2. Check firmware compatibility against the editor's supported range.
3. Download the full system config over SysEx.
4. Store it as the **Baseline**.
5. Set the editor-connected flag in firmware (see §11).

### 4.4 Port contention

On Linux in particular, another application (Mixxx, a DAW) holding the MIDI port will block the connection. Detect this case and say so explicitly — *"Another application is using this device. Close it and scan again."* Never surface it as "device not found."

### 4.5 Disconnect

- Disconnecting with unsaved changes must warn, and state clearly that unstored changes will be lost.
- On unexpected disconnect (cable pulled), the working config is retained locally and offered for re-application if the same serial reconnects.

---

## 5. Configuration editing

### 5.1 Selection

- Click an element to select it.
- **Rubber-band select**: drag on empty canvas to lasso. Shift adds, Alt subtracts, Ctrl/Cmd-A selects the whole bank.
- **Follow mode** (global toggle): selection follows physical interaction with the hardware. Turning an encoder selects that encoder; pressing a switch selects that switch. Off by default — it fights with live use.
- Keyboard: arrow keys move selection within the grid; Tab moves between grid and inspector.

### 5.2 Inspector behaviour

Single selection shows the element's properties directly.

Multi-selection uses **mixed-value state**:

- Fields where all selected elements agree show the shared value.
- Fields where they differ show an explicit "Mixed" indicator — never the first value, never blank.
- Mixed fields remain **editable**; entering a value writes it to every selected element.
- Editing one field must not flatten the others. Setting CC does not collapse six different colours to one.
- Checkboxes are tri-state: checked, unchecked, indeterminate.
- Dropdowns show "Mixed" as a transient pseudo-option that disappears on selection. It must never be selectable or persistable.

### 5.3 Dirty state and Store

- A persistent, prominent indicator in the header whenever the working config differs from EEPROM.
- The indicator names the scope: which unit, how many changes.
- **Store** writes RAM to EEPROM. It is the only action that does so.
- Dirty state persists across bank switching and unit switching. The editor tracks dirty state per unit; switching away from a dirty unit does not store, discard, or warn — but the sidebar row keeps its dirty marker.

### 5.4 Edit log, undo and redo

- Every configuration edit is logged as a discrete entry.
- Continuous edits **coalesce**: dragging a colour slider is one entry, not two hundred.
- Applying a preset or a full config is **one** entry.
- The user can revert to any position in the log. Undone entries are greyed out rather than deleted, and can be selectively redone.
- Making a new edit while entries are undone clears the undone entries and appends the new one.
- Undo, redo and revert only push to **RAM**. They never write EEPROM.

**Design note:** the edit log should be browsable, not just a pair of buttons. A list with human-readable entry labels ("Encoder 7 → CC 22", "Applied preset: Mixxx 4-deck") is what makes selective redo usable.

### 5.5 Conflict detection

- Detect duplicate channel + CC assignments across all elements, including across banks.
- Flag conflicting elements inline on the grid and in the inspector.
- Each flag carries a **"View conflict"** action that navigates to the other conflicting element — switching bank if necessary.

### 5.6 Copy and paste

- Element-to-element copy/paste.
- Whole-bank copy/paste.
- **Paste special**: on paste, show a filter selector letting the user choose what to include — MIDI assignment, colours, encoder behaviour, switch behaviour. Colour schemes and MIDI maps are designed independently and must be transferable independently.

### 5.7 Bulk assign

Beyond set-all, one bulk operation needs its own small dialog:

**Assign ascending** — takes a start value, a step, and a direction (row-major, column-major, reverse). This is what people actually want when laying a bank out against a DAW control surface. It does not belong inside an inspector field.

### 5.8 Reset actions

Four scopes, in increasing severity:

| Action | Scope | Confirmation |
|---|---|---|
| Reset encoder | One encoder to firmware default | None (undoable) |
| Reset switch | One switch to firmware default | None (undoable) |
| Reset bank | 16 encoders in current bank | Inline confirm |
| Factory reset | Entire device config | Modal warning dialog |

All four write to RAM only. The user still has to Store.

---

## 6. Presets

### 6.1 Sources

- **Factory presets** — shipped with the editor and mirrored on GitHub.
- **Community presets** — loaded from GitHub.
- **Local presets** — saved and loaded from the user's machine. Available offline.

**Implementation note:** do not query the GitHub API to browse presets. Publish a static index JSON from CI and fetch that; reserve API calls for votes and comments, which are authenticated and therefore not subject to the low unauthenticated rate limit.

### 6.2 Applying a preset

- Applying a preset shows the **same filter selector** used on export: which elements, which banks, what property groups get overwritten.
- Applying a preset writes to **RAM only**. The user must still Store. This must be stated in the flow, not assumed.
- A preset application is a single undo entry.

### 6.3 Export

- Export with a filter: selectively export specific elements, individual banks, or property groups.

### 6.4 Versioning

- Every preset carries a **schema version**.
- Schema versions map to supported firmware version ranges.
- Loading a preset outside the supported range must be detected and surfaced before anything is applied — with the option to proceed, migrate, or cancel as appropriate.

### 6.5 Community layer

- Voting and commenting on factory and community presets.
- Backed by GitHub OAuth and GitHub Discussions — identity, moderation and spam handling come with it.
- **Read is always public.** Only submitting requires sign-in.

---

## 7. Firmware update and recovery

### 7.1 Detection and notification

- Detect the currently flashed firmware version on connect.
- Fetch the latest release binary, version and changelog from GitHub releases.
- When an update is available, surface a non-blocking badge in the header. Never auto-update, never interrupt.

### 7.2 Update flow

A modal, stepped workflow (§9.1):

1. **Review** — current version, target version, changelog summary.
2. **Config handling** — detect configuration schema conflicts between the two versions and notify the user before proceeding. Offer to export the current config to file first.
3. **Flash** — real progress, not a fake bar.
4. **Verify and reconnect** — confirm the new version, restore config if applicable.

### 7.3 Local binary

- The user can load a local firmware binary instead of a GitHub release. This is also the offline path and the fork path, so it must be discoverable, not hidden.

### 7.4 Validation

Before flashing, validate:

- Product / hardware ID match
- CRC / integrity
- Version parse

This catches the real failure cases — wrong `.hex`, corrupt download — which is the goal. Be explicit in the copy that this prevents mistakes rather than tampering.

### 7.5 Downgrade

Downgrading is a supported, explicit path — not an accident. Warn about config schema implications.

### 7.6 Interruption and recovery

- **Disable tab and window closing during flash.** `beforeunload` guard, active for the duration of the write only.
- **USB disconnect during flash** → clear error state with a recovery path, not a dead modal.
- A unit left in bootloader mode must appear in the sidebar (§4.2) and route directly to a recovery flash.

---

## 8. Stock DJTT firmware handling

### 8.1 Detection

Units running the original DJ TechTools firmware are detected and shown in the sidebar with a **distinct icon**.

### 8.2 Selection behaviour

Selecting such a unit replaces the main canvas with a dialog rather than the editor. There is nothing to edit until the unit is migrated.

### 8.3 Migration dialog

Contents:

- What NEOSAM is and what it changes.
- **Warranty confirmation** — the user must confirm they understand this likely voids the OEM warranty.
- **No config backup** — the user must confirm they understand existing DJTT configurations cannot be backed up and will be lost.
- **"How to revert to DJTT firmware"** section — reassuring and concrete: the original DJTT software can reflash the stock firmware. This is the single most important anxiety-reducer in the whole flow and should not be buried behind a disclosure triangle.
- **Start** button, gated on both confirmations.

---

## 9. Dialogs, notifications and errors

### 9.1 Dialog rules

- Workflow dialogs (firmware update, migration, factory reset, tutorial) render above everything and dim the rest of the UI.
- Only one workflow dialog at a time. There is no dialog-on-dialog.

### 9.2 Modal + device events

A device disconnect while a modal is open must **surface inside the modal**, not as a toast behind the dim layer where it cannot be seen or actioned. Every modal flow needs a defined behaviour for: unit disconnected, unit changed, port lost.

### 9.3 Error copy

Errors say what happened and what to do, in one sentence. No raw exception strings, no apology, no first person. "Another application is using this device. Close it and scan again." — not "Error: failed to open MIDI port."

---

## 10. Supporting features

### 10.1 MIDI monitor

- All MIDI traffic including SysEx, with timestamps.
- Filterable by direction and message type.
- User-facing, not developer-only — it is the primary tool for debugging a mapping against a DAW.

### 10.2 Developer log

- Major info, warnings and errors.
- **Does not** log individual configuration edits — that noise makes it useless for debugging. The edit log (§5.4) covers that separately.

### 10.3 Bug reporting

Auto-attaches: developer log, firmware version, editor version, browser and OS, and optionally the current configuration.

**The user sees exactly what is being sent before it goes.** Show the payload, allow the config to be excluded, then submit. Requires sign-in.

### 10.4 Tutorial

- Dialog-based walkthrough of each region, then the edit flow, then the push/Store flow.
- "Don't show again" persists locally.
- Available on demand afterwards from the links menu.

### 10.5 Links

GitHub repository, user manual, releases and changelog.

### 10.6 Browser gate

Firefox and Safari users get a clear, non-blaming message naming what does work, with a link — not a broken app.

---

## 11. State machine (implementation)

### 11.1 Unit lifecycle

```
        ┌──────────────┐
        │ Disconnected │
        └──────┬───────┘
               │ USB detect
        ┌──────▼────────┐
        │  Identifying  │
        └──────┬────────┘
               │
     ┌─────────┼──────────┬─────────────────┬──────────────┐
     ▼         ▼          ▼                 ▼              ▼
 ┌────────┐ ┌──────┐ ┌──────────┐   ┌─────────────┐  ┌──────────┐
 │  DJTT  │ │Incom-│ │Bootloader│   │  Connected  │  │   Port   │
 │firmware│ │patible│ │  mode   │   │    clean    │  │unavailable│
 └───┬────┘ └──┬───┘ └────┬─────┘   └──────┬──────┘  └──────────┘
     │         │          │                │ edit
     │         │          │         ┌──────▼──────┐
     │         │          │         │  Connected  │
     │         │          │         │    dirty    │
     │         │          │         └──────┬──────┘
     │         │          │                │ Store
     └─────────┴──────────┴────────────────┴──▶ (flash / reconnect)
```

### 11.2 Connect sequence

1. Enumerate USB MIDI devices
2. Identify request → serial, firmware version, firmware family (NEOSAM / DJTT / unknown)
3. Compatibility check against editor's supported firmware range
4. Full config download over SysEx
5. Capture as Baseline
6. Set editor-connected flag in firmware
7. Enter `Connected clean`

### 11.3 Edit sequence

1. Inspector change → update working config
2. Push affected element(s) to device RAM over SysEx
3. Append coalesced entry to edit log
4. Set dirty flag locally
5. Firmware sets its own dirty flag

### 11.4 Store sequence

1. Write RAM to EEPROM (device-side commit)
2. Clear dirty flag, editor and firmware
3. Baseline is **not** updated — Baseline remains the connect-time snapshot for the session so revert-to-original stays available

### 11.5 Required firmware changes

Two changes to NEOSAM firmware to support this editor:

1. **Editor-connected flag** — a new field set while the editor is connected, so the firmware knows it is under external control.
2. **Disable periodic EEPROM backup entirely** — otherwise the firmware will silently persist RAM edits and break the entire uncommitted-changes model.

### 11.6 Persistence (local)

| Data | Where | Lifetime |
|---|---|---|
| Unit nicknames (by serial) | localStorage | Permanent |
| Tutorial dismissal | localStorage | Permanent |
| Local presets | localStorage / file system | Permanent |
| Working config + edit log | localStorage | Session recovery — survives tab crash |
| Baseline | Memory | Connection lifetime |

---

## 12. Empty and first-run states

| State | Treatment |
|---|---|
| No device connected | Empty canvas, no inspector sidebar content, empty unit list with `[+] Scan/Connect`. An invitation, not an error. Auto-detection should mean users rarely see this. |
| Device connected, nothing selected | Grid visible and live. Inspector shows usage guidance. |
| First run | Tutorial dialog. |
| Unsupported browser | Browser gate (§10.6). |
| Offline | Community presets section shows an offline state; everything else works normally. |

---

## 13. Open decisions

Flagged for resolution before build:

1. **Bootloader transport** — USB DFU (WebUSB, separate permission model) or SysEx (same channel as config, simpler, slower). This determines the entire firmware flow's permission and error handling.
2. **Colour model in the inspector** — the Twister's rings take a hue index, not RGB. A conventional colour picker will lie about the result. Decide between rendering the device's actual palette as swatches, or round-tripping through the device for a true preview. Live preview on hardware while hovering a swatch is the strongest option.
3. **Undo scope across units** — is the edit log per-unit or global? Per-unit is almost certainly correct but the header controls need to reflect it.
4. **Baseline after Store** — §11.4 keeps the connect-time Baseline. Confirm this is the desired behaviour versus re-baselining on each Store.
5. **Bank switching while dirty** — currently permitted. Confirm no firmware constraint requires blocking it.
6. **Licensing** — the stock DJTT firmware source is under a restrictive personal-use, no-redistribution licence. If NEOSAM is a clean-room rewrite this is a non-issue; if it derives from that tree, distributing release binaries needs deliberate handling.

---

## 14. Design brief notes

For the visual design pass:

- **Subject matter to draw from:** the device itself — anodised aluminium, backlit encoder rings, RGB, the four-bank paging metaphor, DJ booth lighting. The Twister's own visual language is strong and specific; the app should feel like an extension of the hardware rather than a generic settings panel.
- **Audience:** DJs and live performers, technically confident but not necessarily developers, often working in low light, often in a hurry before or during a set.
- **The one job of the main screen:** let someone see the whole state of their device at a glance and change part of it without fear.
- **Signature element candidate:** the live device grid. It is the thing a user will look at for 95% of their time in the app, and the thing that most distinguishes this from a form-based config tool. Spend the boldness there and keep the inspector and chrome quiet.
- **Legibility over decoration.** Colour is *data* in this app — it encodes the user's own configuration. The chrome must not compete with it. This is a real constraint on the palette: the interface should be substantially desaturated so the user's encoder colours read accurately.
- **Dark mode is not optional** given the use context.