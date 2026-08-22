"""
Sysex wire-protocol encode/decode for the neon_samurai firmware
(src/midi/sysex.c, src/include/midi/sysex.h).

This is a from-scratch reimplementation of the wire protocol, not a binding
to the firmware's C code - it exists specifically so hardware tests have an
independent second implementation of the protocol to cross-check the
firmware against (a bug shared between the firmware and this file would slip
through either implementation alone).

Wire format (see sysex.c's top-of-file comment for the original):
    F0 [mf_id x3] [cmd] [param_enum] [packed payload...] F7

Manufacturer ID is "SAM" (0x53 0x41 0x4D). The payload is 7-bit packed
(see pack7/unpack7 below) - sysex data bytes must be <= 0x7F, but this
protocol's values are full 8-bit, so every 7 raw bytes become 8 wire bytes:
a header byte (bit i = the stripped high bit of raw byte i) followed by the
7 bytes with their high bit cleared.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum

MFR_ID = (0x53, 0x41, 0x4D)  # "SAM"

SYSEX_START = 0xF0
SYSEX_END = 0xF7


class Cmd(IntEnum):
    GET = 0
    GET_RESPONSE = 1
    SET = 2
    SET_RESPONSE = 3
    STOP = 4
    # Outbound-only: an unsolicited value, not a reply to a request this
    # client sent. Same param/data shape a GET_RESPONSE for that param
    # would carry.
    WEBUI_PUSH = 5


class Param(IntEnum):
    """Mirrors enum mf_sysex_param in src/include/midi/sysex.h. Keep this
    list's order and membership in sync with that enum - the numeric value
    is what goes on the wire."""

    ENCODER_DETENT = 0
    ENCODER_DISPLAY_MODE = 1
    ENCODER_VMAP_DISPLAY_MODE = 2
    ENCODER_VMAP_MODE = 3
    # Also pushed unsolicited (Cmd.WEBUI_PUSH) when a switch press changes it.
    ENCODER_VMAP_ACTIVE = 4
    ENCODER_SWITCH_STATE = 5
    ENCODER_SWITCH_MODE = 6
    ENCODER_SWITCH_PROTO = 7
    VMAP_RANGE = 8
    VMAP_POSITION = 9
    VMAP_RGB = 10
    VMAP_RB = 11
    VMAP_PROTO = 12
    VMAP_HSV = 13
    SIDE_SWITCH = 14
    ACTIVE_BANK = 15
    DEVICE_INFO = 16
    # Live knob position (struct virtmap.curr_pos) - also pushed
    # unsolicited (Cmd.WEBUI_PUSH) while ENCODER_LIVE_POSITION_STREAM is
    # enabled.
    VMAP_CURR_POS = 17
    # SET-only trigger (data: 0 = stop, nonzero = start) controlling
    # whether VMAP_CURR_POS is streamed. Off by default and on every
    # device reboot.
    ENCODER_LIVE_POSITION_STREAM = 18
    SYSTEM_RESET = 19
    CONFIG_RESET = 20


def pack7(data: bytes) -> bytes:
    """Pack raw 8-bit bytes into 7-bit wire form. Mirrors sysex_pack7() in
    src/midi/sysex.c exactly - see that function's comment for the format."""
    out = bytearray()
    for i in range(0, len(data), 7):
        group = data[i : i + 7]
        header = 0
        packed_group = bytearray()
        for j, b in enumerate(group):
            if b & 0x80:
                header |= 1 << j
            packed_group.append(b & 0x7F)
        out.append(header)
        out.extend(packed_group)
    return bytes(out)


def unpack7(data: bytes) -> bytes:
    """Inverse of pack7(). Mirrors sysex_unpack7() in src/midi/sysex.c."""
    out = bytearray()
    i = 0
    while i < len(data):
        header = data[i]
        i += 1
        group = data[i : i + 7]
        i += len(group)
        for j, b in enumerate(group):
            if header & (1 << j):
                b |= 0x80
            out.append(b)
    return bytes(out)


@dataclass
class SysexMessage:
    cmd: Cmd
    param: Param
    data: bytes  # unpacked payload bytes


