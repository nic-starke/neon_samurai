---
id: encoders
title: Encoders
summary: How turning a knob becomes a value, and the settings that change the feel of it.
order: 4
---

## Position {: #position }

An encoder holds a position from 0 to 255. Turning it clockwise counts up,
anticlockwise counts down, and it stops at each end rather than wrapping
around.

That range is internal. What gets sent as MIDI depends on the layer
settings, which can map the full sweep onto any range you like - see
[MIDI](06-midi.md).

## Acceleration {: #acceleration }

The encoders fitted to the Twister produce 72 steps per full revolution.
That is slightly over six bits of resolution, which is less than the 128
values ordinary MIDI carries, let alone the 16,384 of the high-resolution
form.

The firmware makes up the difference by watching how fast you are turning.
A slow turn moves the position by one step at a time, so you can settle on
an exact value; a fast turn moves it in larger jumps, so you can cross the
whole range in a reasonable sweep. The result is that the full range is
reachable without giving up fine control.

This is on by default and is what an encoder does unless told otherwise.

## Fine adjust {: #fine-adjust }

Fine adjust turns acceleration off, so every physical step moves the
position by exactly one regardless of how fast you turn.

It is worth having on a control where you want to land on an exact value and
do not mind taking several revolutions to cross the range. An encoder switch
can be set to toggle it, or to apply it only while held - see
[Switches](07-switches.md).

## Detent {: #detent }

Detent mode makes an encoder behave like a pan control: it has a marked
centre, and the two detent LEDs light when the position is sitting exactly
on it.

The centre is the midpoint of the range, at position 127. The indicator arc
draws outwards from the centre rather than up from the bottom, so you can
see at a glance how far either side of centre you are.

Layers still work normally on a detented encoder.

## Resetting a value {: #reset-value }

An encoder switch can be set to send the encoder straight back to zero,
either the moment it is pressed or when it is released. Which of the two you
want depends on whether you would rather the jump happen under your finger
or after it leaves.

Reset applies to the layer that is currently active, not to both at once. On
an encoder driving two layers together, only the active one moves.

See [Switches](07-switches.md) for how to set that up.
