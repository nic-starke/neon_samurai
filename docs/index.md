---
toc: false
---

# NEON_SAMURAI documentation

Before using this software you must read and accept the licence.

## For people using the device

The [user manual](manual/) covers everything the device does, from plugging
it in to configuring layers and MIDI. It is published as a site next to the
browser editor, and the same text appears as contextual help inside the
editor - both come from `docs/manual/`, so there is only one copy to keep
right.

Build it locally with:

```sh
pip install markdown pyyaml
python3 tools/docs/build_manual.py
```

| Page                                             | Covers                                        |
| ------------------------------------------------ | --------------------------------------------- |
| [Getting started](manual/01-getting-started.md)  | What it is, what you need, how settings persist |
| [Installing](manual/02-installing.md)            | Flashing, the bootloader, factory reset       |
| [Your device](manual/03-the-device.md)           | Encoders, lights, side switches, banks        |
| [Encoders](manual/04-encoders.md)                | Position, acceleration, fine adjust, detent   |
| [Layers](manual/05-layers.md)                    | Two configurations per knob                   |
| [MIDI](manual/06-midi.md)                        | Modes, channels, high-resolution CC           |
| [Switches](manual/07-switches.md)                | Encoder and side switch behaviour             |
| [Display and colour](manual/08-display.md)       | Display styles, colour, brightness            |
| [Banks](manual/09-banks.md)                      | Four banks and moving between them            |
| [Troubleshooting](manual/10-troubleshooting.md)  | When it does not do what you expect           |

## For people working on the firmware

- [Code structure](Code%20Structure.md)
- [Technical](../wiki/Technical.md) - bootloader and memory layout
- [Bootloader recovery](../wiki/BootloaderRecovery.md) - recovering a unit with a blank boot section
- [XMega128A4U specs](../wiki/XMega128A4U_Specs.md)
- [Assembly](Assembly.md)
- [Binary analysis](Binary%20Analysis.md)
