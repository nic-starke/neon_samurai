---
id: getting-started
title: Getting started
summary: What NEON_SAMURAI is, what it needs, and what to expect the first time you plug the unit in.
order: 1
---

NEON_SAMURAI is alternative firmware for the DJ Tech Tools Midi Fighter
Twister. It keeps the shape of the original - sixteen encoders, sixteen
encoder switches, six side switches - and extends what each of them can be
made to do.

This manual is written for someone using the device, not building the
firmware. If you are after the source, the build instructions, or the
protocol, those live in the repository.

## What you need {: #what-you-need }

A Midi Fighter Twister, a USB cable that carries data rather than power
alone, and something to receive MIDI. Nothing else is required to use the
device once the firmware is on it.

To change settings you also need a browser that supports Web MIDI with
system exclusive access. In practice that means Chrome or Edge. Firefox does
not implement Web MIDI, and Safari's support is partial. If your browser
cannot do it, the editor tells you so rather than half-working.

## The first thing to know {: #warranty }

Installing this firmware will almost certainly void your warranty with DJ
Tech Tools, and the software is provided as-is with no warranty of any kind.
That is not a formality - read [Installing](02-installing.md) before you
flash anything, particularly the part about making sure you can recover the
device afterwards.

## Plugging in {: #plugging-in }

The unit is class-compliant. There is no driver to install: connect it and
it appears as a MIDI device called NEON_SAMURAI.

On power-up the encoders light in the colours you last set, and the top row
shows which bank is active. Everything you had configured is restored from
the device's own memory - settings are held on the unit, not in the editor,
so the device behaves the same way on a machine it has never been connected
to before.

## How settings are stored {: #settings-storage }

Changes are kept in the device's EEPROM, which survives being unplugged.
Writes are batched rather than immediate: the firmware saves at most once
every five seconds, and only the parts that actually changed.

That delay is deliberate. EEPROM wears out after a finite number of erase
cycles, and writing on every knob movement would spend that budget quickly.
The practical consequence is that if you change something and pull the USB
cable within a few seconds, that last change may not have been written. Give
it a moment before unplugging.
