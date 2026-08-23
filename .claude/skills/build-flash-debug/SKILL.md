---
name: build-flash-debug
description: Use whenever building, flashing, or debugging the neon_samurai firmware — "build the firmware", "build debug/release", "flash the board", "start a debug session", or anything involving CMake configure/build, avrdude, dfu-programmer, or Bloom/GDB in this repo. Encodes the exact CMake+Ninja multi-build-type layout, flashing commands, and the bootloader-recovery safety constraint so commands work without re-deriving flags each time.
---

# Build, flash, debug — neon_samurai

Firmware for an Atmel/Microchip ATxmega128A4U (AVR), built with CMake +
Ninja, cross-compiled with avr-gcc. LUFA (USB stack) is fetched at
configure time via `FetchContent` into `build/_deps`, shared across all
build-type directories.

## Build types and layout

Three build types, each in its own out-of-tree directory —
`build/<Type>/`, never a single shared `build/`:

| Type | Directory | Optimization | LTO | Debug info |
|---|---|---|---|---|
| `Debug` | `build/Debug` | `-Og` | off (breaks stepping) | full (`-g3 -ggdb -gdwarf-4`) |
| `Release` | `build/Release` | `-Os` | on | none |
| `RelWithDebInfo` | `build/RelWithDebInfo` | `-Os` | off | full (`-g -ggdb`) |

`RelWithDebInfo` is the practical default for day-to-day flashing — it's
what [scripts/flash.sh](scripts/flash.sh) defaults to when no build type
is given, and what most VSCode debug tasks build.

### Configure + build (CLI)

```sh
cmake -G Ninja -B build/<Type> -S . --toolchain=cmake/toolchain.cmake -DCMAKE_BUILD_TYPE=<Type>
cmake --build build/<Type>
```

`<Type>` is one of `Debug`, `Release`, `RelWithDebInfo`. Always pass
`--toolchain=cmake/toolchain.cmake` — this is the AVR cross-compile
toolchain file; omitting it configures a host build that won't link
against AVR libc.

Outputs land in `build/<Type>/`: `neosam.elf`, `.hex`, `.bin`, `.eep`,
plus `output.map` (linker map, useful for size/symbol investigation —
see [docs/Binary Analysis.md](docs/Binary%20Analysis.md) for puncover/elf_diff
workflows on top of it).

### Via VSCode tasks

Prefer these when working inside VSCode — see
[.vscode/tasks.json](.vscode/tasks.json):
- **CMake Configure** — prompts for build type, configures it.
- **CMake Configure Debug** / **CMake Configure RelWithDebInfo** —
  non-interactive, fixed build type.
- **Build (clean)** — default build task (Cmd/Ctrl+Shift+B): configures
  (prompted type) + clean + build.
- **Build Debug** / **Build RelWithDebInfo** — configure+build a fixed type
  without the picker.

### Rebuilding after a dependency or CMakeLists change

`FETCHCONTENT_BASE_DIR` is pinned to `build/_deps` and shared across all
three build-type dirs — LUFA is only fetched once, not once per type. A
clean of one build type does not re-fetch LUFA; delete `build/_deps` too
if you actually need a fresh checkout.

## Flashing

**Only the application section is touched by normal flashing.** The
bootloader (boot section, `0x20000–0x21FFF`) is separate and almost never
needs attention — see the safety note below before ever touching it.

```sh
scripts/flash.sh [Debug|Release|RelWithDebInfo]   # defaults to RelWithDebInfo
```

This runs `avrdude -c jtag3pdi -p x128a4u -P usb -U flash:w:build/<Type>/neosam.hex:a`
— i.e. it assumes a JTAGICE3 (or compatible PDI programmer) is attached.
Equivalent VSCode tasks: **Flash Board** (prompts build type) and
**Flash Board (RelWithDebInfo)**.

If flashing over USB DFU instead of PDI (device already has a working
bootloader and is currently in DFU mode): `dfu-programmer` per the
commented example at the bottom of [scripts/flash.sh](scripts/flash.sh)
— `dfu-util` does **not** work with Atmel's DFU variant, don't suggest it.

### ⚠️ Bootloader safety constraint

Never flash `firmware/bootloader_xmega_twister.hex` via
[scripts/flash_bootloader.sh](scripts/flash_bootloader.sh) unless the
device is confirmed to have a blank/missing boot section (four-corner
gesture and the `bootloader` console command both just reset the device
instead of entering DFU mode). This is a one-time, PDI-only, no-recovery-if-
wrong operation — read [docs/manual/11-bootloader-recovery.md](docs/manual/11-bootloader-recovery.md)
before running it, and always confirm with the user first; it's the one
truly destructive operation in this project's tooling. Normal firmware
updates (`scripts/flash.sh`) never need it and don't touch the boot
section at all.

## Debugging (Bloom + GDB)

[Bloom](https://bloom.oscillate.io) is the hardware debug server bridging
GDB to the JTAGICE3/PDI probe.

- **Debug: Build (Debug) & Start Bloom** — builds `Debug` (no LTO, full
  symbols — the right type for real single-stepping), then launches
  Bloom (VSCode background task, watches for
  "Starting TargetController" → "Waiting for GDB RSP connection").
- **Debug: Build (RelWithDebInfo) & Start Bloom** — same, optimized binary
  with symbols; use when Debug's `-Og` changes timing-sensitive behavior
  you're trying to reproduce (LED PWM, USB timing, encoder quadrature) but
  you still want line info.
- **Stop Bloom** — `pkill -f bloom`.
- After Bloom reports it's waiting for GDB, attach via the VSCode C/C++
  debugger (see [.vscode/launch.json](.vscode/launch.json)) or `avr-gdb`
  directly against `build/<Type>/neosam.elf`.

### Serial console monitor

If built with `ENABLE_CONSOLE` (on by default per
[CMakeLists.txt](CMakeLists.txt)), the device exposes a USB-CDC serial
console (commands like `reset`, `bootloader`, `help`, temperature/RNG
diagnostics — see [src/console/console.c](src/console/console.c)):

```sh
scripts/monitor.sh   # minicom -D /dev/ttyACM0, may need sudo
```

## clangd / compile_commands.json

CMake symlinks `compile_commands.json` at the repo root to whichever
build type was configured most recently (see the `file(CREATE_LINK ...)`
in [CMakeLists.txt](CMakeLists.txt)) — clangd only looks in fixed
locations and has no notion of the three parallel build dirs. If clangd
diagnostics look stale after switching build types, re-run CMake Configure
for the type you're actively working in so the symlink repoints.
[.clangd](.clangd) also suppresses `-Wmain`/`main_returns_nonint` since
this is freestanding AVR firmware where `main()` legitimately returns
`void`/is `noreturn` — don't "fix" that if you see it flagged.
