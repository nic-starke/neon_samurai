# neosam-configurator — plan

A browser-based editor for NEON_SAMURAI: configure, save profiles, monitor,
debug, and flash firmware, with a real-time 3D twin of the Midi Fighter Twister
at the centre of the UI.

This directory is the design record. Nothing here is built yet.

| Doc | Covers |
| --- | --- |
| [01-transport.md](01-transport.md) | How the browser talks to the device. Options, trade-offs, recommendation. |
| [02-protocol.md](02-protocol.md) | NSP — the reusable framed protocol that replaces the current SysEx scheme. |
| [03-editor-ux.md](03-editor-ux.md) | Information architecture, interaction model, visual language. |
| [04-device-twin-3d.md](04-device-twin-3d.md) | Building and driving the 3D model. |
| [05-firmware-update.md](05-firmware-update.md) | In-browser DFU, and why it is the hardest part. |
| [06-roadmap.md](06-roadmap.md) | Phasing, milestones, what to build first. |

---

## The short version

**Transport.** Ship two, behind one interface. A **vendor HID interface** is the
primary path — driverless on every OS, 8-bit clean, 64-byte reports, no
contention with the DAW for MIDI ports. **Web MIDI SysEx** is the fallback so the
editor still works on firmware that predates the HID interface, and on Firefox.
Details and the four rejected options in [01](01-transport.md).

**Protocol.** The current SysEx scheme cannot carry the config. It sends raw
native structs — `u16` colour channels, `i8` ranges, AVR enum padding — inside a
frame where every byte must be ≤ 0x7F, and the firmware's own comment
acknowledges this. Colour is the parameter the editor most needs to write, and
it is the one that is definitively broken today (see
[02 § What is wrong with the current scheme](02-protocol.md#what-is-wrong-with-the-current-scheme)).
Replace it with **NSP**: a versioned, CRC'd, transport-agnostic PDU generated
from one schema into both C and TypeScript, so firmware and editor cannot drift.
The C side is a drop-in directory with no dependencies beyond `stdint.h` —
that is the "protocol I can put into my project" piece.

**Editor.** Live-edit model, borrowed from VIA/Vial: every change applies to the
device immediately; writing to EEPROM is a separate, explicit commit. One
canonical selection drives one inspector. Monitors (MIDI, protocol, console,
scope) are first-class, not an afterthought. Profiles are plain JSON files you
can diff and commit.

**3D twin.** Do **not** ship an AI-generated mesh. The device needs 16
independently rotating knobs and 208 individually addressable emissive elements;
that requires named, rigged parts, which a single-image generator does not give
you. Author the geometry **parametrically** — it is a 150×150×25 mm box with a
4×4 grid of revolved knobs — and use AI image-to-3D and photogrammetry as
*reference* for proportions and surface detail. Then drive it from the same state
store the 2D panels use, so the twin is a pure render of device state rather than
a separate application. [04](04-device-twin-3d.md) has the part taxonomy and the
render pipeline.

**Firmware update.** Entering the bootloader is easy; talking to it from a
browser is not. The DJTT bootloader is Atmel's DFU variant, which has no WCID
descriptors, so Chrome on **Windows** cannot claim it without a manual Zadig
driver swap. Linux and macOS are fine. Plan for a graceful three-tier answer
rather than pretending the problem away — [05](05-firmware-update.md).

---

## What this is built on

Read from the firmware in this repo, not assumed:

| | |
| --- | --- |
| Encoders | 16, in 3 banks (`NUM_ENC_BANKS`), 2 virtmaps each |
| Side switches | 6 |
| LEDs | 256 total = 16 encoders × 16, of which 11 are ring indicators, 3 RGB, 2 detent |
| Brightness | 32 BCM frames, per-channel gamma LUTs in `src/led/color.c` |
| Colour model | HSV is authoritative in EEPROM (hue 0–1535, sat/val 0–255); RGB is derived |
| Per-encoder config | detent, display mode ×3, virtmap display mode ×2, virtmap mode ×2, active vmap, switch mode ×8, switch protocol cfg |
| Per-virtmap config | range (lower/upper), position (start/stop), protocol cfg, HSV, RGB, RB |
| Protocol cfg | MIDI: mode (disabled/CC/CC14/relative CC/note), channel, CC or note number. OSC enumerated, unimplemented |
| Global config | encoder dead time, MIDI throttle time |
| Persistence | `struct eeprom` v11, 384 B of 2 KB used |
| USB | VID `0x2580` PID `0x0007`; MIDI class, plus CDC ACM console when `ENABLE_CONSOLE` |
| Not yet wired | LFO (`struct lfo` exists, unused), virtual banks, HID, OSC, standalone config |

Total addressable config surface: 3 banks × 16 encoders × (8 encoder fields + 2
vmaps × 6 fields) ≈ **960 parameters**, plus side switches and globals. That
number is the reason the current one-parameter-per-round-trip design has to go —
see [02](02-protocol.md).

---

## Decisions I need from you

These change the shape of the work. Everything else I can call myself.

1. **Firmware scope.** Am I allowed to add a USB interface (HID or vendor) to the
   firmware, or must the editor work against the MIDI/SysEx surface only? This is
   the single biggest fork in the plan. My recommendation is yes, add HID — it
   removes an entire class of problems — but it is your device and your flash
   budget.
2. **Windows DFU.** Are you willing to ship a small native helper for firmware
   flashing on Windows, or should the editor hand Windows users a Zadig
   walkthrough and stay pure-web? ([05](05-firmware-update.md) costs both.)
3. **Do you have the hardware to photograph?** If yes, a 40–60 photo
   photogrammetry pass gives a far better reference than anything generated from
   web images, and changes how I'd sequence [04](04-device-twin-3d.md).
4. **Repo layout.** In-repo (`configurator/` alongside the firmware, one version
   number, one CI) or a separate repo? I'd default to in-repo — the protocol
   schema has to be shared, and keeping it in one place is what stops drift.
