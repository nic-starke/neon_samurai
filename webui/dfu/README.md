# WebUSB firmware update

Porting browser-based Atmel DFU flashing to the ATxmega128A4U, so the editor
can update firmware without a command-line tool.

```
webui/dfu/
  vendor/            tmk/AVRFlashOnWeb, unmodified. MIT, (c) 2025 Jun Wako.
  xmega-dfu.js       the XMEGA port - what the editor flashes with
  intel-hex.js       parses the .hex an image arrives as
  fake-device.js     a DFU device that models flash, for the tests
  harness.html       manual flashing harness, for trying things by hand
  probe.html         reports what a device in DFU actually exposes
```

Reference documents are not in the repository - see docs/reference/README.md
for what they are and how to get them.

## Why this is possible

A WebUSB probe against the device in DFU mode came back clean:

```
device             ATMEL / ATXMEGA128A4U DFU Bootloader  (03eb:2fde)
open()             ok
interface 0 alt 0  class 255 sub 0 proto 0, 0 endpoint(s)
claimInterface(0)  ok
DFU_GETSTATUS      00 00 00 00 02 00   -> bStatus 0, bState 2 (dfuIDLE)
```

Two details in that matter. The interface is **vendor-specific (class 255)**,
not the DFU class - which is why no generic driver binds to it, why `dfu-util`
cannot drive it, and, helpfully, why WebUSB can claim it uninterrupted. And it
has **no endpoints**: every transfer is a control transfer, so there is no bulk
plumbing to write.

## The protocol

From AVR4023. Every command is six bytes:

| Offset | Field | Size |
| ------ | ----- | ---- |
| 0 | Group identifier | 1 |
| 1 | Command identifier | 1 |
| 2 | Arguments | 4 |

| Group | Value | Purpose |
| ----- | ----- | ------- |
| `CMD_GROUP_DOWNLOAD` | 01h | Program a memory |
| `CMD_GROUP_UPLOAD` | 03h | Read or check a memory |
| `CMD_GROUP_EXEC` | 04h | Erase the chip, or start the application |
| `CMD_GROUP_SELECT` | 06h | Select a memory and the area within it |

Commands ride on `DFU_DNLOAD`/`DFU_UPLOAD`, with `DFU_GETSTATUS` polled
between them. An error puts the device in `dfuERROR`, where it stays until
`DFU_CLRSTATUS` - so a failed step must be cleared before the next attempt or
everything after it fails too.

The vendored code already implements this faithfully, including the 64 KB
`SELECT_MEMORY_PAGE` that 128 KB of flash needs.

## What the port has to change

The vendored flasher targets AT90USB and ATmega parts. Two differences are
known:

1. **Flash page size.** `AtmelDFU.js` rejects a download whose start address
   is not a multiple of 256. The A4U's flash page is not 256 bytes, so that
   alignment check and any page-sized chunking are wrong for this part.
   Confirm the figure against the datasheet rather than by experiment.

2. **Memory unit identifiers.** `SELECT_MEMORY_UNIT` takes a unit id. Whether
   XMEGA numbers them as the AT90USB parts do is unverified.

## What is already known to work

Because dfu-programmer drives this exact device correctly, its behaviour is
the reference for anything ambiguous:

- **A chip erase is mandatory before writing.** Production parts ship with
  security bits set, and AVR1916 is explicit that chip erase is the only
  command permitted until one has been performed. `erase` then `flash` is not
  a convention, it is the requirement.
- Flashing 0x4D00 bytes and validating takes a few seconds.

## Testing

Entering DFU no longer needs the encoder gesture - the firmware takes a
guarded sysex command:

```sh
tests/robot/.venv/bin/python3 -c "
import sys; sys.path.insert(0,'tests/robot/lib')
from NeonSamuraiLibrary import NeonSamuraiLibrary
l = NeonSamuraiLibrary(); l.connect(); l.enter_bootloader()"
```

Recovering a device is `dfu-programmer atxmega128a4u erase`, then `flash`,
then `launch`. A failed write leaves the bootloader untouched, so the worst
case is trying again.

Linux needs `scripts/99-neon-samurai.rules` installed, or the browser cannot
reach the device. Windows is expected to need WinUSB/Zadig, and that is the
outstanding risk for this whole approach - it is not solvable from here.
