# Bootloader Recovery

This page covers what to do if your device has no working DFU bootloader -
the four-corner gesture and the `bootloader` console command both reset the
device instead of entering DFU mode, and there is no other software path
back in.

## Background

The ATxmega128A4U reserves a small, protected region of flash (the "boot
section", `0x20000`-`0x21FFF`, 8 KiB) for a bootloader, separate from the
128 KiB application section that firmware updates normally write to. See
[Technical.md](Technical.md#bootloader) for how NEON_SAMURAI enters the
bootloader.

If the boot section is blank - because the chip was never programmed with a
bootloader, or it was erased at some point (e.g. by a full chip-erase flash
rather than the default page-erase) - `bootloader_check()` still runs and
still jumps into the boot section as designed, but there's nothing there to
execute. The device does not crash; it just sits idle until the next reset
puts it back into the application. This looks identical to the device
"getting stuck" or "just resetting" when you try the gesture or console
command.

You can confirm this is what's happening by checking whether the device
ever enumerates as a DFU device (`03eb:2fde`, "Atmel Corp. ATXMEGA128A4U DFU
Bootloader") when you trigger bootloader entry - if it never does, the boot
section is very likely empty.

## Recovery

Recovering from this requires a PDI programmer (an Atmel/Microchip
JTAGICE3, Atmel-ICE, or similar) connected to the PDI header inside the
device - there is no way to write the boot section over USB alone.

1. Connect the PDI programmer to the device and to your computer.
2. From the repository root, run:

   ```sh
   scripts/flash_bootloader.sh
   ```

   This flashes `firmware/bootloader_xmega_twister.hex` into the boot
   section via `avrdude`. It does not touch the application section - your
   existing NEON_SAMURAI firmware is left as-is.
3. Power-cycle the device.
4. Test recovery: try the four-corner gesture or the `bootloader` console
   command. The device should now enumerate as a DFU device and show the
   checkerboard LED pattern.

This is a **one-time operation per device**. Once the boot section is
programmed, normal firmware updates (`scripts/flash.sh`, or DFU tools like
`dfu-programmer`) never touch it again.

### Verifying the bootloader over DFU

With the device in DFU mode (after step 4 above), you can confirm the
bootloader responds correctly:

```sh
dfu-programmer atxmega128a4u get bootloader-version
```

This should print a bootloader version (e.g. `Bootloader Version: 0x04`)
rather than an error. From here you can flash any application firmware over
DFU, for example:

```sh
dfu-programmer atxmega128a4u erase
dfu-programmer atxmega128a4u flash build/RelWithDebInfo/neosam.hex
dfu-programmer atxmega128a4u launch
```

Note `dfu-util` does not work against this bootloader - Atmel's DFU
implementation uses a vendor-specific protocol variant that `dfu-util`
(generic USB-IF DFU class) does not recognise. Use `dfu-programmer`
instead.

## Provenance of the bootloader file

See [firmware/README.md](../firmware/README.md) for where
`bootloader_xmega_twister.hex` comes from and how it was verified.
