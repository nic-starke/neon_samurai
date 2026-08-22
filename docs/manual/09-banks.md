---
id: banks
title: Banks
summary: Four complete sets of encoder settings, and how to move between them.
order: 9
---

## What a bank holds {: #what-a-bank-holds }

A bank is a complete set of settings for all sixteen encoders - both layers
of each, with their MIDI configuration, ranges, colours, switch behaviour
and display style.

There are four of them, and only one is active at a time. Switching bank
changes what all sixteen encoders do at once, which makes banks the right
place to separate whole contexts: one bank per deck, one per instrument, one
per section of a set.

Four banks of sixteen encoders with two layers each comes to 128
independently configurable controls.

## Switching bank {: #switching-bank }

By default, side switch 2 steps back a bank and side switch 5 steps forward.
Both wrap around, so going forward from the last bank returns to the first.

The editor can also switch bank directly.

## Knowing which bank you are on {: #bank-indicator }

The top row of encoders shows the active bank, counting from the right: the
rightmost encoder of the top row is bank 1, and the leftmost is bank 4.

When you change bank the indicating encoder plays a short animation - it
fades out, flashes white, fades out again, and settles into its own colour.
The white flash is there so the change is visible even when the encoder's
own colour is dark or close to the colour of its neighbours.

## Why four {: #why-four-banks }

The number of banks is limited by the device's stored-settings memory, which
holds two kilobytes in total.

Each additional bank is another sixteen encoders' worth of configuration to
store. Four is what fits alongside everything else with a little room to
spare. A fifth would not fit without giving something else up.
