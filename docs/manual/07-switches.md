---
id: switches
title: Switches
summary: What pressing an encoder can be made to do, and what the six side switches control.
order: 7
---

## Encoder switches {: #encoder-switch-modes }

Every encoder presses in, and that press can be assigned one of the
following.

**Nothing** leaves the switch inactive.

**Cycle layer** swaps between layer A and layer B each time it is pressed.
The change sticks until you press again.

**Hold layer** switches to the other layer for as long as the switch is
held, and returns to the first one when you let go. Good for momentary
access to a second parameter without losing your place on the first.

**Reset on press** sends the active layer's position to zero the instant the
switch goes down.

**Reset on release** does the same, but waits until you let go. Which one
suits depends on whether you want the jump to happen while your finger is
still there.

**Toggle fine adjust** turns [fine adjust](04-encoders.md#fine-adjust) on and
off. The change sticks.

**Hold fine adjust** applies fine adjust only while the switch is held,
which lets you drop into precise control for a moment without changing the
encoder's normal behaviour.

**MIDI** makes the switch send its own MIDI message, independently of what
the encoder is doing.

## Side switches {: #side-switch-modes }

The six switches down the sides act on the whole device rather than on a
single encoder. Each can be set to one of the following.

**Nothing** leaves it inactive.

**Cycle layer on every encoder** swaps all sixteen encoders between their
layers at once.

**Hold layer on every encoder** does the same for as long as it is held,
restoring the previous state on release. Because it remembers what each
encoder was on rather than assuming they all matched, encoders that were
already on layer B go back to layer B.

**Previous bank** and **next bank** step through the four banks. They wrap
around, so going past either end brings you back to the other.

## The default arrangement {: #side-switch-defaults }

Out of the box the side switches are set up as mirrored pairs, so the two
sides do related jobs at the same height:

| Switch | Position      | Action                             |
| ------ | ------------- | ---------------------------------- |
| 1      | Left, top     | Cycle the layer on every encoder   |
| 2      | Left, middle  | Previous bank                      |
| 3      | Left, bottom  | Nothing                            |
| 4      | Right, top    | Hold the other layer while pressed |
| 5      | Right, middle | Next bank                          |
| 6      | Right, bottom | Nothing                            |

Switches 3 and 6 are left free deliberately, so there is somewhere to put
something of your own without giving up a default.
