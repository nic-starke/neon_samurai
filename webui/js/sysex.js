// sysex.js - sysex wire-protocol encode/decode for the neon_samurai
// firmware (src/midi/sysex.c, src/include/midi/sysex.h).
//
// This is a from-scratch reimplementation of the wire protocol (a JS port
// of tests/robot/lib/sysex.py, which is itself independent of the
// firmware's C code) - not a binding. Keep this in sync with sysex.py and
// sysex.h whenever the protocol changes; a divergence between the two
// implementations is exactly the kind of bug this duplication exists to
// help catch (see sysex.py's module docstring).
//
// Wire format (see sysex.c's top-of-file comment for the original):
//   F0 [mf_id x3] [cmd] [param_enum] [packed payload...] F7
//
// Manufacturer ID is "SAM" (0x53 0x41 0x4D). The payload is 7-bit packed
// (see pack7/unpack7 below) - sysex data bytes must be <= 0x7F, but this
// protocol's values are full 8-bit, so every 7 raw bytes become 8 wire
// bytes: a header byte (bit i = the stripped high bit of raw byte i)
// followed by the 7 bytes with their high bit cleared.

export const MFR_ID = [0x53, 0x41, 0x4d]; // "SAM"

export const SYSEX_START = 0xf0;
export const SYSEX_END = 0xf7;

export const Cmd = Object.freeze({
	GET: 0,
	GET_RESPONSE: 1,
	SET: 2,
	SET_RESPONSE: 3,
	STOP: 4,
});

// Mirrors enum mf_sysex_param in src/include/midi/sysex.h. Keep this
// object's membership and values in sync with that enum exactly - the
// numeric value is what goes on the wire.
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
	SYSTEM_RESET: 17,
	CONFIG_RESET: 18,
});

// Reverse lookup (numeric wire value -> name), for logging/debugging.
const PARAM_NAMES = Object.fromEntries(
	Object.entries(Param).map(([name, value]) => [value, name]),
);

/**
 * Pack raw 8-bit bytes into 7-bit wire form. Mirrors sysex_pack7() in
 * src/midi/sysex.c exactly - see that function's comment for the format.
 * @param {number[]|Uint8Array} data
 * @returns {number[]}
 */
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

/**
 * Inverse of pack7(). Mirrors sysex_unpack7() in src/midi/sysex.c.
 * @param {number[]|Uint8Array} data
 * @returns {number[]}
 */
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

/**
 * Build the raw byte array (F0 ... F7) for a request.
 * @param {number} cmd one of Cmd
 * @param {number} param one of Param
 * @param {number[]|Uint8Array} [data] unpacked payload appropriate for
 *   `param` - see the build*Payload() helpers below.
 * @returns {number[]}
 */
export function encode(cmd, param, data = []) {
	const payload = pack7(data);
	return [SYSEX_START, ...MFR_ID, cmd, param, ...payload, SYSEX_END];
}

/**
 * @typedef {{cmd: number, param: number, paramName: string, data: number[]}} SysexMessage
 */

/**
 * Parse a raw *response* (F0 ... F7, a GET_RESPONSE/SET_RESPONSE from the
 * device) back into cmd/param/unpacked data. Throws on any framing/mfr-ID
 * mismatch.
 *
 * Response framing differs from request framing (see encode()): the
 * firmware's midi_out_handler() (midi_lufa.c) sends one extra raw byte -
 * the semantic (unpacked) data_len - before the 7-bit-packed data itself,
 * so a client knows how many bytes to expect after unpacking without
 * needing to already know the param. That data_len byte is NOT part of the
 * packed group and must be split off before unpack7() runs.
 * @param {number[]|Uint8Array} raw
 * @returns {SysexMessage}
 */
export function decode(raw) {
	raw = Array.from(raw);
	if (raw.length < 7 || raw[0] !== SYSEX_START || raw[raw.length - 1] !== SYSEX_END) {
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
				`length (${data.length}): ${toHex(raw)}`,
		);
	}
	return { cmd, param, paramName: PARAM_NAMES[param] ?? `UNKNOWN(${param})`, data };
}

function toHex(bytes) {
	return Array.from(bytes)
		.map((b) => b.toString(16).padStart(2, "0"))
		.join(" ");
}

// --- Per-param payload builders --------------------------------------
// Mirror the mf_sysex_*_param_s wire structs in sysex.h. Each returns the
// *unpacked* payload bytes for encode()'s `data` argument.

export function encoderPayload(bank, enc, value) {
	return [bank, enc, value & 0xff];
}

export function vmapRangePayload(bank, enc, vmap, lower, upper) {
	return [bank, enc, vmap, lower & 0xff, upper & 0xff];
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

/** hue is u16 (0-1535), little-endian on the wire (AVR/GCC default). */
export function vmapHsvPayload(bank, enc, vmap, hue, sat, val) {
	return [bank, enc, vmap, hue & 0xff, (hue >> 8) & 0xff, sat & 0xff, val & 0xff];
}

export function sideSwitchPayload(swIdx, mode) {
	return [swIdx, mode & 0xff];
}

export function activeBankPayload(bank) {
	return [bank & 0xff];
}

/**
 * Bare index prefix with no data - used for GET requests, where the
 * trailing data bytes are ignored by the firmware but still need to be
 * present and correctly *sized* to pass the packet-length check.
 */
export function indexPayload(...indices) {
	return indices;
}
