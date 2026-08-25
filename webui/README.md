# NEON_SAMURAI digital twin

A minimal, dependency-free browser-based **live digital twin** of the
Midi Fighter Twister, talking to a real device over Web MIDI + sysex. No
framework, no build step, no `npm`/`node_modules` - vanilla HTML/CSS/JS,
loaded directly as ES modules.

Connect, and watch the physical device's real state - encoder colours,
detent, display mode, and live knob rotation - render as a skeuomorphic
chassis in the browser. The one thing you can change from this page is the
active bank; everything else is read-only. It is a viewer, not a
configuration tool.

`js/protocol.js` carries only the two setters the live view uses
(`setActiveBank`, `setLivePositionStreaming`). An editing UI would add the
rest back against `tests/robot/lib/NeonSamuraiLibrary.py`, which keeps the
full keyword set - note that HSV is the device's writable colour source of
truth and RGB a read-only derived mirror, so the setter to add is
`setVmapHsv`, not `setVmapRgb`.

## Browser support

Web MIDI with sysex access (`{sysex: true}`) is **Chrome or Edge only** as
of this writing. Firefox does not implement Web MIDI by default. Safari's
support is partial/recent. The page detects this
(`navigator.requestMIDIAccess` missing) and shows a banner rather than
failing silently - if you see that banner, switch browsers rather than
trying to work around it.

## Running

Serve the directory over HTTP - **do not** open `index.html` via a
`file://` URL, which can block sysex permission in some Chromium versions
(and separately, ES module imports are blocked cross-origin under
`file://` regardless of sysex):

```sh
cd webui
python3 -m http.server 8420
```

Then open <http://localhost:8420/> in Chrome or Edge, and click **Connect**.
(<http://localhost:8420/test.html> runs the self-test suite - see "Tests"
below - and needs no device.)

On a successful connection the page immediately pulls the
device's full configuration (colours, detent, display mode, side-switch
modes, active bank) with no further action needed, then starts live
knob-position tracking - see "How live tracking works" below.

