"""
Robot Framework library for hardware-in-the-loop testing of the
neon_samurai firmware's sysex configuration protocol over MIDI.

Requires a real device connected and enumerated as a MIDI port (default
name substring: "SAMURAI" - see the `device_port` argument). No mocking:
these tests talk to actual firmware on actual hardware, exactly like the
web GUI will.

Transport: direct ALSA rawmidi (/dev/snd/midiCxDy via rawmidi.py), NOT
python-rtmidi/mido. python-rtmidi's Linux backend goes through the ALSA
sequencer, which was found - by direct comparison against `amidi`, which
uses rawmidi - to silently drop sysex messages past a small size (13 bytes
total was enough to reproduce it here) rather than deliver them. See
rawmidi.py's module docstring for the full story. If this library is ever
ported to a platform without /dev/snd/midi* (e.g. running these tests from
outside Linux), the transport in rawmidi.py is the one piece that would
need a platform-specific replacement - everything else in this file is
platform-agnostic.
"""

from __future__ import annotations

import sysex as sx
from rawmidi import RawMidiPort, find_rawmidi_device
from robot.api.deco import keyword, library

DEFAULT_TIMEOUT_S = 2.0
DEFAULT_PORT_SUBSTRING = "SAMURAI"


@library(scope="GLOBAL")
class NeonSamuraiLibrary:
    def __init__(self):
        self._port: RawMidiPort | None = None

    # --- Connection lifecycle ---------------------------------------------

    @keyword("Connect To Device")
    def connect(self, port_substring: str = DEFAULT_PORT_SUBSTRING) -> None:
        """Open the ALSA rawmidi device whose `amidi -l` name contains
        `port_substring`. Fails the test if no matching device is found -
        this library never falls back to a mock, by design."""
        path = find_rawmidi_device(port_substring)
        self._port = RawMidiPort(path)

    @keyword("Disconnect From Device")
    def disconnect(self) -> None:
        if self._port is not None:
            self._port.close()
            self._port = None

    # --- Low-level sysex I/O -------------------------------------------

    @keyword("Send Sysex And Wait For Response")
    def send_and_wait(
        self,
        cmd: sx.Cmd,
        param: sx.Param,
        data: bytes = b"",
        timeout_s: float = DEFAULT_TIMEOUT_S,
    ) -> sx.SysexMessage:
        """Send a sysex request and block until a reply for the same
        `param` arrives (or `timeout_s` elapses). Any unrelated sysex
        traffic seen in between is discarded, not returned."""
        if self._port is None:
            raise AssertionError("Not connected - call 'Connect To Device' first")

        # Discard stale/unsolicited bytes sitting in the device buffer
        # before sending, so a leftover reply from a previous test can't be
        # mistaken for this request's response.
        self._port.drain()

        raw = bytes(sx.encode(cmd, param, data))
        self._port.send(raw)

        reply_bytes = self._port.receive(timeout_s)
        if reply_bytes is None:
            raise AssertionError(
                f"No sysex reply for {param.name} within {timeout_s}s "
                f"(cmd={cmd.name}, sent data={data.hex()})"
            )

        try:
            decoded = sx.decode(reply_bytes)
        except ValueError as e:
            raise AssertionError(
                f"Received bytes that don't decode as a valid sysex "
                f"reply: {reply_bytes.hex()} ({e})"
            ) from e

        if decoded.param != param:
            raise AssertionError(
                f"Expected a reply for {param.name} but got one for "
                f"{decoded.param.name}: {reply_bytes.hex()}"
            )
        return decoded

    # --- High-level GET/SET keywords, one per param family ------------
    # These build the correctly-shaped payload for the request and return
    # the *unpacked* response data bytes, so test suites work with plain
    # values rather than hand-building wire payloads every time.

    @keyword("Get Encoder Param")
    def get_encoder_param(
        self, param: sx.Param, bank: int, enc: int, timeout_s: float = DEFAULT_TIMEOUT_S
    ) -> bytes:
        req = sx.encoder_payload(bank, enc, 0)
        reply = self.send_and_wait(sx.Cmd.GET, param, req, timeout_s)
        return reply.data

    @keyword("Set Encoder Param")
    def set_encoder_param(
        self,
        param: sx.Param,
        bank: int,
        enc: int,
        value: int,
        timeout_s: float = DEFAULT_TIMEOUT_S,
    ) -> int:
        req = sx.encoder_payload(bank, enc, value)
        reply = self.send_and_wait(sx.Cmd.SET, param, req, timeout_s)
        return reply.data[0] if reply.data else -1

    @keyword("Get Vmap Range")
    def get_vmap_range(
        self, bank: int, enc: int, vmap: int, timeout_s: float = DEFAULT_TIMEOUT_S
    ) -> tuple[int, int]:
        req = sx.vmap_range_payload(bank, enc, vmap, 0, 0)
        reply = self.send_and_wait(sx.Cmd.GET, sx.Param.VMAP_RANGE, req, timeout_s)
        return reply.data[0], reply.data[1]

    @keyword("Set Vmap Range")
    def set_vmap_range(
        self,
        bank: int,
        enc: int,
        vmap: int,
        lower: int,
        upper: int,
        timeout_s: float = DEFAULT_TIMEOUT_S,
    ) -> int:
        req = sx.vmap_range_payload(bank, enc, vmap, lower, upper)
        reply = self.send_and_wait(sx.Cmd.SET, sx.Param.VMAP_RANGE, req, timeout_s)
        return reply.data[0] if reply.data else -1

    @keyword("Get Vmap Hsv")
    def get_vmap_hsv(
        self, bank: int, enc: int, vmap: int, timeout_s: float = DEFAULT_TIMEOUT_S
    ) -> tuple[int, int, int]:
        req = sx.vmap_hsv_payload(bank, enc, vmap, 0, 0, 0)
        reply = self.send_and_wait(sx.Cmd.GET, sx.Param.VMAP_HSV, req, timeout_s)
        hue = reply.data[0] | (reply.data[1] << 8)
        return hue, reply.data[2], reply.data[3]

    @keyword("Set Vmap Hsv")
    def set_vmap_hsv(
        self,
        bank: int,
        enc: int,
        vmap: int,
        hue: int,
        sat: int,
        val: int,
        timeout_s: float = DEFAULT_TIMEOUT_S,
    ) -> int:
        req = sx.vmap_hsv_payload(bank, enc, vmap, hue, sat, val)
        reply = self.send_and_wait(sx.Cmd.SET, sx.Param.VMAP_HSV, req, timeout_s)
        return reply.data[0] if reply.data else -1

    @keyword("Get Device Info")
    def get_device_info(self, timeout_s: float = DEFAULT_TIMEOUT_S) -> dict:
        reply = self.send_and_wait(sx.Cmd.GET, sx.Param.DEVICE_INFO, b"", timeout_s)
        d = reply.data
        return {
            "fw_version": f"{d[0]}.{d[1]}.{d[2]}",
            "num_encoders": d[3],
            "num_banks": d[4],
            "num_vmaps_per_encoder": d[5],
            "num_side_switches": d[6],
        }

    @keyword("Set Active Bank")
    def set_active_bank(self, bank: int, timeout_s: float = DEFAULT_TIMEOUT_S) -> int:
        req = sx.active_bank_payload(bank)
        reply = self.send_and_wait(sx.Cmd.SET, sx.Param.ACTIVE_BANK, req, timeout_s)
        return reply.data[0] if reply.data else -1

    @keyword("Build Vmap Range Payload")
    def build_vmap_range_payload(
        self, bank: int, enc: int, vmap: int, lower: int, upper: int
    ) -> bytes:
        """Exposes sysex.vmap_range_payload() as a keyword, for tests that
        need to send a deliberately invalid index (e.g. bounds-check
        tests) where the normal Set Vmap Range keyword's own bank/enc/vmap
        parameters would be too restrictive or the wrong shape."""
        return sx.vmap_range_payload(bank, enc, vmap, lower, upper)

    @keyword("Build Active Bank Payload")
    def build_active_bank_payload(self, bank: int) -> bytes:
        return sx.active_bank_payload(bank)

    @keyword("Expect No Response")
    def expect_no_response(
        self,
        cmd: sx.Cmd,
        param: sx.Param,
        data: bytes = b"",
        timeout_s: float = 1.0,
    ) -> None:
        """For malformed/out-of-range requests the firmware is expected to
        silently drop rather than NAK (see the bounds-check design in
        sysex.c) - asserts no reply for `param` arrives within `timeout_s`."""
        try:
            reply = self.send_and_wait(cmd, param, data, timeout_s)
        except AssertionError:
            return  # Correctly silent - this is the expected outcome.
        raise AssertionError(
            f"Expected no response, but got one: cmd={reply.cmd.name} "
            f"param={reply.param.name} data={reply.data.hex()}"
        )

    @keyword("Trigger Device Reset")
    def trigger_reset(self) -> None:
        """Device reset requires the CDC console (separate serial port),
        not available through this MIDI-only library. See
        tests/robot/README.md for how to trigger a reset manually or via a
        serial library in your suite."""
        raise NotImplementedError(
            "Device reset requires the CDC console (separate serial port), "
            "not available through this MIDI-only library. See "
            "tests/robot/README.md for how to trigger a reset manually or "
            "via a serial library in your suite."
        )
