# Reference documents

Datasheets, application notes and specifications the firmware is written
against. Fetch them with:

```sh
tools/dfu/fetch-docs.sh
```

## Why these are not in the repository

They belong to Microchip and the USB-IF, and neither licenses them for
redistribution. Committing them into an MIT-licensed public repository would
be the same mistake as vendoring GPL code into it. The PDFs are gitignored;
this page and the fetch script are what is tracked.

Two of them Microchip now serves only through its own search - a direct
request returns 403 whatever the path - so the script names them by document
number instead of downloading them.

## What each one is for

| Document | Number | Why it matters |
| -------- | ------ | -------------- |
| XMEGA USB DFU Boot Loader | AVR1916 / 8429 | The bootloader actually sitting in this chip's boot section. Its memory layout and command set are what a flasher has to satisfy. |
| FLIP USB DFU Protocol | AVR4023 / 8457 | The protocol itself: how commands are framed over the control endpoint, and where Atmel departs from standard DFU. |
| USB Firmware Upgrade for AT90USB | AVR282 / 7769 | The variant the vendored flasher implements. Read next to AVR1916 to see exactly what the XMEGA port has to change. |
| USB DFU Specification 1.1 | — | The standard the Atmel variant is patterned on. The shared requests behave as described here; the rest do not. |
| XMEGA AU Manual | 8331 | Family reference for the peripherals. The NVM and flash chapters are the ones this work needs. |
| XMEGA A4U Datasheet | 8387 | Flash geometry for this exact part - page size, section boundaries, addressing. |

## The one number that matters most

The vendored flasher assumes 256-byte flash pages, which is right for the
AT90USB and ATmega parts it was written for. The ATxmega128A4U does not use
that page size, and its 128 KB of flash needs the 64 KB page-select command to
reach the upper half. Those two differences are the substance of the port -
confirm both against the datasheet rather than by experiment.
