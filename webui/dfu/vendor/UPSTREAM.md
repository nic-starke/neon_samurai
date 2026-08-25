# Vendored: tmk/AVRFlashOnWeb

Unmodified copy of <https://github.com/tmk/AVRFlashOnWeb>, MIT licensed,
copyright (c) 2025 Jun Wako. `LICENSE` is the upstream file and must stay
with these sources.

    commit  ca7700bb143e8c88ff0c4ed2fef91c1332e2e214
    taken   2026-08-23

## Why it is here

It is the only working implementation of Atmel's DFU protocol in a browser.
It supports the AT90USB and ATmega parts, not the ATxmega128A4U this project
uses, so it is a starting point rather than a drop-in - the XMEGA variant
differs in how flash is addressed and paged.

## Do not edit these files

Keeping the copy pristine is what makes it possible to diff against upstream
and see exactly what the XMEGA port changed. The port lives one directory up,
in `webui/dfu/`, and imports from here.
