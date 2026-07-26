# 05 — Firmware update

Flashing NEON_SAMURAI from the browser. Getting *into* the bootloader is solved;
talking to it from a web page is the hard part, and it is hard for one specific
reason that no amount of good design removes.

## The chain

```
1. back up      SNAPSHOT_GET → save profile keyed to device_id
2. enter DFU    NSP ENTER_BOOTLOADER  (or corner gesture, or console cmd)
3. re-enumerate device drops off USB, reappears as Atmel DFU
4. flash        DFU over WebUSB
5. reset        device restarts into the new firmware
6. verify       reconnect, HELLO, check fw_version and schema_hash
7. restore      SNAPSHOT_SET if the EEPROM version changed and wiped config
```

Steps 1, 6 and 7 are the ones users will care about and that the stock utility
does not do. An automatic config backup before every flash, and an offer to
restore afterwards, turns "flashing is scary" into "flashing is fine".

Step 2 already has three routes in this firmware: `bootloader_start()` exposed as
a console command, the four-corner encoder gesture (encoders 0, 3, 12, 15), and —
once NSP exists — an `ENTER_BOOTLOADER` opcode. Belt and braces, which is right
given [the recovery warning in the README](../../README.md#before-you-flash).

## The problem: step 4 on Windows

The DJTT bootloader is the factory **Atmel DFU** bootloader in the ATxmega's boot
section. Two things follow.

**It is not standard DFU 1.1.** It presents as USB class `0xFE` / subclass `0x01`,
but the command set inside `DFU_DNLOAD` is Atmel's own — the one `dfu-programmer`
implements, not the one `dfu-util` speaks. So a generic WebDFU library will not
work; you need an Atmel-flavoured implementation. Good news: one already exists —
**[tmk/AVRFlashOnWeb](https://github.com/tmk/AVRFlashOnWeb)** is a WebUSB flasher
for exactly the Atmel AVR USB DFU bootloader. That is the thing to port or vendor.

**It has no WCID descriptors.** WebUSB can only claim an interface the OS has not
bound to a driver, and on Windows that requires a WinUSB driver. Modern devices
advertise MS OS 2.0 descriptors so Windows binds WinUSB automatically — but this
bootloader was burned at the factory years ago and does not. It cannot be changed,
because changing it means writing to the boot section, which is what you needed
the bootloader for.

Per platform:

| | Can Chrome claim the Atmel DFU interface? |
| --- | --- |
| **macOS** | Yes. Nothing binds it. |
| **Linux** | Yes, with a udev rule granting the user access to the device node. |
| **Windows** | **No**, until the user installs WinUSB over it with [Zadig](https://zadig.akeo.ie/). |

The Linux case is a documented one-liner. The Windows case is a third-party tool,
a driver swap, and a genuinely intimidating UI — for the exact users least
equipped to handle it. This is why QMK ships [a whole Zadig
guide](https://docs.qmk.fm/driver_installation_zadig).

## Three tiers, be honest about all of them

**Tier 1 — macOS and Linux: full in-browser flashing.** Enter DFU, WebUSB, done.
Linux users get a one-click "copy this udev rule" step with the exact `99-neosam.rules`
content and the `udevadm` reload command.

**Tier 2 — Windows: guided.** Detect Windows, detect that the DFU interface
cannot be claimed, and walk the user through Zadig *in the editor* — with the
right VID/PID pre-identified, screenshots, and a "check again" button that
re-probes. It works, it is a bad first-run experience, and pretending otherwise
helps nobody.

**Tier 3 — the native helper.** A small Tauri build using Rust `nusb`, which does
not need WinUSB at all on Windows. This is the actual fix. If firmware updating
matters to you — and for a firmware project distributed to strangers, it does —
this is worth building, and it is the same argument as
[01 § E](01-transport.md#e-native-helper-tauri-wrapping-the-same-web-ui): the same
editor bundle, one extra transport implementation.

My recommendation: **build tier 1 first**, ship tier 2 alongside it because it is
mostly documentation, and treat tier 3 as the answer to "how do non-technical
Windows users update?" when you get there. Do not block the editor on it.

## Safety

A flasher that bricks people's hardware is worse than no flasher. The
bootloader does not validate images, so validation is the editor's job.

- **Verify the image before writing.** Parse the Intel HEX / binary, check it fits
  the 128 KB application section, and refuse anything that would touch the boot
  section. The bootloader will not protect itself here — the boot section is
  physically separate, but a malformed image is still a bricked application.
- **Check the target.** `HELLO` gives `device_id` and the build gives an expected
  signature; refuse to flash a firmware image built for a different target.
- **Back up first, always.** Automatic `SNAPSHOT_GET` to a timestamped profile
  before entering DFU. No opt-out.
- **Surface the recovery path prominently.** Link
  [wiki/BootloaderRecovery.md](../../wiki/BootloaderRecovery.md) *before* the
  flash, not after it fails. The README's warning about blank boot sections
  needing a PDI programmer applies here and users should read it once, up front.
- **Offer stock DJTT firmware** as a first-class option in the same flow. "Go
  back to the official firmware" being a button rather than a forum search is a
  meaningful trust signal, and it costs nothing — it is the same DFU path.
- **Progress and interruption.** Real progress, an explicit "do not unplug", and
  a clear recovery message if the write fails mid-way (which is survivable — you
  are still in the bootloader; retry).

## Releases

For any of this to work the editor needs somewhere to get firmware from. A
release manifest published alongside GitHub releases:

```json
{
  "releases": [{
    "version": "0.4.1",
    "channel": "stable",
    "url": "https://github.com/nic-starke/neon_samurai/releases/download/v0.4.1/neosam.hex",
    "sha256": "…",
    "size": 18640,
    "nsp_schema": "0x8f21ac03",
    "eeprom_version": 11,
    "notes_url": "…"
  }]
}
```

`eeprom_version` is the field that matters operationally: the editor compares it
against the connected device and warns *before* flashing that the config will be
wiped and will need restoring from the backup it just took. `nsp_schema` tells the
editor whether it will still be able to talk to the device afterwards.

## Sources

- [tmk/AVRFlashOnWeb](https://github.com/tmk/AVRFlashOnWeb) — WebUSB flasher for the Atmel AVR USB DFU bootloader
- [WebDFU](https://devanlai.github.io/webdfu/) — the standard DFU 1.1 / DfuSe equivalent, for contrast
- [dfu-programmer](http://dfu-programmer.github.io/) — the reference Atmel DFU implementation
- [QMK: bootloader driver installation with Zadig](https://docs.qmk.fm/driver_installation_zadig)
- [wiki/Technical.md](../../wiki/Technical.md) · [wiki/BootloaderRecovery.md](../../wiki/BootloaderRecovery.md)
