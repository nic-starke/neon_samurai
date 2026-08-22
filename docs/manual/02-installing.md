---
id: installing
title: Installing
summary: Putting the firmware on the device, and making sure you can get back if something goes wrong.
order: 2
---

## Before you flash {: #before-you-flash }

Make sure your device has a working recovery path first. This matters more
than anything else on this page.

NEON_SAMURAI can put the unit into its bootloader - the small program that
accepts new firmware over USB - either by a gesture on the encoders or by a
console command. Both of those depend on a working bootloader already being
present in the chip's boot section. If that section is empty, neither path
works, and the device can only be recovered with a hardware programmer
connected to its PDI pins.

A unit running factory DJ Tech Tools firmware has a bootloader. A unit that
has been flashed with a full-chip erase may not. If you are not certain
which you have, find out before you overwrite anything.

## Entering the bootloader {: #bootloader-gesture }

Hold the two left-hand encoders on the top row, and the encoder directly
beneath the leftmost one, while connecting the USB cable. Keep them held
until the device appears as a DFU device rather than as a MIDI device.

The three encoders form a small triangle in the top-left corner, chosen so
the gesture can be done with one hand while the other holds the cable.

If the device is already running and connected, the editor and the debug
console can both request the bootloader without the gesture.

## Flashing {: #flashing }

Once the unit is in its bootloader it accepts a firmware image over USB DFU.
Released builds are published as `.hex` files.

Note that Atmel's DFU implementation is its own variant - `dfu-util` does
not speak it. Use `dfu-programmer`, or the flashing script in the
repository.

Flashing writes only the application section. The bootloader itself is a
separate, one-time operation per device and is not part of a normal
firmware update.

## If the device stops responding {: #recovery }

A unit that shows no lights at all and does not enumerate is usually one
with an empty boot section rather than a broken one. The chip is fine; there
is simply nothing to run. Recovering it needs a PDI programmer.

A unit that lights up but behaves oddly is a different problem, and
[Troubleshooting](10-troubleshooting.md) is the place to start.

## Returning to factory settings {: #factory-reset }

Hold the two right-hand encoders on the bottom row, and the encoder directly
above the rightmost one, while connecting the USB cable. This clears every
stored setting and returns the device to its defaults.

This is the mirror image of the bootloader gesture, in the opposite corner,
so the two are hard to confuse and hard to trigger by accident.

It does not touch the firmware - only your settings.
