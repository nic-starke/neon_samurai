# 01 — Transport

How the browser reaches the device. This is the first decision because it
constrains the protocol, the telemetry rate the 3D twin can run at, and which
browsers the editor works in at all.

## Requirements

| | |
| --- | --- |
| Config traffic | ~960 parameters, bursty. A full read must complete in well under a second. |
| Telemetry | 16 encoder positions + 16 switch states + optionally 208 LED values, at 30–60 Hz, to drive the twin. |
| Coexistence | The user's DAW is usually running. The editor must not steal the device from it. |
| Payload | 8-bit clean. Colour is `u16` hue + `u8` sat/val; ranges are signed. |
| Reach | Works on the machines the users actually have, without a driver install. |

## The five options

### A. Web MIDI + SysEx

The device is already a MIDI class device, so this works against today's
firmware with no USB changes.

- **Browsers**: Chrome/Edge/Opera by default; Firefox 108+ via a one-time site
  permission add-on. **Safari refuses**, on fingerprinting grounds, with no
  roadmap. SysEx needs a separate `sysex: true` grant.
- **Payload**: every byte in a SysEx frame must be ≤ 0x7F. Needs an 8-in-7 packing
  layer (7 payload bytes → 8 wire bytes, +14.3%). Cheap, but it must exist —
  the current firmware does not have it, which is why colour is unsendable today.
- **Throughput**: USB-MIDI packs 3 SysEx bytes per 4-byte USB event, 64-byte
  bulk packets, 1 ms frames. Ceiling around 48 KB/s, realistically far less once
  the AVR's event loop is in the path. Fine for config, marginal for 60 Hz LED
  telemetry.
- **The coexistence problem**: on Windows, the pre-2025 MIDI stack gives
  **exclusive** access to a MIDI port. If Ableton has the Twister open, the
  editor cannot open it, and vice versa. Windows MIDI Services (Win11) fixes
  this with multi-client, but you cannot rely on the user having it. On
  macOS/Linux, ports are shared and this is a non-issue.
- **Verdict**: keep as the **universal fallback**. It is the only option that
  works on unmodified firmware and on Firefox, and that matters for a project
  where users may be on an older build. It should not be the primary path.

### B. Vendor HID interface — *recommended primary*

Add a vendor-defined HID interface (usage page `0xFF60`) alongside the existing
MIDI interface.

- **Browsers**: WebHID — Chromium only (Chrome, Edge, Opera, Brave, Arc). No
  Firefox, no Safari.
- **Drivers**: none, on any OS. The OS HID class driver binds it; WebHID claims
  vendor-defined usage pages without a driver swap. **This is the property that
  makes it the right default** — it is precisely why VIA and Vial use HID for
  keyboard configuration and why they "just work" for non-technical users.
- **Payload**: 8-bit clean, 64-byte reports (32 on the XMega if endpoint budget
  is tight — the ATxmega128A4U has 16 endpoint pairs, so 64 is comfortable).
