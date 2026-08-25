#!/usr/bin/env bash
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Fetch the reference documents the DFU work is written against.
# Copyright 2024 - Nicolaus Starke
# SPDX-License-Identifier: MIT
#
# https://github.com/nic-starke/neon_samurai
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
#
# The documents are downloaded rather than committed. They are Microchip's and
# the USB-IF's, not ours, and neither licenses them for redistribution - the
# same reason the GPL quadrature decoder had to go. docs/reference/ is
# gitignored; this script puts the files there.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
out="$repo_root/docs/reference"
mkdir -p "$out"

# Fetchable: name|url
docs=(
"AVR1916-XMEGA-USB-DFU-Bootloader.pdf|https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ApplicationNotes/ApplicationNotes/doc8429.pdf"
"AVR4023-FLIP-USB-DFU-Protocol.pdf|https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ApplicationNotes/ApplicationNotes/doc8457.pdf"
"AVR282-USB-Firmware-Upgrade-AT90USB.pdf|https://ww1.microchip.com/downloads/en/Appnotes/doc7769.pdf"
"USB-DFU-1.1-Specification.pdf|https://usb.org/sites/default/files/DFU_1.1.pdf"
)

# Microchip serves these only through its own search now - a direct request
# gets a 403 whatever the path. Named here with their document numbers so they
# can be found and dropped in by hand.
manual=(
"XMEGA-AU-Manual.pdf|Microchip document 8331 - \"XMEGA AU Manual\""
"ATxmega128A4U-Datasheet.pdf|Microchip document 8387 - \"XMEGA A4U Datasheet\""
)

failed=0
for entry in "${docs[@]}"; do
  IFS='|' read -r name url _why <<<"$entry"

  if [ -s "$out/$name" ]; then
    printf '  %-46s already present\n' "$name"
    continue
  fi

  printf '  %-46s ' "$name"
  if curl -sSLf --max-time 120 -o "$out/$name.part" "$url" 2>/dev/null; then
    mv "$out/$name.part" "$out/$name"
    printf 'ok (%s)\n' "$(du -h "$out/$name" | cut -f1)"
  else
    rm -f "$out/$name.part"
    printf 'FAILED\n'
    failed=$((failed + 1))
  fi
done

echo
for entry in "${manual[@]}"; do
  IFS='|' read -r name what <<<"$entry"

  if [ -s "$out/$name" ]; then
    printf '  %-46s already present\n' "$name"
  else
    printf '  %-46s fetch by hand: %s\n' "$name" "$what"
  fi
done

echo
echo "Documents are in docs/reference/ (gitignored)."

if [ "$failed" -gt 0 ]; then
  echo "$failed download(s) failed - see docs/reference/README.md." >&2
  exit 1
fi
