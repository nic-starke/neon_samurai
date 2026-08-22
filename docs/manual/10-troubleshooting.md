---
id: troubleshooting
title: Troubleshooting
summary: What to check when the device is not doing what you expect.
order: 10
---

## The device does not appear at all {: #not-appearing }

Check the cable first. A surprising number of USB cables carry power but not
data, and a device on one of those lights up perfectly while being invisible
to everything else.

If the unit shows no lights whatsoever and does not enumerate, see
[If the device stops responding](02-installing.md#recovery). That is usually
an empty boot section rather than a fault.

## Every light is red {: #all-red }

Every LED lit solid red means the firmware stopped on purpose during
start-up, because something it cannot work without failed to initialise.

It is not a warning; the device has halted and will not do anything else
until it is power-cycled. If it happens repeatedly, the firmware image is
suspect - reflash it.

## The browser cannot see the device {: #browser-cannot-connect }

The editor needs Web MIDI with system exclusive access, which in practice
means Chrome or Edge. Firefox does not implement Web MIDI at all.

The page must also be served over HTTP rather than opened as a file from
disk, because some browsers withhold sysex permission from local files.

If another application has the device open exclusively, close it and try
again.

## A control sends nothing {: #control-sends-nothing }

Check that the layer is not set to disabled, and that it has a value range
wider than a single value.

Then check its start and stop positions. A layer only sends while the knob
is inside its own span, so a layer confined to the top half is silent for
the whole lower half of the rotation - which is correct behaviour, and looks
exactly like a broken control if you have forgotten it is set that way.

## A control sends two different things {: #two-messages }

That is overlay mode working as intended - both layers are live and each is
sending its own message. If you only want one at a time, set the encoder to
toggle instead. See [Toggle or overlay](05-layers.md#layer-mode).

If the second message is on a controller number 32 higher than the first,
that is not a second layer but high-resolution CC sending its fine half. See
[High-resolution CC](06-midi.md#high-resolution).

## Settings did not survive being unplugged {: #settings-lost }

The device saves at most once every five seconds. Changing something and
pulling the cable immediately can lose that last change.

This is a deliberate trade to protect the stored-settings memory, which
wears out with use. See
[How settings are stored](01-getting-started.md#settings-storage).

## The values jump when I first touch a knob {: #value-jumps }

The device sends the position it holds, which need not match what your
software currently has. Relative CC avoids this: it sends movements rather
than positions, so the software carries on from wherever it was. See
[Modes](06-midi.md#midi-mode).

## Fine control is impossible {: #too-coarse }

The encoders accelerate by default, so a fast turn moves in large jumps.
Turn more slowly for single steps, or assign a switch to fine adjust. See
[Fine adjust](04-encoders.md#fine-adjust).

## Checking for internal faults {: #fault-counters }

The firmware counts faults it can carry on through - work it had to drop
rather than stop for - under four headings: events lost, redraws lost, MIDI
transmissions lost, and settings writes lost.

On a healthy device all four are zero. A number that climbs during normal
use is worth reporting, along with what you were doing at the time. Builds
with the debug console enabled report them with the `diag` command.
