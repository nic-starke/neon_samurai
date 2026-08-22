---
id: the-device
title: Your device
summary: What each control and light on the unit is, and the words this manual uses for them.
order: 3
---

## The encoders {: #encoders }

Sixteen rotary encoders in a four-by-four grid. Each one turns without end -
there is no stop at either extreme - and each one also presses in, acting as
a switch.

Throughout this manual encoders are counted in reading order: the top-left
encoder first, along the top row, then the next row down, and so on to the
bottom-right.

An encoder holds a position rather than an angle. The position runs from 0
to 255, and it is that number, not the physical rotation, that gets turned
into MIDI.

## The lights {: #the-lights }

Each encoder has sixteen LEDs arranged around it, doing three different
jobs.

Eleven of them form the indicator arc - the ring of small white lights that
shows the encoder's position. Three more are a red, a green and a blue LED
that light the encoder body from beneath, which is what gives each encoder
its colour. The last two are the detent lights, which mark the centre
position.

The firmware drives all of them with 256 brightness levels and applies a
perceptual correction, so a level halfway up the scale looks halfway bright
rather than measuring halfway.

## The side switches {: #side-switches }

Six switches down the sides of the unit, three on the left and three on the
right. They are numbered 1 to 3 down the left-hand side and 4 to 6 down the
right, matching the markings on the case.

Unlike the encoder switches, these act on the whole device rather than on
one encoder. By default they are set up in mirrored pairs:

| Switch | Position     | Default action                      |
| ------ | ------------ | ----------------------------------- |
| 1      | Left, top    | Cycle the layer on every encoder    |
| 2      | Left, middle | Previous bank                       |
| 3      | Left, bottom | Nothing                             |
| 4      | Right, top   | Hold the other layer while pressed  |
| 5      | Right, middle| Next bank                           |
| 6      | Right, bottom| Nothing                             |

Switches 3 and 6 are deliberately left free.

## Banks {: #banks }

The device holds four complete sets of encoder settings, called banks. Only
one is active at a time, and switching between them changes what all
sixteen encoders do at once.

Four banks of sixteen encoders, each with two layers, is 128 separately
configurable controls. See [Banks](09-banks.md).

## Layers {: #layers-intro }

Each encoder carries two independent MIDI configurations, called layers -
layer A and layer B. They can be swapped between, or both driven at once
from the same knob.

Layers are the main thing that makes this firmware different from the
original, and they have [a page of their own](05-layers.md).
