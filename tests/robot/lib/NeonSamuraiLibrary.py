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

import time

import sysex as sx
from rawmidi import RawMidiPort, find_rawmidi_device
from robot.api import logger
from robot.api.deco import keyword, library

DEFAULT_TIMEOUT_S = 2.0
DEFAULT_PORT_SUBSTRING = "SAMURAI"

# How long to wait, after triggering a reset, before starting to poll for
# the device to come back. A plain reboot is fast; a config reset also
# does a byte-by-byte EEPROM erase (see init_eeprom() in config.c) that
# has been observed taking up to ~30s on real hardware, and that's before
# USB re-enumeration time on top - an earlier, tighter 40s budget was
# observed timing out on CONFIG_RESET specifically (not SYSTEM_RESET) in
# back-to-back suite runs, so this has real margin above the ~30s+
# worst case actually seen, not just the erase time alone.
RECONNECT_INITIAL_DELAY_S = 2.0
RECONNECT_POLL_TIMEOUT_S = 90.0
RECONNECT_POLL_INTERVAL_S = 1.0


class NoReply(AssertionError):
    """Raised only when a request went out and nothing came back.

    Not connected, and a reply for the wrong param, are also failures, but
    they are different failures - a silent-drop test that accepts any of the
    three passes just as happily with the device unplugged.
    """


def _to_signed16(word: int) -> int:
    """virtmap.range.{lower,upper} are i16 - widened from i8 so a 14-bit CC
    range (0-16383) is expressible."""
    return word - 65536 if word > 32767 else word


