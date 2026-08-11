"""
Direct ALSA rawmidi transport (/dev/snd/midiCxDy), bypassing the ALSA
sequencer entirely.

python-rtmidi's Linux backend (and therefore mido, which wraps it) goes
through the ALSA sequencer (snd-seq), which has a long-standing, widely
reported limitation: sysex events beyond a certain size are silently
dropped or truncated rather than delivered, because the sequencer's
internal event pool chunks sysex into fixed-size ALSA events and (at least
on the driver/library versions available in this environment) does not
reliably reassemble or flush longer messages. This was confirmed directly
against this project's own firmware: an identical byte-for-byte sysex
message delivered fine via `amidi` (which talks straight to the rawmidi
character device, no sequencer involved) but never arrived via
rtmidi.MidiOut.send_message() using the ALSA sequencer API - a message as
short as 13 bytes total was enough to trigger it.

`amidi` (alsa-utils) works because it opens /dev/snd/midiCxDy directly and
read()/write()s raw bytes with no sequencer layer in between. This module
does the same thing directly in Python, avoiding both the sequencer issue
and a subprocess dependency on the `amidi` binary.

Device discovery: enumerate /proc/asound/ or use `amidi -l` output to find
the card/device numbers for a given ALSA client name, since rawmidi devices
are identified by numeric card/device index, not by name directly.
"""

from __future__ import annotations

import glob
import os
import re
import select
import subprocess
import time

# How long to keep reading after the last byte arrives before deciding the
# device is done replying. The device's rawmidi write does not reliably
# surface the literal 0xF7 terminator byte to a read() (observed directly:
# an amidi -d capture of a real GET_RESPONSE consistently came back one
# byte short, ending at the last data byte) - so termination is inferred
# from a quiet period, not from seeing 0xF7.
QUIET_PERIOD_S = 0.15
POLL_INTERVAL_S = 0.05


def find_rawmidi_device(name_substring: str) -> str:
    """Return the /dev/snd/midiCxDy path for the first ALSA rawmidi device
    whose `amidi -l` name contains `name_substring`. Raises RuntimeError if
    none is found."""
    result = subprocess.run(
        ["amidi", "-l"], capture_output=True, text=True, check=True
    )
    for line in result.stdout.splitlines():
        # Lines look like: "IO  hw:4,0,0  SAMURAI MIDI 1"
        m = re.match(r"\S+\s+hw:(\d+),(\d+),(\d+)\s+(.+)$", line.strip())
        if m and name_substring.lower() in m.group(4).lower():
            card, device = m.group(1), m.group(2)
            path = f"/dev/snd/midiC{card}D{device}"
            if os.path.exists(path):
                return path
    available = glob.glob("/dev/snd/midiC*")
    raise RuntimeError(
        f"No rawmidi device matching '{name_substring}' found via `amidi -l`. "
        f"Devices present: {available!r}. Is the device connected and "
        f"enumerated?"
    )


class RawMidiPort:
    """A bidirectional ALSA rawmidi device, opened for raw byte I/O."""

    def __init__(self, path: str):
        self._path = path
        self._fd = os.open(path, os.O_RDWR)

    def close(self) -> None:
        os.close(self._fd)

    def drain(self) -> None:
        """Discard any bytes currently waiting to be read (e.g. a stale
        reply from a previous, already-abandoned request)."""
        while True:
            r, _, _ = select.select([self._fd], [], [], 0)
            if not r:
                return
            os.read(self._fd, 4096)

    def send(self, raw: bytes) -> None:
        os.write(self._fd, raw)

    def receive(self, timeout_s: float) -> bytes | None:
        """Read bytes until a quiet period (see QUIET_PERIOD_S) or
        `timeout_s` elapses with nothing received at all. Returns None on a
        full timeout with zero bytes read; otherwise returns the
        accumulated bytes, with a synthetic 0xF7 appended if the read
        didn't end with one (see module docstring - this device's rawmidi
        reads consistently omit the literal terminator byte)."""
        buf = bytearray()
        deadline = time.monotonic() + timeout_s
        last_byte_time = None

        while time.monotonic() < deadline:
            r, _, _ = select.select([self._fd], [], [], POLL_INTERVAL_S)
            if r:
                chunk = os.read(self._fd, 4096)
                buf.extend(chunk)
                last_byte_time = time.monotonic()
            elif last_byte_time is not None and (
                time.monotonic() - last_byte_time
            ) > QUIET_PERIOD_S:
                break

        if not buf:
            return None
        if buf[-1] != 0xF7:
            buf.append(0xF7)
        return bytes(buf)
