---
id: layers
title: Layers
summary: Two independent MIDI configurations on one knob, and the ways they can share it.
order: 5
---

Every encoder carries two layers, A and B. Each has its own MIDI settings,
its own value range, its own colour, and its own slice of the knob's
rotation. They are the reason one encoder can do the work of two.

## Toggle or overlay {: #layer-mode }

An encoder uses its layers in one of two ways.

In **toggle** mode one layer is active at a time and the other is dormant.
The knob's full sweep maps onto whichever layer is active, and a switch
swaps between them. This is the one to use when you want two separate
controls on one knob and only need one of them at a time.

In **overlay** mode both layers are live together. One turn of the knob
drives both, each according to its own range, so a single movement sends two
different MIDI messages. This is the one to use for linked parameters - a
filter cutoff and its resonance, a send level and its return.

## Start and stop positions {: #layer-span }

A layer does not have to use the whole knob. Each one has a start and a stop
position, and the layer is only active between them.

That makes some arrangements possible that a single control cannot manage.
Set layer A to the lower half and layer B to the upper half and you get two
controls end to end on one knob, with a handover in the middle. Give them
both the full sweep and they move together. Overlap them partially and they
share the middle while each keeps a region of its own.

## Value range {: #layer-range }

Separately from where a layer sits on the knob, it has a range of values it
sends: a minimum and a maximum.

Setting a minimum of 10 and a maximum of 20 means the layer only ever emits
values from 10 to 20, spread across whatever part of the knob it occupies. It
is a way of restricting a control to a useful region rather than having to
be careful with your hand.

The minimum and maximum can also be given the other way round - a minimum of
20 and a maximum of 10 - which inverts the control, so turning clockwise
counts down.

The range is fitted to whatever the chosen MIDI mode can carry, so switching
a layer from ordinary to high-resolution CC widens it to match rather than
leaving it stuck at the old scale.

## Colour {: #layer-colour }

Each layer has its own colour, and the encoder shows the colour of the layer
that is active.

When both layers are active together the two colours are blended in
proportion, so the encoder's colour tells you which layer is currently doing
most of the work.

## Why only two? {: #why-two-layers }

Earlier versions had four layers per encoder, but they were barely
configurable - fixed positions, fixed ranges.

Making a layer properly configurable costs memory, and the device has a
fixed and quite small amount of it. Two fully configurable layers turned out
to be worth considerably more than four rigid ones. With four banks that
still comes to 128 independent controls.