# Robot instantiates a library once per test case by default, which would
# discard the connection opened in Suite Setup and leave every test reporting
# "Not connected". One instance for the whole run is what the suite's
# setup/teardown structure assumes.
@library(scope="SUITE")
class NeonSamuraiLibrary:
    def __init__(self):
        self._port: RawMidiPort | None = None
        self._port_substring = DEFAULT_PORT_SUBSTRING
        self._backup: dict | None = None
        self._device_info: dict | None = None

    # --- Connection lifecycle ---------------------------------------------

    @keyword("Connect To Device")
    def connect(self, port_substring: str = DEFAULT_PORT_SUBSTRING) -> None:
        """Open the ALSA rawmidi device whose `amidi -l` name contains
        `port_substring`. Fails the test if no matching device is found -
        this library never falls back to a mock, by design."""
        self._port_substring = port_substring
        path = find_rawmidi_device(port_substring)
        self._port = RawMidiPort(path)

    @keyword("Disconnect From Device")
    def disconnect(self) -> None:
        if self._port is not None:
            self._port.close()
            self._port = None

    def _reconnect_after_reset(self, poll_timeout_s: float) -> None:
        """After a reset/config-reset SET ack is received, the device
        drops off the bus and re-enumerates - possibly under a different
        ALSA card index (observed directly: card index has shifted across
        a reset in this environment when other USB MIDI devices were
        present). Close the current handle and re-discover/re-open by
        name rather than assuming the same /dev/snd/midiCxDy path is
        still valid."""
        if self._port is not None:
            try:
                self._port.close()
            except OSError:
                pass  # fd may already be dead if the device vanished mid-op
            self._port = None

        time.sleep(RECONNECT_INITIAL_DELAY_S)

        deadline = time.monotonic() + poll_timeout_s
        last_error: Exception | None = None
        while time.monotonic() < deadline:
            try:
                path = find_rawmidi_device(self._port_substring)
                self._port = RawMidiPort(path)
                return
            except Exception as e:  # noqa: BLE001 - retry on any discovery/open failure
                last_error = e
                time.sleep(RECONNECT_POLL_INTERVAL_S)

        raise AssertionError(
            f"Device did not re-enumerate within {poll_timeout_s}s after "
            f"reset (last error: {last_error})"
        )

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
        traffic seen in between is discarded, not returned - this includes
        WEBUI_PUSH messages, which can share a param with a pending
        request/response pair (e.g. VMAP_CURR_POS) but are never a reply
        to it."""
        if self._port is None:
            raise AssertionError("Not connected - call 'Connect To Device' first")

        # Discard stale/unsolicited bytes sitting in the device buffer
        # before sending, so a leftover reply from a previous test can't be
        # mistaken for this request's response.
        self._port.drain()

        raw = bytes(sx.encode(cmd, param, data))
        self._port.send(raw)

        deadline = time.monotonic() + timeout_s
        while time.monotonic() < deadline:
            remaining = deadline - time.monotonic()
            reply_bytes = self._port.receive(remaining)
            if reply_bytes is None:
                break
            try:
                decoded = sx.decode(reply_bytes)
            except ValueError:
                continue
            if decoded.cmd == sx.Cmd.WEBUI_PUSH:
                continue
            if decoded.param != param:
                raise AssertionError(
                    f"Expected a reply for {param.name} but got one for "
                    f"{decoded.param.name}: {reply_bytes.hex()}"
                )
            return decoded

        raise NoReply(
            f"No sysex reply for {param.name} within {timeout_s}s "
            f"(cmd={cmd.name}, sent data={data.hex()})"
        )

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
        return (
            _to_signed16(reply.data[0] | (reply.data[1] << 8)),
            _to_signed16(reply.data[2] | (reply.data[3] << 8)),
        )

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

    @keyword("Get Vmap Curr Pos")
    def get_vmap_curr_pos(
        self, bank: int, enc: int, vmap: int, timeout_s: float = DEFAULT_TIMEOUT_S
    ) -> int:
        req = sx.vmap_curr_pos_payload(bank, enc, vmap, 0)
        reply = self.send_and_wait(sx.Cmd.GET, sx.Param.VMAP_CURR_POS, req, timeout_s)
        return reply.data[0]

    @keyword("Set Live Position Streaming")
    def set_live_position_streaming(
        self, enabled: bool, timeout_s: float = DEFAULT_TIMEOUT_S
    ) -> int:
        req = sx.live_position_stream_payload(enabled)
        reply = self.send_and_wait(
            sx.Cmd.SET, sx.Param.ENCODER_LIVE_POSITION_STREAM, req, timeout_s
        )
        return reply.data[0] if reply.data else -1

    @keyword("Wait For Live Position Push")
    def wait_for_live_position_push(self, timeout_s: float = DEFAULT_TIMEOUT_S) -> dict:
        """Waits for an unsolicited VMAP_CURR_POS WEBUI_PUSH - nothing is
        sent (unlike Send And Wait, there is no request to correlate a
        reply against; the firmware pushes this on its own). Streaming
        must already be enabled (see Set Live Position Streaming) and an
        encoder physically turned, which on hardware-in-the-loop tests
        means driving the quadrature/movement fixture, not sysex. Any
        unrelated sysex traffic seen while waiting is discarded, matching
        Send And Wait's behaviour."""
        if self._port is None:
            raise AssertionError("Not connected - call 'Connect To Device' first")

        deadline = time.monotonic() + timeout_s
        while time.monotonic() < deadline:
            remaining = deadline - time.monotonic()
            reply_bytes = self._port.receive(remaining)
            if reply_bytes is None:
                break
            try:
                decoded = sx.decode(reply_bytes)
            except ValueError:
                continue
            if decoded.cmd == sx.Cmd.WEBUI_PUSH and decoded.param == sx.Param.VMAP_CURR_POS:
                return {
                    "bank": decoded.data[0],
                    "enc": decoded.data[1],
                    "vmap": decoded.data[2],
                    "curr_pos": decoded.data[3],
                }
        raise AssertionError(
            f"No unsolicited VMAP_CURR_POS push received within {timeout_s}s"
        )

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
        except NoReply:
            return  # Correctly silent - this is the expected outcome.
        raise AssertionError(
            f"Expected no response, but got one: cmd={reply.cmd.name} "
            f"param={reply.param.name} data={reply.data.hex()}"
        )

    # --- Config backup and per-test baseline -----------------------------
    #
    # This suite writes to real settings on a real device, and two of its
    # tests factory-reset it. Without a backup that is somebody's saved
    # configuration gone. Suite Setup reads the whole configuration off the
    # device before anything is touched, and Suite Teardown writes it back.

    # Per (bank, encoder, vmap).
    _VMAP_PARAMS = ("range", "position", "hsv")

    # Per (bank, encoder), each a single byte.
    _ENCODER_PARAMS = (
        sx.Param.ENCODER_DETENT,
        sx.Param.ENCODER_DISPLAY_MODE,
        sx.Param.ENCODER_VMAP_MODE,
        sx.Param.ENCODER_VMAP_DISPLAY_MODE,
        sx.Param.ENCODER_SWITCH_MODE,
    )

    @keyword("Back Up Device Config")
    def back_up_device_config(self) -> dict:
        """Read the whole configuration off the device so it can be put
        back afterwards. Returns it as well, for tests that want to assert
        against what was there originally."""
        info = self.get_device_info()
        banks = info["num_banks"]
        encoders = info["num_encoders"]
        vmaps = info["num_vmaps_per_encoder"]

        backup = {
            "active_bank": self.get_active_bank(),
            "encoders": {},
            "vmaps": {},
        }

        for bank in range(banks):
            for enc in range(encoders):
                backup["encoders"][(bank, enc)] = {
                    param: self.get_encoder_param(param, bank, enc)[0]
                    for param in self._ENCODER_PARAMS
                }

                for vmap in range(vmaps):
                    backup["vmaps"][(bank, enc, vmap)] = {
                        "range": self.get_vmap_range(bank, enc, vmap),
                        "position": self.get_vmap_position(bank, enc, vmap),
                        "hsv": self.get_vmap_hsv(bank, enc, vmap),
                    }

        self._backup = backup
        logger.info(
            f"Backed up {len(backup['encoders'])} encoder(s) and "
            f"{len(backup['vmaps'])} vmap(s)"
        )
        return backup

    @keyword("Restore Device Config")
    def restore_device_config(self, settle_s: float = 6.0) -> None:
        """Write the backed-up configuration back to the device.

        The device only flushes to EEPROM every 5s (cfg_update() in
        config.c), so this waits afterwards - otherwise a restore followed
        by unplugging the device would leave the test suite's values on it,
        which is the exact outcome the backup exists to prevent.
        """
        if not getattr(self, "_backup", None):
            raise AssertionError("Nothing backed up - call 'Back Up Device Config' first")

        backup = self._backup

        for (bank, enc), params in backup["encoders"].items():
            for param, value in params.items():
                self.set_encoder_param(param, bank, enc, value)

        for (bank, enc, vmap), params in backup["vmaps"].items():
            lower, upper = params["range"]
            start, stop = params["position"]
            hue, sat, val = params["hsv"]
            self.set_vmap_range(bank, enc, vmap, lower, upper)
            self.set_vmap_position(bank, enc, vmap, start, stop)
            self.set_vmap_hsv(bank, enc, vmap, hue, sat, val)

        self.set_active_bank(backup["active_bank"])

        time.sleep(settle_s)
        logger.info("Configuration restored and flushed to EEPROM")

    @keyword("Set Element To Baseline")
    def set_element_to_baseline(self, bank: int, enc: int, vmap: int) -> None:
        """Put one bank/encoder/vmap into a fixed, documented state.

        Tests assert on values they set themselves, but they share a device:
        without this, a value left behind by an earlier test is
        indistinguishable from one this test wrote, and a test that has
        stopped writing what it thinks it writes still passes.
        """
        # Deliberately not the firmware's own defaults (0/127). "Factory Reset
        # Restores Defaults" asserts the device comes back to those, and it
        # could not tell a real reset from this baseline if the two matched.
        self.set_vmap_range(bank, enc, vmap, 3, 96)
        self.set_vmap_position(bank, enc, vmap, 0, 255)
        self.set_vmap_hsv(bank, enc, vmap, 512, 200, 128)
        self.set_encoder_param(sx.Param.ENCODER_DETENT, bank, enc, 0)

    @keyword("Set Elements Under Test To Baseline")
    def set_elements_under_test_to_baseline(self, enc: int = 0, vmap: int = 0) -> None:
        """The baseline for every bank's copy of one encoder/vmap - the
        elements this suite writes to."""
        for bank in range(self._banks()):
            self.set_element_to_baseline(bank, enc, vmap)

    def _banks(self) -> int:
        """Bank count, asked of the device once. It cannot change at runtime,
        and this is called on every test setup and teardown."""
        if self._device_info is None:
            self._device_info = self.get_device_info()
        return self._device_info["num_banks"]

    # --- Params the backup needs that had no keyword yet -----------------

    @keyword("Get Active Bank")
    def get_active_bank(self, timeout_s: float = DEFAULT_TIMEOUT_S) -> int:
        reply = self.send_and_wait(
            sx.Cmd.GET, sx.Param.ACTIVE_BANK, sx.active_bank_payload(0), timeout_s
        )
        return reply.data[0]

    @keyword("Get Vmap Position")
    def get_vmap_position(
        self, bank: int, enc: int, vmap: int, timeout_s: float = DEFAULT_TIMEOUT_S
    ) -> tuple[int, int]:
        req = sx.vmap_position_payload(bank, enc, vmap, 0, 0)
        reply = self.send_and_wait(sx.Cmd.GET, sx.Param.VMAP_POSITION, req, timeout_s)
        return (reply.data[0], reply.data[1])

    @keyword("Set Vmap Position")
    def set_vmap_position(
        self,
        bank: int,
        enc: int,
        vmap: int,
        start: int,
        stop: int,
        timeout_s: float = DEFAULT_TIMEOUT_S,
    ) -> int:
        req = sx.vmap_position_payload(bank, enc, vmap, start, stop)
        reply = self.send_and_wait(sx.Cmd.SET, sx.Param.VMAP_POSITION, req, timeout_s)
        return reply.data[0] if reply.data else -1

    def _trigger_reset(self, param: sx.Param, timeout_s: float) -> None:
        """Send a reset-trigger SET and reconnect afterward. The ack is
        sent by the firmware *before* it reboots (see the design note in
        sysex.c), so it's normally readable here - but the device
        physically drops off the USB bus very shortly after, and on a
        real (not virtual) device that disconnect can race the read
        itself, raising OSError from the rawmidi fd mid-read rather than
        a clean timeout. Either outcome (a normal ack, or the read
        erroring out because the device vanished) is equally valid
        evidence the reset is underway - the thing that actually needs to
        succeed is the reconnect after, which runs regardless."""
        try:
            self.send_and_wait(sx.Cmd.SET, param, b"", timeout_s)
        except (AssertionError, OSError):
            pass
        self._reconnect_after_reset(RECONNECT_POLL_TIMEOUT_S)

    @keyword("Reset Device")
    def reset_device(self, timeout_s: float = DEFAULT_TIMEOUT_S) -> None:
        """Soft-reboots the device via MF_SYSEX_PARAM_SYSTEM_RESET (config
        untouched) and reconnects once it re-enumerates."""
        self._trigger_reset(sx.Param.SYSTEM_RESET, timeout_s)

    @keyword("Factory Reset Device")
    def factory_reset_device(self, timeout_s: float = DEFAULT_TIMEOUT_S) -> None:
        """Factory-resets the device via MF_SYSEX_PARAM_CONFIG_RESET (wipes
        EEPROM back to defaults) and reboots, then reconnects once it
        re-enumerates. This is the keyword suites should use to establish
        a known starting state in Suite/Test Setup, rather than relying on
        whatever state a previous run left behind."""
        self._trigger_reset(sx.Param.CONFIG_RESET, timeout_s)
