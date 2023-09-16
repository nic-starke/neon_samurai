# User Manual

## Installation

A complete installation guide is provided [here](wiki/InstallationGuide.md).

## Encoders

Each encoder has the following features:

- 2 rotary midi controls.
- 1 switch control.

The rotary midi controls are referred to as "Virtual Layers". These are seperately configurable, and can be enabled/disabled independently, a full description of the virtual layers is [given below](#virtual-layers).

Each encoder is also a switch (press the encoder down to active the switch). The switch can be configured to transmit midi messages, or to control the rotaries (such as resetting the value, switching layers, etc).

### Virtual Layers

Each encoder has 2 virtual layers - Layer A and Layer B. Each layer has a unique midi configuration, and can be enabled/disabled independently.

It is possible to have both layers active, and transmitting midi at the same time, allowing two midi parameters to be controlled by a single encoder.

The mapping of the layer to the encoder is also configurable, you can choose a start and stop position as required. This means the layers can either overlap, or occupy seperate spaces within the encoder rotation - Layer A could be on the left side, Layer B could be on the right, or any other combination you can think of!

The following options can be configured per layer:

- Start Position
  - The position on the encoder at which the layer will be active and begin to send midi messages.
- Stop Position
  - The position on the encoder at which the layer stops being active and stops sending midi messages.
- Minimum Value/Maximum Value
  - The user can customise the value range of midi messages, for example you could set the minimum value at 10, and the maximum at 20. The midi messages will only be of value 10,11,12,13... upto 20.
  - 7-bit and 14-bit (NRPN) ranges are possible.
  - The minimum and maximum can be reversed, so values will then be sent in reverse (20, 19, 18... ).
- [Midi Configuration - See Below](#layer-midi-configuration)
- RGB Colour
  - When the layer is active, the RGB LEDs will switch to this colour. If two layers are active the colours are blended proportionally.

> **Why only 2 layers?**
>
> Initially each encoder had 4 layers, but they were not very configurable (fixed positions and ranges). Additional memory is required to have all the customisation options, therefore the number of layers had to be limited to 2 to not go over the memory limits.

#### **Layer Midi Configuration**

The following Midi options can be configured **per layer**:

- Mode
  - CC
  - Relative CC
  - Note
- Channel (1 to 16)
- Value - the value to send based on the selected mode (the CC number, note value, etc..)

### Encoder Configuration

The following options can be configured **per encoder**:

- Display Style
  - The encoder display can be set to one of three styles, [see below](#display-styles)
- Detent LED Colour
  - The colour of the detent LEDs can be modified
- Detent Mode
  - The detent mode means the encoder now behaves like a pan pot on a mixer.
  - The virtual layers are still active and configurable.
  - Can also be controlled via the encoder switch, and/or side switches.
- Layer Configuration
  - Enable/Disable individual layers permanently.
  - Can also be controlled via the encoder switch, and/or side switches.
- Fine Adjust/Accelerated Rotation
  - Toggle between direct encoder rotation or accelerated rotation.
  - Can also be controlled via the encoder switch, and/or side switches.

### Display Styles

The encoder indicator display styles match the original MFT firmware:

1. Dot
2. Bar
3. Blended Bar

### Notes on Encoder Hardware

The encoders on the MFT have a limited resolution. Encoder resolution is determined by the number of output pulses there are per revolution. The EC11 encoders used in the MFT have a maximum PPR of 18. Each pulse is actually 4 logic level shifts (there are two output channels on the encoder, they both shift high then low - this is known as quadrature encoding), which gives a maximum of 18*4 = 72 steps per full revolution.

> 72 steps is just over 6-bits of resolution, less than the 7-bit resolution of standard midi messages!

To provide 7-bits or even 14-bits of resolution requires the firmware to increment by larger values. If this is done in a smart way (i.e by measuring the speed of encoder rotation) it is possible to fake a higher resolution. Therefore slow movements result in small increments, and fast movements result in larg increments - based on the rotational velocity.
