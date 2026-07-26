# Firmware Assets

## bootloader_xmega_twister.hex

The Atmel DFU bootloader for the ATxmega128A4U, as used by the original DJ
TechTools Midifighter Twister firmware. This is **not** built by this
project - it is a separate binary, entirely independent of the neon_samurai
application code, that occupies the MCU's protected boot section
(`0x20000`-`0x21FFF`, 8 KiB) rather than the application section.

- Source: <https://github.com/nic-starke/Midi_Fighter_Twister_Open_Source/blob/master/Release/bootloader_xmega_twister.hex>
- SHA-256: `adb54909fea5e000119a6de1c4c1af2bbed8d53d651bed816d13cc3f0081e90d`
- Verified: flashed to a real ATxmega128A4U via PDI and confirmed working -
  `dfu-programmer` can read the bootloader version, erase, flash, and
  validate the application section over USB DFU.

### Why this is needed

`bootloader_check()` (see `src/hal/boot.c`) jumps into this boot section on
a watchdog-triggered reset with the correct magic key set. If the boot
section is blank - which it will be on a chip that was never flashed with
this bootloader, or had it erased - that jump lands in unprogrammed flash
and does nothing useful. See [issue #29](https://github.com/nic-starke/neon_samurai/issues/29)
and the "Bootloader Recovery" wiki page for the full story.

This file only needs to be (re-)flashed once per device, via
`scripts/flash_bootloader.sh` - normal application firmware updates never
touch the boot section.
