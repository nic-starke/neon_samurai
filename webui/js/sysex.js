// Sysex wire-protocol encode/decode, a from-scratch reimplementation of
// src/midi/sysex.c rather than a binding to it - same approach as
// tests/robot/lib/sysex.py. Keep all three in sync; a divergence between two
// independent implementations is what this duplication exists to catch.
//
// Wire format:  F0 [mf_id x3] [cmd] [param_enum] [packed payload...] F7
//
// Sysex data bytes must be <= 0x7F but this protocol's values are full 8-bit,
// so every 7 raw bytes become 8 wire bytes: a header byte holding the stripped
// high bits, then the 7 bytes with their high bit cleared.

export const MFR_ID = [0x53, 0x41, 0x4d]; // "SAM"

export const SYSEX_START = 0xf0;
export const SYSEX_END = 0xf7;

// WEBUI_PUSH is outbound-only: an unsolicited value shaped like a
// GET_RESPONSE, tagged so a host can tell it from a reply it asked for.
export const Cmd = Object.freeze({
  GET: 0,
  GET_RESPONSE: 1,
  SET: 2,
  SET_RESPONSE: 3,
  STOP: 4,
  WEBUI_PUSH: 5,
});

// Mirrors enum mf_sysex_param in src/include/midi/sysex.h - membership and
// values must match exactly, since the numeric value goes on the wire.
//
// ENCODER_VMAP_ACTIVE, ACTIVE_BANK and VMAP_CURR_POS are also pushed
// unsolicited while ENCODER_LIVE_POSITION_STREAM is on (a SET-only trigger,
// 0 = stop, off by default and after every reboot). SYSTEM_RESET and
// CONFIG_RESET are SET-only triggers that carry no payload.
export const Param = Object.freeze({
  ENCODER_DETENT: 0,
  ENCODER_DISPLAY_MODE: 1,
  ENCODER_VMAP_DISPLAY_MODE: 2,
  ENCODER_VMAP_MODE: 3,
  ENCODER_VMAP_ACTIVE: 4,
  ENCODER_SWITCH_STATE: 5,
  ENCODER_SWITCH_MODE: 6,
  ENCODER_SWITCH_PROTO: 7,
  VMAP_RANGE: 8,
  VMAP_POSITION: 9,
  VMAP_RGB: 10,
  VMAP_RB: 11,
  VMAP_PROTO: 12,
  VMAP_HSV: 13,
  SIDE_SWITCH: 14,
  ACTIVE_BANK: 15,
  DEVICE_INFO: 16,
  VMAP_CURR_POS: 17,
  ENCODER_LIVE_POSITION_STREAM: 18,
  SYSTEM_RESET: 19,
  CONFIG_RESET: 20,
  // Guarded: the payload must be BOOTLOADER_KEY exactly. The two above cost a
  // reboot if they fire by accident; this one takes the device off the bus
  // until something flashes it or it is power-cycled.
  BOOTLOADER: 21,
});

// Spells "BOOT". Sysex data bytes carry seven bits, so every byte is under
// 0x80 - see MF_SYSEX_BOOTLOADER_KEY_* in src/include/midi/sysex.h.
export const BOOTLOADER_KEY = Object.freeze([0x42, 0x4f, 0x4f, 0x54]);

export function bootloaderPayload() {
  return Uint8Array.from(BOOTLOADER_KEY);
}

const PARAM_NAMES = Object.fromEntries(
  Object.entries(Param).map(([name, value]) => [value, name])
);

// Mirrors sysex_pack7()/sysex_unpack7() in src/midi/sysex.c.

export function pack7(data) {
  const out = [];
  for (let i = 0; i < data.length; i += 7) {
    const group = Array.from(data.slice(i, i + 7));
    let header = 0;
    const packedGroup = [];
    for (let j = 0; j < group.length; j++) {
      const b = group[j];
      if (b & 0x80) {
        header |= 1 << j;
      }
      packedGroup.push(b & 0x7f);
    }
    out.push(header, ...packedGroup);
  }
  return out;
}