While disconnected, every LED renders in a dim "powered off" state (see
`design-system/tokens.css`'s `--ds-led-powered-off`) rather than showing
stale or default-looking state that could be mistaken for a live reading.

### Firmware version requirement

This page depends on sysex protocol fixes that landed after the original
firmware was written: working `GET` (read-back), the `MF_SYSEX_PARAM_HSV`
color-setting param, 7-bit MIDI data packing. Firmware built before those
commits will not work correctly - the initial config pull on connect in
particular will hang or return garbage on older firmware.

## How live tracking works

Two different kinds of "live" are involved, from two different sources:

- **Configuration** (colour, detent, display mode, side-switch modes,
  active bank) - pulled via sysex `GET` immediately on connect (see
  `js/editor.js`'s `onConnectClick()`). This is a snapshot, not
  continuously polled - if you change the device's configuration through
  some other tool while this page is connected, reconnect to see it.
- **Knob rotation** - `MF_SYSEX_PARAM_VMAP_CURR_POS` is read explicitly
  (an ordinary `GET`, one per vmap) as part of the initial config pull
  (see `device-model.js`'s `loadFromDevice()`), so every encoder shows
  its real position immediately on connect, not a neutral placeholder.
  From then on the firmware pushes updates to a connected client itself,
  unsolicited: the same param is sent again as a `WEBUI_PUSH`-shaped
  `{bank, enc, vmap, curr_pos}` message on every subsequent encoder
  movement, but only while streaming is enabled
  (`MF_SYSEX_PARAM_ENCODER_LIVE_POSITION_STREAM`, a `SET`-only trigger -
  see `sysex.h`). `js/editor.js` seeds its `LivePushTracker` instances
  from the just-loaded model and then enables streaming right after the
  initial config pull; the trackers listen for the pushed messages via
  `Device.onSysex()` (see `midi.js`) rather than polling or inferring
  anything - `curr_pos` is exact, not
  derived from MIDI CC output. Streaming defaults off and resets to off
  on every device reboot, so a device that's never had a web client
  connect emits zero extra sysex traffic; this page also makes a
  best-effort attempt to disable it again on page unload (see
  `editor.js`'s `beforeunload` handler). Only an encoder that moves
  *between* connecting and streaming actually being enabled (a narrow
  window) could show a briefly stale position; anything else is exact
  from the moment the page connects.

  This was previously derived by listening for the MIDI CC traffic an
  encoder's colour layer happened to transmit and inverting firmware's
  range/position mapping - workable, but lossy: firmware only transmits
  *absolute* 7-bit CC, so a knob turned past what one CC byte can express
  reads as stuck at 0/127 with no way to tell "parked at the rail" from
  "still turning past it". The sysex push above replaced that as the
  real mechanism; see git history on `js/live-position.js` (since folded
  into `js/live-tracker.js`) if you need the CC-based version.

## Digital twin rendering

The chassis - plastic faceplate, rubber bevel, 16 knurled encoder caps
with LED rings and RGB indicator arcs, 6 side switches - is built from a
small component library in [`design-system/`](design-system/) (see that
directory's own README for the full component catalog, the "Matrix"
bluey-green monochrome palette `tokens.css` defines for the surrounding
chrome, and the two-light knurl-shading system that makes the 16 caps
read as one lit surface rather than 16 identical stickers).

Two pages use it:

- **`index.html`** (`js/editor.js`) - the live device view described
  above.
- **`twin.html`** (`js/twin.js`) - a standalone geometry/colour tuning
  tool, open directly with no connection involved. Renders demo state (a
  fixed value ramp, a small rainbow palette across encoders) - it exists
  to dial in chassis/encoder/cap geometry and defaults, not to mirror
  live hardware state. Both pages read their chassis geometry from
  `design-system/geometry.js`, so retuning one retunes both; `twin.js`'s
  `SPEC` adds only the cap-colour fields on top.

## Structure

```text
webui/
  index.html            - live device view page markup
  editor.css          - chrome styling for index.html (header/toast/
                          banner - the chassis itself is styled inline
                          by design-system/components/*)
  twin.html              - standalone digital-twin geometry/colour tuning tool
  twin.css                - chrome styling for twin.html
  test.html               - dependency-free self-test page (see "Tests")
  check-imports.py        - resolves every relative import (see "Tests")
  README.md              - this file
  design-system/
    geometry.js             - chassis geometry shared by both pages
    tokens.css              - design tokens: palette, spacing, glow effects
    README.md              - component catalog, aesthetic rationale
    components/
      index.js                - barrel re-export; import from here
      dom.js                    - elc()/svgEl() DOM helpers
      color-utils.js           - cosmetic HSV/hex material-shading math
      chassis.js                 - faceplate + rubber bevel (static)
      device-chassis.js          - full assembled device: chassis + 4x4
                                  grid + side switches + two-light knurl
                                  system (composes chassis.js/encoder.js/
                                  side-switch.js) - shared by twin.js and
                                  editor.js
      encoder.js                  - composed encoder assembly (see below)
      cap.js                        - knurled cap top-view SVG, incl. the
                                  subtle coloured knurl-reflection effect
      led-ring.js                  - indicator LED ring
      led-mask.js                  - firmware-accurate lit-LED bitmask math
                                  (ports src/led/led.c's LUTs)
      rgb-arc.js                    - RGB backlight indicator arc
      vmap-pill.js                   - small A/B active-vmap indicator pill
      side-switch.js                - one side switch button
  js/
    sysex.js            - wire-protocol encode/decode (JS port of
                         tests/robot/lib/sysex.py - keep both in sync)
    midi.js              - Web MIDI access, connection, and sysex request/
                         response correlation (Device.request()/onSysex())
    protocol.js         - per-param GET/SET methods on top of midi.js,
                         incl. setLivePositionStreaming()/getVmapCurrPos()
    device-model.js    - in-memory banks[3].encoders[16].vmaps[2] +
                        sideSwitches[6], device load orchestration
    live-tracker.js      - LivePushTracker: one per unsolicited field
                        (knob position, active vmap, active bank) - see
                        "How live tracking works" above
    encoder-signature.js - props -> short string, for the render diff
    bank-fade.js         - reproduces the firmware's bank-change LED
                        flicker; suppressed under prefers-reduced-motion
    color.js              - HSV->RGB matching the firmware's color model
                         (src/led/hsv2rgb.c, src/led/color.c) - distinct
                         from design-system/components/color-utils.js's
                         cosmetic HSV, see that file's header comment
    editor.js         - connect flow + state + render orchestration
                         for index.html
    twin.js               - state and control panel for twin.html
    vendor/
      signals-core.js        - vendored @preact/signals-core (MIT), used by
                            twin.js for reactive state - see its header
                            comment and design-system/README.md for why
                            it's vendored rather than CDN-imported
      signals-core.LICENSE
```

## Rendering

`editor.js` builds the chassis once, then updates it in two stages:

1. **Frame coalescing.** Every live data source calls `scheduleRender()`,
   never `renderChassis()` directly. The firmware pushes `curr_pos` on every
   quadrature tick - hundreds a second while a knob turns - and this collapses
   those into at most one render per animation frame.
2. **Per-encoder diffing.** A render computes each encoder's props, reduces
   them to a short string via `js/encoder-signature.js`, and rebuilds only the
   cells whose signature changed. `buildDeviceChassis()` returns an
   `encoderCells` array for exactly this. Turning one knob replaces one
   encoder, not sixteen.

Components themselves stay pure prop-driven functions with no identity or
lifecycle - the diffing happens above them, on props. That keeps the
"add a third caller" contract in `design-system/README.md` intact.

Anything rendered inside a rebuilt encoder is still discarded and recreated,
so don't put a CSS transition or a focusable element inside one and expect it
to survive. The bank selector sits outside the chassis for that reason.

## Tests

`test.html` is the whole test suite for `webui/`: 67 checks over the sysex
codec, the firmware LED-mask port, the colour model, the design-system
components, the render signature, and the connection lifecycle - the last of
those driven against a `FakePort` stand-in for a MIDI port, so `Device`'s
request correlation, listener cleanup and heartbeat pausing are all covered
without hardware. Serve the directory and open it.

`check-imports.py` verifies every relative import resolves and that the names
imported are actually exported. With no bundler or type checker, a renamed
export otherwise fails only at runtime, in a browser, on whichever code path
happens to touch it.

Both run in CI on any change under `webui/` - see
[`.github/workflows/webui.yml`](../.github/workflows/webui.yml). The test page
stamps `PASS`/`FAIL` into its `<title>`, which is how a headless Chrome DOM
dump is turned into a pass/fail gate without a driver protocol.

Anything needing real hardware belongs in `tests/robot/` instead.

`sysex.js` is a from-scratch reimplementation of the wire protocol
(mirroring `src/midi/sysex.c`/`sysex.h`), not a binding to the firmware's
C code - the same approach `tests/robot/lib/sysex.py` takes, and for the
same reason: two independent implementations of the same protocol are a
better cross-check than one implementation both testing and driving
itself. If the firmware's sysex protocol changes, update `sysex.js`,
`sysex.py`, and the relevant `.h`/`.c` files together - see
[../tests/robot/README.md](../tests/robot/README.md)'s "Adding a new
test" section for the same principle applied to the test suite.

## Adding a new sysex param

1. Add it to `enum mf_sysex_param` in `src/include/midi/sysex.h`, plus
   its wire struct (reuse an existing `mf_sysex_*_param_s` shape - e.g.
   `mf_sysex_vmap_param_s` for anything addressed by bank/enc/vmap - if
   one already fits) and a `sysex_data_info[]` entry in `src/midi/sysex.c`.
2. Mirror the same param name/numeric value in `webui/js/sysex.js`'s
   `Param` object and `tests/robot/lib/sysex.py`'s `Param` enum - both
   must match `mf_sysex_param` exactly, and each other. Add a
   `*Payload()` builder in both.
3. Add `get*`/`set*` methods to `protocol.js`'s `Protocol` class and a
   matching keyword to `tests/robot/lib/NeonSamuraiLibrary.py` - see
   [../tests/robot/README.md](../tests/robot/README.md)'s "Adding a new
   test" section.
4. Wire it into `device-model.js` (the field itself, plus
   `loadFromDevice()`) and, if it should affect the live render,
   `editor.js`'s `renderChassis()`.

If the param is *pushed unsolicited* rather than only read on request
(like `VMAP_CURR_POS`/`ENCODER_VMAP_ACTIVE`), the firmware side needs
more: a `MF_SYSEX_WEBUI_PUSH`-shaped push, gated on
`gRT.live_position_streaming` so an unlistened stream doesn't run
forever. Two patterns exist depending on how often the field changes:

- **Low-frequency changes** (once per switch press, sysex write, etc.) -
  route through `EVENT_CHANNEL_IO`: post an `EVT_IO_ENCODER_FIELD_CHANGED`
  event (see `set_vmap_active()` in `src/io/input_manager.c`) and add a
  case in `src/midi/webui_bridge.c`'s handler that calls a small exported
  `sysex_push_*()` function in `sysex.c` (see `sysex_push_vmap_active()`).
  Every mutation site (input handling, sysex `SET`, wherever else the
  field can change) just posts the same event - none of them need to know
  a web-ui or sysex exists.
- **High-frequency changes** (every quadrature tick while an encoder
  turns, like `curr_pos`) - push inline instead, matching
  `vmap_update()` in `input_manager.c`. The event-channel hop is real but
  negligible overhead per-call; at hundreds of calls/sec while turning
  it's the one case where skipping it is worth the inconsistency.
