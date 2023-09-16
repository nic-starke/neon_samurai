# Muffin Features

This document will be updated during the development of the Muffin firmware.

## Feature Differences

The following is a list of the feature differences between Muffin and the original MFT firmware:

### Midi Channels/Modes (Complete)

The primary/secondary and super knobs can be on any midi channel, and use any midi mode (note, cc, etc..)

### Dual Super Knobs (Complete)

Every encoder has a primary mode and secondary mode. And EACH has a separate super knob (PrimarySuper and SecondarySuper)

Effectively it is possible to control 4 midi parameters on a single knob.

### 14-bit Midi (WIP)

WIP - still investigating this.
The encoders are processed as 16-bit values, so some important work is already done in this area.

### Standalone Configuration (WIP)

You can configure the encoders without the PC utility.

A button combination will put Muffin into a "Configuration Mode", allowing you to edit Encoder parameters such as channel, value, mode, detent, colour etc..

### Traktor Sequencer Removed

I have not used this feature, users that require this feature can use the original MFT firmware.

### 3 Virtual Banks (instead of 4)

For memory reasons the 4th bank has been removed.
I could remove the super-knobs and increase the bank size to 5 or 6.
If I get requests for it I may consider virtualising all the knobs and allow the user to choose if they want primary/secondary/superPrimary/superSecondary per encoder.

## Functional Differences

The following is a list of the functional differences between Muffin and the original MFT firmware:

### Input Processing

Muffin has been developed from the ground-up, its implementation to handle inputs (encoders and switches) differs greatly from the original firmware.

I have not quantified the differences, but they are likely to be subtle to the end-user.

### Output Processing

Processing of the LEDs has also been developed from the ground-up. Muffin uses an HSV colour space and then converts this to RGB where required.
This means that interpolation between two colours does not go through a gray phase.

Overall this means the user can select any colour they want (16 million...), however at low brightness the bit-reduction lowers the possible colours down to the number of display frames in the display buffer (32).
Most users will probably not notice.

### USB

The USB functionality is provided by the same open-source driver from the original firmware, no differences there.
