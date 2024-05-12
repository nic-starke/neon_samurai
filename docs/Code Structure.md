# Code Structure

The NEOSAM source codebase structure aims to be modular, and follows the principle of clear "separation of concerns".

Each module should be self-contained and have a minimal amount of coupling with other modules.

Modules tend to be localised into a single directory, for example all code that handles events is placed in the 'src/event' directory. Other modules that need to post/subscribe to events include the event.h header (which contains a complete interface for interacting with the event system).

## Interfaces and Backends

Another aim of the codebase structure is to allow for various "backends" - modules that implement some functionality that can be swapped in/out as required.

You may find that only a single backend is provided for each of these modules, that is because these were the best options at the time. Note that these backends are available via a single interface - a layer that sits between the application code and the lower level system functionality.

The LUFA USB library is an example of a backend - it provides the core USB functions, but in the future perhaps tinyUSB could replace it.

## Board(s)

This firmware is very specifically designed for the Midi Fighter Twister hardware, but maybe it will be ported to another device in the future. The specifics of the underlying device are all contained in a "board" directory.

This structure should hopefully allow future boards to be added fairly easily (with an appropriate HAL layer).

## HAL - Hardware Abstraction Layer

Every platform/MCU has its own drivers/headers/libraries which are generally provided by the vendor. Each of these is contained under the "hal" directory, in the case of the AVR XMega used in the MFT it is located under "/hal/avr/xmega/128a4u".

## Include Files

All header files are located in the "includes" directory - this separates the implementation from the interfaces.

## Code Documentation

Code documentation is provided in the source and header files themselves. Additional documentation is provided in the "docs" directory.
