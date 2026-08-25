<div align="center">
  <h1>NEON_SAMURAI</h1>

  <img src="./.github/logo.png" alt="NEON_SAMURAI" style="max-width: 500px; max-height: 200;">

<i>An alternative firmware for the Midifighter Twister</i>

  <h4 align="center">
    <a href="#introduction">Introduction</a> -
    <a href="#features">Features</a> -
    <a href="#before-you-flash">Before You Flash</a> -
    <a href="#building">Building</a> -
    <a href="#flashing">Flashing</a> -
    <a href="#documentation">Manual</a> -
    <a href="#documentation">Documentation</a> -
    <a href="#contributing">Contributing</a>
  </h4>

  <p>
    <img alt="contributors" src="https://img.shields.io/github/contributors/nic-starke/neon_samurai">
    <img alt="last commit (branch)" src="https://img.shields.io/github/last-commit/nic-starke/neon_samurai/main">
    <img alt="issues" src="https://img.shields.io/github/issues/nic-starke/neon_samurai">
    <a href="https://github.com/nic-starke/neon_samurai/blob/main/LICENSE"> <img alt="license" src="https://img.shields.io/github/license/nic-starke/neon_samurai"> </a>
  </p>

</div>

## Introduction

NEON_SAMURAI is an alternative firmware for the DJ Tech Tools Midi Fighter Twister. It focuses on extending the original functionality and aims to explore new features (see the features section for more info).

If you use this software you must accept the License, please be aware that using this firmware will likely invalidate your warranty with DJ Tech Tools.

## Features

KEY:

- ✅ Feature implemented.
- ⏲ Feature planned, not complete yet.
- Not planned.

| Feature                      |    Status   |
| ---------------------------- | :---------: |
| Configurable Channels        |     ✅      |
| Acceleration                 |     ✅      |
| Firmware Recovery (DJTT)     |     ✅      |
| 14-bit CC/NRPN               |     ✅      |
| Sysex-based Configuration    |     ✅      |
| LFOs                         |     ⏲       |
| Virtual Banks                |     ⏲       |
| HID - Mouse/Keyboard         |     ⏲       |
| Standalone Configuration     |     ⏲       |
| Improved Button/Switch Modes |     ⏲       |
| Hyper Knobs                  |     ✅      |
| Open Sound Control (OSC)     |     ⏲       |
| Traktor Sequencer            | Not planned |
| Midi 2.0                     | Not planned |

## Before You Flash

> **Warning:** Make sure your device has a working recovery path before
> flashing custom firmware. NEON_SAMURAI can enter the bootloader via the
> top-left encoder gesture or the `bootloader` console command, but both
> depend on the ATxmega's boot section already containing a working DFU
> bootloader. If that section is blank neither path works and the device requires a PDI
> programmer (e.g. a JTAGICE3) to recover. See
> [Bootloader recovery](docs/manual/11-bootloader-recovery.md) if you hit this.

<i>Using this firmware will void your warranty, the software is provided "as is", without warranty of any kind. Please refer to the license.</i>

## Download

