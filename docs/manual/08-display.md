---
id: display
title: Display and colour
summary: What the ring of lights around each encoder is showing you, and how to change it.
order: 8
---

## Display styles {: #display-styles }

The indicator arc can draw an encoder's position in one of three ways.

**Dot** lights a single LED at the current position. Clearest for reading an
exact value at a glance.

**Bar** lights every LED from the start up to the current position, filling
as the value rises. Easiest to read across the whole grid at once.

**Blended bar** does the same but fades the leading LED part-way, so the bar
moves smoothly rather than jumping a whole LED at a time. There are only
eleven LEDs in the arc and 256 positions, so this is what makes small
movements visible at all.

On a detented encoder all three draw outwards from the centre instead of up
from the bottom.

## Choosing a colour {: #encoder-colour }

Each layer has its own colour, set as hue, saturation and brightness rather
than as red, green and blue amounts. Picking a colour by hue is easier to
reason about - you choose the colour first and then how strong and how
bright you want it.

The device converts that to what its red, green and blue LEDs need. If you
read the RGB values back they are a derived mirror of the hue setting rather
than a separate control, so the hue is the thing to change.

## Brightness and how it is produced {: #brightness }

Each LED has 256 brightness levels.

The device has no analogue dimming, so brightness is produced by switching
each LED on and off far faster than the eye can follow and varying the
proportion of time it spends on. A perceptual correction is applied on top,
because eyes do not respond to light linearly - without it, most of the
range would look the same and all the visible change would be crowded into
the bottom.

The practical result is that a setting halfway up the scale looks about
halfway bright.

## When the device is left alone {: #idle-display }

When nothing is connected that could be using the device - a charger, a dead
hub, a computer that has not enumerated it - and nothing has been touched for
ten seconds, the panel fades out to an idle animation.

## The detent lights {: #detent-lights }

The two detent LEDs light when a detented encoder is sitting exactly on its
centre position. On an encoder without detent mode they stay off.
