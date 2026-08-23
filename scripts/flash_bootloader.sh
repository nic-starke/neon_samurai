#!/usr/bin/env bash
# Flashes the Atmel DFU bootloader into the ATxmega128A4U's boot section via
# PDI (JTAGICE3). This is a ONE-TIME operation per device - normal firmware
# updates (scripts/flash.sh) only ever touch the application section and
# never need this script.
#
# Only run this if the device has no working bootloader: the four-corner
# gesture or the `bootloader` console command resets the device instead of
# entering DFU mode. See docs/manual/11-bootloader-recovery.md for background.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BOOTLOADER_HEX="${SCRIPT_DIR}/../firmware/bootloader_xmega_twister.hex"

if [[ ! -f "${BOOTLOADER_HEX}" ]]; then
	echo "error: bootloader hex not found at ${BOOTLOADER_HEX}" >&2
	exit 1
fi

echo "This will flash the DFU bootloader into the boot section (0x20000-0x21FFF)."
echo "The application section is not touched by this operation."
echo "File: ${BOOTLOADER_HEX}"
read -r -p "Continue? [y/N] " confirm
if [[ "${confirm}" != "y" && "${confirm}" != "Y" ]]; then
	echo "Aborted."
	exit 1
fi

# Note: avrdude's "boot" memory alias expects addresses relative to the boot
# section (0x0000-0x1FFF), but this hex file uses absolute addresses
# (0x20000+), matching the chip's full flat flash address space. Target
# "flash" instead - avrdude only writes the pages the hex file actually
# contains data for, so the application section is left untouched.
avrdude -v -c jtag3pdi -p x128a4u -P usb -U flash:w:"${BOOTLOADER_HEX}":i
