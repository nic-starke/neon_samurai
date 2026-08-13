# NEON_SAMURAI web config GUI

A minimal, dependency-free browser-based configuration tool for the
neon_samurai firmware, talking to a real device over Web MIDI + sysex. No
framework, no build step, no `npm`/`node_modules` - vanilla HTML/CSS/JS,
loaded directly as ES modules.

## Browser support

Web MIDI with sysex access (`{sysex: true}`) is **Chrome or Edge only** as
of this writing. Firefox does not implement Web MIDI by default. Safari's
support is partial/recent. The page detects this
(`navigator.requestMIDIAccess` missing) and shows a banner rather than
failing silently - if you see that banner, switch browsers rather than
trying to work around it.

## Running

Serve the directory over HTTP - **do not** open `index.html` via a
`file://` URL, which can block sysex permission in some Chromium versions:

```sh
cd webui
python3 -m http.server 8420
```

Then open <http://localhost:8420/> in Chrome or Edge, connect a
neon_samurai device (flashed with firmware built from this repo including
the sysex protocol fixes - see the note below), and click **Connect**.

### Firmware version requirement

This GUI depends on sysex protocol fixes that landed after the original
firmware was written: working `GET` (read-back), the `MF_SYSEX_PARAM_HSV`
color-setting param, 7-bit MIDI data packing, and the
`SYSTEM_RESET`/`CONFIG_RESET` triggers. Firmware built before those
commits will not work correctly with this GUI - "Load from device" in
particular will hang or return garbage on older firmware. The GUI reads
`MF_SYSEX_PARAM_DEVICE_INFO`'s firmware version on connect and will warn if
it looks too old (see `ui.js`'s connection handler), but treat that as a
best-effort check, not a guarantee.

## Firmware flashing

**Not done by this GUI.** Flashing new firmware over USB DFU would need a
from-scratch reimplementation of Atmel's proprietary DFU protocol variant
in WebUSB (confirmed `dfu-util`, the generic USB-IF DFU tool, does not
work against this bootloader - only `dfu-programmer` does) - a
substantial, higher-risk undertaking kept deliberately out of scope for
this tool. To flash new firmware, use the existing CLI workflow:

```sh
scripts/flash.sh [Debug|Release|RelWithDebInfo]
```

See [../wiki/BootloaderRecovery.md](../wiki/BootloaderRecovery.md) if the
device needs its bootloader recovered first, and the
[build-flash-debug](../.claude/skills/build-flash-debug/SKILL.md) skill
for the full build/flash/debug workflow.

## What's here (v1)

- **Per-encoder**: display mode, detent, switch mode, and two color
  layers ("Layer A"/"Layer B", i.e. `virtmap[0]`/`virtmap[1]`) each with
  HSV color, value range, physical rotation window (position), and MIDI
  proto config (mode/channel/CC or note number).
- **Side switches** (6): mode selection.
- **Banks** (3): browse each bank's config independently of which bank is
  currently active on the device; a separate, explicit action sets the
  device's live active bank, so browsing never accidentally live-switches
  hardware state.
- **Load from device** / **Save to device**: full round-trip against
  every encoder/vmap/side-switch/bank.
- **Local presets**: save/load the current configuration as a downloadable
  JSON file. Purely client-side - loading a preset does not touch the
  device until you explicitly click "Save to device" afterward.
- **Factory reset**: triggers `MF_SYSEX_PARAM_CONFIG_RESET` on the device
  (confirms first - this wipes the device's stored configuration).

### Deliberately not in v1 (see the project's own notes on why)

- 14-bit CC / NRPN, LFOs, OSC protocol - none have a working firmware
  implementation yet (see the `module-architecture` skill's notes); the
  GUI doesn't expose controls for things that would silently do nothing.
- Global timing settings (`enc_dead_time`, `midi_throttle_time`) - not
  EEPROM-persisted or sysex-addressable in firmware yet.
- WebUSB DFU firmware flashing - see above.

## Digital twin (`twin.html`)

A standalone, skeuomorphic render of the physical Twister chassis - 16
knurled encoder caps, LED rings, RGB indicator arcs, side buttons - with a
live geometry-tuning sidebar, ported from a Claude Design Canvas
prototype. Open `twin.html` directly (no connection required); "Connect"
and the rest of `index.html`'s device flow are not part of it.

It renders demo state (a fixed value ramp, a small rainbow palette across
encoders), not anything read from a real device - it exists to preview
and dial in chassis/encoder/cap geometry, not to mirror live hardware
state. If a future revision of the main config GUI's encoder grid wants
this look, `js/twin-render.js`'s builders are written DOM-only /
state-agnostic so they could be reused there directly; `js/twin.js` is
just this page's own demo state and control panel.

## Structure

```text
webui/
  index.html       - page markup
  style.css         - all styling, no preprocessor
  twin.html         - standalone "digital twin" device-geometry preview
                       (see below) - not wired to a real device
  twin.css           - styling for twin.html
  README.md        - this file
  presets/           - example/starter preset JSON files (empty for now)
  js/
    sysex.js          - wire-protocol encode/decode (JS port of
                         tests/robot/lib/sysex.py - keep both in sync)
    midi.js            - Web MIDI access, connection, request/response
                         correlation
    protocol.js       - per-param GET/SET methods on top of midi.js
    device-model.js  - in-memory banks[3].encoders[16].vmaps[2] +
                        sideSwitches[6], device load/save orchestration
    color.js            - HSV<->RGB matching the firmware's color model
                         (src/led/hsv2rgb.c, src/led/color.c)
    storage.js         - local preset save/load (client-side only)
    ui.js                - DOM rendering and event wiring; the only
                         module that touches the DOM
    twin-render.js    - digital-twin rendering primitives (chassis/
                         encoder/cap SVG+DOM builders), used only by twin.js
    twin.js             - state and control panel for twin.html
```

`sysex.js` is a from-scratch reimplementation of the wire protocol
(mirroring `src/midi/sysex.c`/`sysex.h`), not a binding to the firmware's
C code - the same approach `tests/robot/lib/sysex.py` takes, and for the
same reason: two independent implementations of the same protocol are a
better cross-check than one implementation both testing and driving
itself. If the firmware's sysex protocol changes, update `sysex.js`,
`sysex.py`, and the relevant `.h`/`.c` files together - see
[../tests/robot/README.md](../tests/robot/README.md)'s "Adding a new
test" section for the same principle applied to the test suite.

## Adding a new sysex param to the GUI

1. Add it to `sysex.js`'s `Param` object, matching `enum mf_sysex_param`
   in `src/include/midi/sysex.h` exactly (same names, same numeric
   values).
2. Add a `*Payload()` builder function in `sysex.js`, matching the new
   param's wire struct.
3. Add `get*`/`set*` methods to `protocol.js`'s `Protocol` class.
4. Wire it into `device-model.js` (the field itself, plus
   `loadFromDevice()`/`saveToDevice()`) and `ui.js` (a control in the
   detail panel or wherever it belongs).