Releases are published on the
[releases page](https://github.com/nic-starke/neon_samurai/releases), each
with a `.hex` to flash and a checksum. See the manual's
[Installing](docs/manual/02-installing.md) page before you flash anything.

## Building

```sh
cmake -G Ninja -B build/Release -S . --toolchain=cmake/toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build/Release
```

`Debug` and `RelWithDebInfo` build types are also supported - see
`.vscode/tasks.json` for the full set of configure/build/flash tasks.

## Flashing

```sh
scripts/flash.sh [Debug|Release|RelWithDebInfo]
```

Defaults to `RelWithDebInfo`. This only writes the application section -
see [Bootloader recovery](docs/manual/11-bootloader-recovery.md) for flashing
the bootloader itself, which is a separate, one-time operation per device.

### Configuring

Settings are changed from the browser editor in [webui/](webui/README.md),
which talks to the device over Web MIDI. It needs Chrome or Edge - Firefox
does not implement Web MIDI.

## Documentation

The **[user manual](docs/manual/)** is the place to start if you are using the
device rather than working on the firmware. The same text appears as contextual
help inside the browser editor - both are generated from `docs/manual/` by
`tools/docs/build_manual.py`, so there is only ever one copy of it.

The manual and the editor are not currently published anywhere; build and read
them locally:

```sh
pip install markdown pyyaml
python3 tools/docs/build_manual.py
python3 -m http.server 8420
```

Then open `http://127.0.0.1:8420/site/` for the manual, or
`http://127.0.0.1:8420/webui/` for the editor. Serve from the repository root -
the editor reads paths above `webui/`.

Developer-facing notes live separately:

- [Technical reference](docs/manual/12-technical.md) - bootloader, memory layout
- [Bootloader recovery](docs/manual/11-bootloader-recovery.md) - recovering a unit with a blank boot section
- [Chip specifications](docs/manual/13-specifications.md)
- [Code structure](docs/Code%20Structure.md)

## Testing

```sh
cmake -S tests/unit -B build/tests && cmake --build build/tests
ctest --test-dir build/tests --output-on-failure
```

The unit suite builds the hardware-independent modules for the host with
ASan and UBSan enabled. Hardware-in-the-loop tests, which need a real
device attached, live in [tests/robot](tests/robot/README.md) and are run
by hand rather than in CI.

Static analysis:

```sh
tools/analysis/run-analyzer.sh
```

## Contributing

Contributions to make **neosam** even better are welcomed. If you'd like to get involved, you can:

- Report issues or suggest enhancements on our [Issue Tracker](https://github.com/nic-starke/neon_samurai/issues).
- Fork the project, make changes, and submit a pull request to have your improvements considered for inclusion.
- Join our [Discussions](https://github.com/nic-starke/neon_samurai/discussions) to share your ideas, ask questions, or connect with other users.

### Development Environment

#### Dependencies

- avr-gcc
- avr-libc
- python3
- meson
- cmake
- ninja
- avrdude (for programming flash)
- [bloom](https://bloom.oscillate.io) (for hardware debugging via JTAGICE3/PDI)
- dfu-programmer (for flashing over USB DFU once a bootloader is present - `dfu-util` does not work with Atmel's DFU variant)
- pre-commit (for githooks)

#### Setting up pre-commit

- install pre-commit
- run `pre-commit install-hooks` and then `pre-commit install` in the root of the project from a terminal.

#### Setting up the build system

- Select cpp configuration `midifighter`
- Run the user task `CMake Configure`
- Run the default build task.

#### udev Rules for AVR Programmers

`/etc/udev/rules.d/50-avr-isp.rules`

```bash
SUBSYSTEM!="usb", ACTION!="add", GOTO="avrisp_end"

#Atmel Corp. JTAG ICE mkII
ATTR{idVendor}=="03eb", ATTR{idProduct}=="2103", MODE="660", GROUP="dialout"
#Atmel Corp. AVRISP mkII
ATTR{idVendor}=="03eb", ATTR{idProduct}=="2104", MODE="660", GROUP="dialout"
#Atmel Corp. Dragon
ATTR{idVendor}=="03eb", ATTR{idProduct}=="2107", MODE="660", GROUP="dialout"
#Atmel Corp. ATMEL-ICE
ATTR{idVendor}=="03eb", ATTR{idProduct}=="2141", MODE="660", GROUP="dialout"
#Atmel Corp. JTAGICE3
ATTR{idVendor}=="03eb", ATTR{idProduct}=="2140", MODE="660", GROUP="dialout"
```

If you install [bloom](https://bloom.oscillate.io) via its AUR package, it
installs its own broader udev rule set (covering more debug probes, and the
Atmel DFU bootloader itself) - the rules above are only needed if you're
not using Bloom.

### Contributors

<a href="https://github.com/nic-starke"><img src="https://avatars.githubusercontent.com/u/10380155?v=4" title="nic-starke" width="75" height="75"></a>
<a href="https://github.com/deepc0py"><img src="https://avatars.githubusercontent.com/u/17808066?v=4" title="deepc0py" width="75" height="75"></a>



### Sponsors

## Analytics

![Alt](https://repobeats.axiom.co/api/embed/349b4dffd3819c8746b8d91e4de04beaabb05ebe.svg "Repobeats analytics image")

## License

SPDX-License-Identifier: MIT