export function unpack7(data) {
  const out = [];
  let i = 0;
  while (i < data.length) {
    const header = data[i];
    i += 1;
    const group = Array.from(data.slice(i, i + 7));
    i += group.length;
    for (let j = 0; j < group.length; j++) {
      let b = group[j];
      if (header & (1 << j)) {
        b |= 0x80;
      }
      out.push(b);
    }
  }
  return out;
}

export function encode(cmd, param, data = []) {
  const payload = pack7(data);
  return [SYSEX_START, ...MFR_ID, cmd, param, ...payload, SYSEX_END];
}

/**
 * @typedef {{cmd: number, param: number, paramName: string, data: number[]}} SysexMessage
 */

// Parses any device-to-host message. Their framing differs from requests:
// midi_out_handler() in midi_lufa.c emits the semantic (unpacked) data_len
// ahead of the packed data, outside the packed group, so it must be split off
// before unpack7() runs.
export function decode(raw) {
  raw = Array.from(raw);
  if (
    raw.length < 7 ||
    raw[0] !== SYSEX_START ||
    raw[raw.length - 1] !== SYSEX_END
  ) {
    throw new Error(`bad sysex framing: ${toHex(raw)}`);
  }
  if (raw[1] !== MFR_ID[0] || raw[2] !== MFR_ID[1] || raw[3] !== MFR_ID[2]) {
    throw new Error(`bad manufacturer id: ${toHex(raw.slice(1, 4))}`);
  }
  const cmd = raw[4];
  const param = raw[5];
  const dataLen = raw[6];
  const packedPayload = raw.slice(7, raw.length - 1);
  const data = unpack7(packedPayload);
  if (data.length !== dataLen) {
    throw new Error(
      `declared data_len (${dataLen}) doesn't match unpacked payload ` +
        `length (${data.length}): ${toHex(raw)}`
    );
  }
  return {
    cmd,
    param,
    paramName: PARAM_NAMES[param] ?? `UNKNOWN(${param})`,
    data,
  };
}

function toHex(bytes) {
  return Array.from(bytes)
    .map((b) => b.toString(16).padStart(2, "0"))
    .join(" ");
}

// Mirror the mf_sysex_*_param_s wire structs. Each returns unpacked payload
// bytes. On a GET the trailing value bytes are ignored by the firmware but
// must be present and correctly sized to pass its packet-length check.

export function encoderPayload(bank, enc, value) {
  return [bank, enc, value & 0xff];
}

// lower/upper are i16 (0-16383 for 14-bit CC), little-endian on the wire.
export function vmapRangePayload(bank, enc, vmap, lower, upper) {
  return [
    bank,
    enc,
    vmap,
    lower & 0xff,
    (lower >> 8) & 0xff,
    upper & 0xff,
    (upper >> 8) & 0xff,
  ];
}

export function vmapPositionPayload(bank, enc, vmap, start, stop) {
  return [bank, enc, vmap, start & 0xff, stop & 0xff];
}

export function vmapRgbPayload(bank, enc, vmap, r, g, b) {
  return [bank, enc, vmap, r & 0xff, g & 0xff, b & 0xff];
}

export function vmapRbPayload(bank, enc, vmap, r, b) {
  return [bank, enc, vmap, r & 0xff, b & 0xff];
}

// hue is u16 (0-1535), little-endian on the wire (AVR/GCC default).
export function vmapHsvPayload(bank, enc, vmap, hue, sat, val) {
  return [
    bank,
    enc,
    vmap,
    hue & 0xff,
    (hue >> 8) & 0xff,
    sat & 0xff,
    val & 0xff,
  ];
}

export function sideSwitchPayload(swIdx, mode) {
  return [swIdx, mode & 0xff];
}

export function activeBankPayload(bank) {
  return [bank & 0xff];
}

export function vmapCurrPosPayload(bank, enc, vmap, currPos) {
  return [bank, enc, vmap, currPos & 0xff];
}

export function livePositionStreamPayload(enabled) {
  return [enabled ? 1 : 0];
}