def encode(cmd: Cmd, param: Param, data: bytes = b"") -> list[int]:
    """Build the raw byte list (F0 ... F7) for a request. `data` is the
    unpacked payload appropriate for `param` - see build_* helpers below for
    per-param payload construction."""
    payload = pack7(data)
    return [SYSEX_START, *MFR_ID, int(cmd), int(param), *payload, SYSEX_END]


def decode(raw: bytes) -> SysexMessage:
    """Parse a raw *response* (F0 ... F7, a GET_RESPONSE/SET_RESPONSE from
    the device) back into cmd/param/unpacked data. Raises ValueError on any
    framing/mfr-ID mismatch.

    Response framing differs from request framing (see encode()): the
    firmware's midi_out_handler() (midi_lufa.c) sends one extra raw byte -
    the semantic (unpacked) data_len - before the 7-bit-packed data itself,
    so a client knows how many bytes to expect after unpacking without
    needing to already know the param. That data_len byte is NOT part of
    the packed group and must be split off before unpack7() runs, or the
    first packed group's header byte gets misread as if it were data_len
    (and everything after it decodes to garbage)."""
    if len(raw) < 7 or raw[0] != SYSEX_START or raw[-1] != SYSEX_END:
        raise ValueError(f"bad sysex framing: {raw.hex()}")
    if tuple(raw[1:4]) != MFR_ID:
        raise ValueError(f"bad manufacturer id: {raw[1:4].hex()}")
    cmd = Cmd(raw[4])
    param = Param(raw[5])
    data_len = raw[6]
    packed_payload = bytes(raw[7:-1])
    data = unpack7(packed_payload)
    if len(data) != data_len:
        raise ValueError(
            f"declared data_len ({data_len}) doesn't match unpacked "
            f"payload length ({len(data)}): {raw.hex()}"
        )
    return SysexMessage(cmd=cmd, param=param, data=data)


# --- Per-param payload builders -------------------------------------------
# Mirror the mf_sysex_*_param_s wire structs in sysex.h. Each returns the
# *unpacked* payload bytes for encode()'s `data` argument.


def encoder_payload(bank: int, enc: int, value: int) -> bytes:
    return bytes([bank, enc, value & 0xFF])


def vmap_range_payload(bank: int, enc: int, vmap: int, lower: int, upper: int) -> bytes:
    # lower/upper are i16 (0-16383 for 14-bit CC), little-endian on the wire.
    return bytes(
        [
            bank,
            enc,
            vmap,
            lower & 0xFF,
            (lower >> 8) & 0xFF,
            upper & 0xFF,
            (upper >> 8) & 0xFF,
        ]
    )


def vmap_position_payload(bank: int, enc: int, vmap: int, start: int, stop: int) -> bytes:
    return bytes([bank, enc, vmap, start & 0xFF, stop & 0xFF])


def vmap_rgb_payload(bank: int, enc: int, vmap: int, r: int, g: int, b: int) -> bytes:
    return bytes([bank, enc, vmap, r & 0xFF, g & 0xFF, b & 0xFF])


def vmap_rb_payload(bank: int, enc: int, vmap: int, r: int, b: int) -> bytes:
    return bytes([bank, enc, vmap, r & 0xFF, b & 0xFF])


def vmap_hsv_payload(bank: int, enc: int, vmap: int, hue: int, sat: int, val: int) -> bytes:
    """hue is u16 (0-1535), little-endian on the wire (AVR/GCC default)."""
    return bytes(
        [bank, enc, vmap, hue & 0xFF, (hue >> 8) & 0xFF, sat & 0xFF, val & 0xFF]
    )


def side_switch_payload(sw_idx: int, mode: int) -> bytes:
    return bytes([sw_idx, mode & 0xFF])


def active_bank_payload(bank: int) -> bytes:
    return bytes([bank & 0xFF])


def vmap_curr_pos_payload(bank: int, enc: int, vmap: int, curr_pos: int) -> bytes:
    return bytes([bank, enc, vmap, curr_pos & 0xFF])


def live_position_stream_payload(enabled: bool) -> bytes:
    return bytes([1 if enabled else 0])


def index_payload(*indices: int) -> bytes:
    """Bare index prefix with no data - used for GET requests, where the
    trailing data bytes are ignored by the firmware but still need to be
    present and correctly *sized* to pass the packet-length check."""
    return bytes(indices)