- **Throughput**: 64 B per 1 ms interval per direction ≈ 64 KB/s. A full config
  read is ~2 KB packed, so tens of milliseconds. 60 Hz telemetry with all 208 LED
  values is ~150 KB/s uncompressed, which does **not** fit — so telemetry sends
  encoder/switch state at 60 Hz (~64 B/frame, trivial) and LED state as a delta
  or on request. See [02](02-protocol.md#telemetry).
- **Coexistence**: separate interface from MIDI, so the DAW keeps the MIDI port
  and the editor keeps the HID interface. Solves the Windows exclusivity problem
  outright.
- **Cost**: a HID descriptor, an endpoint pair, and a report handler in the
  firmware. LUFA has HID device class support already and the descriptor
  struct in `src/usb/usb_lufa.c` already has a `#ifdef HID_ENABLE` block stubbed
  out for keyboard/mouse — the scaffolding is half there.

### C. WebUSB vendor interface

Add a vendor-specific (class `0xFF`) interface with bulk endpoints, plus a
WebUSB BOS descriptor and MS OS 2.0 descriptors so Windows auto-binds WinUSB.

- **Browsers**: Chromium only.
- **Drivers**: driverless on Windows 10+ *if* you get the MS OS 2.0 descriptors
  right (this is the "WCID" trick). Linux needs a udev rule granting the user
  access to the device node — a real papercut you cannot fix from the web page.
  macOS fine.
- **Throughput**: full-speed bulk, ~1 MB/s ceiling. Comfortably enough for
  everything including full 60 Hz LED telemetry.
- **Verdict**: strictly better than HID on throughput, strictly worse on
  friction (the Linux udev rule, and MS OS 2.0 descriptors are fiddly to get
  right on a device you cannot easily debug). **Not worth it for a config editor**
  — HID's 64 KB/s is already 30× what the config needs. Keep WebUSB in reserve for
  DFU, where it is unavoidable ([05](05-firmware-update.md)).

### D. Web Serial over the existing CDC console

`ENABLE_CONSOLE` already builds a CDC ACM interface. Web Serial can open it today.

- **Browsers**: Chromium only.
- **Drivers**: bound by the OS's built-in usbser/cdc_acm. Works everywhere. On
  Linux the user needs `dialout` group membership — same class of papercut as
  WebUSB's udev rule.
- **Cost**: zero descriptor work, which is genuinely tempting for a fast first
  cut. But the channel currently carries a human-readable text console, so you
  would have to multiplex framed binary with line-oriented text, and CDC is only
  compiled in on console builds.
- **Verdict**: **not the config transport**, but keep it. Expose the console as a
  terminal pane in the editor's debug drawer — that is free, useful, and exactly
  what it is for.

### E. Native helper (Tauri) wrapping the same web UI

Ship the identical editor bundle inside a Tauri shell that provides native USB,
MIDI, and DFU via Rust.

- **Reach**: everyone, including Safari-only and Firefox-only users, and Windows
  DFU without Zadig.
- **Cost**: a second distribution channel, code signing, updates.
- **Verdict**: not phase one. But design the transport interface so this drops in
  later as one more implementation, rather than being a rewrite. That costs
  nothing now and buys the option.

## Recommendation

```
                    ┌─────────────────────────────┐
                    │  editor  (transport-agnostic)│
                    └──────────────┬──────────────┘
                                   │  Transport interface
        ┌──────────────┬───────────┴───────┬──────────────┐
        │              │                   │              │
   ┌────▼────┐   ┌─────▼─────┐      ┌──────▼──────┐  ┌────▼─────┐
   │ WebHID  │   │ Web MIDI  │      │ Web Serial  │  │  Tauri   │
   │ primary │   │  SysEx    │      │ CDC console │  │ (later)  │
   │         │   │ fallback  │      │  debug only │  │          │
   └─────────┘   └───────────┘      └─────────────┘  └──────────┘
   driverless    works on old fw    text console      everything
   all OSes      + Firefox          only              incl. Safari
```

One TypeScript interface, several implementations:

```ts
interface Transport {
  readonly kind: 'hid' | 'midi-sysex' | 'serial' | 'native';
  readonly maxPduBytes: number;      // 64 for HID, ~48 effective for SysEx
  readonly supportsTelemetry: boolean;
  open(): Promise<void>;
  close(): Promise<void>;
  send(pdu: Uint8Array): Promise<void>;
  onReceive(cb: (pdu: Uint8Array) => void): () => void;
  readonly state: 'closed' | 'opening' | 'open' | 'error';
}
```

The protocol layer above this is identical in every case — only the framing shim
below it differs (8-in-7 packing for SysEx, report ID prefix for HID). That is
the whole point of [02](02-protocol.md).

**Connection flow in the UI:** enumerate HID first; if the device answers `HELLO`
on HID, use it. If not (older firmware), fall back to Web MIDI and say so in the
status bar, with a "your firmware predates the fast connection — update for
better performance" nudge. Never make the user pick a transport; make it
diagnosable when it goes wrong.

## Browser support summary

| | Chrome/Edge | Firefox | Safari |
| --- | :---: | :---: | :---: |
| Web MIDI (+SysEx) | ✅ | ✅ via add-on | ❌ |
| WebHID | ✅ | ❌ | ❌ |
| WebUSB | ✅ | ❌ | ❌ |
| Web Serial | ✅ | ❌ | ❌ |
| WebGL2 / WebGPU (for the twin) | ✅ | ✅ | ✅ |

Practical read: **Chromium is the target**, Firefox gets a degraded-but-working
SysEx path, Safari gets a read-only demo with the simulator and a "download the
app" prompt. Say this plainly on the landing page rather than letting Safari
users hit a wall.

## Sources

- [Web MIDI API — MDN](https://developer.mozilla.org/en-US/docs/Web/API/Web_MIDI_API)
- [Access to MIDI devices now requires user permission — Chrome for Developers](https://developer.chrome.com/blog/web-midi-permission-prompt)
- [Web MIDI in 2026: Which Browsers Actually Work](https://www.supersimplepiano.com/blog/web-midi-browser-compatibility-2026)
- [WebUSB API — MDN](https://developer.mozilla.org/en-US/docs/Web/API/WebUSB_API)
- [Web Serial API — MDN](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API)
- [Understanding WebHID and WebUSB — Configur.io](https://blog.jonathanlau.io/posts/understanding-webhid-and-webusb-configur/)
- [awesome-webhid](https://github.com/citterio/awesome-webhid)
- Prior art: [Phaeilo/mft-web-config](https://github.com/Phaeilo/mft-web-config), a
  browser config utility for stock MFT firmware, unmaintained since ~2021
  ([forum thread](https://forum.djtechtools.com/t/browser-based-configuration-utility-for-midi-fighter-twister/154794))
