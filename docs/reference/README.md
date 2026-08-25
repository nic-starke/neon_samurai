# Reference documents

Datasheets, application notes and specifications the firmware is written
against. Download them into this directory by hand - the table below gives the
document number for each, which is what Microchip's search takes.

## Why these are not in the repository

They belong to Microchip and the USB-IF, and neither licenses them for
redistribution. Committing them into an MIT-licensed public repository would
be the same mistake as vendoring GPL code into it. The PDFs are gitignored;
this page is what is tracked.

Microchip returns 403 to a direct request for most of these whatever the path
and whatever the user agent, so they have to come through its search.

## What each one is for

| Document | Number | Why it matters |
| -------- | ------ | -------------- |
| XMEGA USB DFU Boot Loader | AVR1916 / 8429 | The bootloader actually sitting in this chip's boot section. Its memory layout and command set are what a flasher has to satisfy. |
| FLIP USB DFU Protocol | AVR4023 / 8457 | The protocol itself: how commands are framed over the control endpoint, and where Atmel departs from standard DFU. |
| USB Firmware Upgrade for AT90USB | AVR282 / 7769 | The variant the vendored flasher implements. Read next to AVR1916 to see exactly what the XMEGA port has to change. |
| USB DFU Specification 1.1 | — | The standard the Atmel variant is patterned on. The shared requests behave as described here; the rest do not. |
| XMEGA AU Manual | 8331 | Family reference for the peripherals. The NVM and flash chapters are the ones this work needs. |
| XMEGA A4U Datasheet | 8387 | Flash geometry for this exact part - page size, section boundaries, addressing. |

## What the port actually had to change

Two things differ from the AT90USB and ATmega parts the vendored flasher was
written for, and both are worth confirming in the documents rather than by
experiment:

- **The command packet is padded to 64 bytes**, not 32. `bMaxPacketSize0` on
  this device is 64, and the write command's data has to begin after a full
  packet of header.
- **128 KB of flash needs the 64 KB page-select command** to reach the upper
  half, because the command's address fields are only 16 bits wide.

The flash page size is *not* one of the differences. This page previously said
the ATxmega128A4U did not use the vendored 256-byte page, and that was wrong:
table 7-2 of the datasheet gives it a 128-word page, which is 256 bytes, and
128 KB in 512 application pages agrees. Acting on the claim set the flasher's
page size to 512, which made its alignment check stricter than the hardware
and would have refused to write an image that did not begin at zero.
