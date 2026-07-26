# Third-party code and assets

`neon_samurai` is MIT licensed (see [LICENSE](LICENSE)). This file records every
third-party component it contains, links to, or fetches at build time, so the
licence position of a built firmware image is auditable.

## Compiled into the firmware image

### LUFA — USB stack

- **Location:** fetched at configure time into `build/_deps/lufa-src/`
- **Upstream:** <https://github.com/nic-starke/lufa> (a fork), pinned in
  `CMakeLists.txt` to commit `0200c233f30774935822ddb2b963a7ae0675edd9`
- **Copyright:** Dean Camera, 2021 — <http://www.lufa-lib.org>
- **Licence:** MIT-style permissive ("Permission to use, copy, modify, and
  distribute this software and its documentation for any purpose is hereby
  granted without fee..."). Full text in `LUFA/License.txt` in the fetched tree.
- **Compatible with MIT:** yes.

The fork exists to carry local fixes. It is pinned to an explicit commit rather
than a branch so that a given checkout of this repository always builds the same
USB stack.

### `src/led/hsv2rgb.c`, `src/include/led/hsv2rgb.h` — HSV to RGB conversion

- **Copyright:** B. Stultiens, 2016
- **Licence:** MIT (full text reproduced at the top of both files)
- **Compatible with MIT:** yes.

Vendored rather than fetched because it is small, stable, and AVR-specific.

## Assets

### `firmware/bootloader_xmega_twister.hex` — Atmel DFU bootloader

- **Source:** <https://github.com/nic-starke/Midi_Fighter_Twister_Open_Source>
- **SHA-256:** `adb54909fea5e000119a6de1c4c1af2bbed8d53d651bed816d13cc3f0081e90d`
- **Origin:** the Atmel DFU bootloader as shipped by DJ TechTools on the Midi
  Fighter Twister. Not built by this project and not linked into the firmware —
  it occupies the MCU's separate boot section.

See [`firmware/README.md`](firmware/README.md) for provenance and verification
details.

### Gamma lookup table in `src/led/color.c`

Generated with <https://victornpb.github.io/gamma-table-generator> (gamma 2.20,
256 steps, range 0–255). Generated numeric data, not carrying a licence.

## Build-time only (not in the firmware image)

### `cmake/utils` — git submodule

- **Upstream:** <https://github.com/nic-starke/cmake-buildsystem>, a fork of
  Embedded Artistry's `cmake-buildsystem`
- **Copyright:** Embedded Artistry, 2020
- **Licence:** MIT (`cmake/utils/LICENSE`)

No longer required to configure the project — `CMakeLists.txt` stopped including
it. It is retained because it carries clang-tidy, cppcheck and unit-test CMake
modules that are worth adopting.

## USB vendor and product identifiers

`src/usb/usb_lufa.c` declares `idVendor = 0x2580`, `idProduct = 0x0007`. These
are DJ TechTools' identifiers for the Midi Fighter Twister, reused so that the
device continues to enumerate as the hardware it is running on.

This is not a licensing matter, but it is worth stating plainly: these
identifiers are not registered to this project. Anyone distributing this
firmware widely, or building derivative hardware, should obtain their own — for
open-source projects <https://pid.codes> allocates them free of charge.

## Previously included, now removed

### Quadrature decoder state table (removed 2026-07)

`src/encoder/quadrature.c` previously contained a state-transition table
attributed to Ben Buxton's *Rotary* Arduino library, marked "Licenced under the
GNU GPL Version 3". GPL-3 code cannot be redistributed under the MIT licence
this project uses, so it has been replaced.

The current decoder was re-derived from the encoder's Gray-code cycle. It
arrives at the same transitions — for a given cycle and half-step resolution
those are determined by the encoder's physics rather than chosen — and
`tests/test_quadrature.c` verifies the two are behaviourally identical across an
exhaustive walk of every input sequence, so the replacement is a drop-in.
