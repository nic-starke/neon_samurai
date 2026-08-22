---
id: midi
title: MIDI
summary: What each layer sends, on which channel, and how to get more than 128 steps out of a control.
order: 6
---

Every layer has its own MIDI settings. Two layers on the same encoder can
sit on different channels, send different kinds of message, and cover
different value ranges.

## Channel {: #midi-channel }

Each layer sends on one of the sixteen MIDI channels, chosen per layer
rather than per device.

## Modes {: #midi-mode }

A layer sends one of the following.

**Control change** is ordinary MIDI CC: a controller number and a value from
0 to 127. This is what most software expects and is the sensible default.

**High-resolution control change** sends the same parameter across two
controller numbers, giving 16,384 steps instead of 128. See
[below](#high-resolution).

**Relative control change** sends the direction and size of the movement
rather than the position. Useful where the software keeps its own value and
you want the knob to nudge it from wherever it happens to be, without the
value jumping when you first touch it.

**Note** sends a note on and note off rather than a continuous value.

**Disabled** switches the layer off entirely. A layer set to disabled sends
nothing and takes no part in the encoder's colour.

## High-resolution CC {: #high-resolution }

Ordinary MIDI CC carries 128 steps, which is coarse for something like a
filter cutoff - the steps are audible.

High-resolution CC splits the value across two controller messages: a coarse
one on the controller number you chose, and a fine one on that number plus
32. Together they carry 16,384 steps. This is the standard arrangement, and
software that understands it will pair the two automatically.

The controller number therefore needs to be below 32 for the pair to fit
inside the usable range. The device widens the layer's value range to match
when you select this mode, so you do not have to rescale anything by hand.

If your software does not understand the pairing it will see two unrelated
controls, one of which moves in very small increments. Use ordinary CC
instead.

## Ranges {: #midi-range }

The minimum and maximum values a layer sends are part of the layer, not of
the mode - see [Value range](05-layers.md#layer-range). Changing the mode
rescales them rather than discarding them.
