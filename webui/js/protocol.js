// protocol.js - per-param GET/SET orchestration on top of midi.js/sysex.js.
//
// Mirrors tests/robot/lib/NeonSamuraiLibrary.py's keyword shape (one
// get*/set* function per param family) so the two independent client
// implementations - this GUI and the hardware test suite - stay easy to
// compare when the wire protocol changes. If you add a param here, add the
// matching keyword there too (and vice versa) - see
// tests/robot/README.md's "Adding a new test" section.

import { Cmd, Param, activeBankPayload, encoderPayload, sideSwitchPayload, vmapHsvPayload, vmapPositionPayload, vmapRangePayload, vmapRbPayload, vmapRgbPayload } from "./sysex.js";

/**
 * Thin wrapper around a midi.js Device exposing one method per sysex
 * param, each returning plain values instead of raw payload bytes.
 */
export class Protocol {
	/** @param {import("./midi.js").Device} device */
	constructor(device) {
		this.device = device;
	}

	// --- Device info -----------------------------------------------------

	async getDeviceInfo() {
		const reply = await this.device.request(Cmd.GET, Param.DEVICE_INFO);
		const d = reply.data;
		return {
			fwVersion: `${d[0]}.${d[1]}.${d[2]}`,
			numEncoders: d[3],
			numBanks: d[4],
			numVmapsPerEncoder: d[5],
			numSideSwitches: d[6],
		};
	}

	// --- Encoder-level params (detent, display mode, vmap mode/active,
	// switch mode/proto) - all share the same {bank, enc, u8 value} shape. ---

	async getEncoderParam(param, bank, enc) {
		const reply = await this.device.request(Cmd.GET, param, encoderPayload(bank, enc, 0));
		return reply.data[0];
	}

	async setEncoderParam(param, bank, enc, value) {
		const reply = await this.device.request(
			Cmd.SET,
			param,
			encoderPayload(bank, enc, value),
		);
		return reply.data[0]; // return code, 0 = success
	}

	// --- Vmap range / position -----------------------------------------

	async getVmapRange(bank, enc, vmap) {
		const reply = await this.device.request(
			Cmd.GET,
			Param.VMAP_RANGE,
			vmapRangePayload(bank, enc, vmap, 0, 0),
		);
		return { lower: toSigned8(reply.data[0]), upper: toSigned8(reply.data[1]) };
	}

	async setVmapRange(bank, enc, vmap, lower, upper) {
		const reply = await this.device.request(
			Cmd.SET,
			Param.VMAP_RANGE,
			vmapRangePayload(bank, enc, vmap, lower, upper),
		);
		return reply.data[0];
	}

	async getVmapPosition(bank, enc, vmap) {
		const reply = await this.device.request(
			Cmd.GET,
			Param.VMAP_POSITION,
			vmapPositionPayload(bank, enc, vmap, 0, 0),
		);
		return { start: reply.data[0], stop: reply.data[1] };
	}

	async setVmapPosition(bank, enc, vmap, start, stop) {
		const reply = await this.device.request(
			Cmd.SET,
			Param.VMAP_POSITION,
			vmapPositionPayload(bank, enc, vmap, start, stop),
		);
		return reply.data[0];
	}

	// --- Color: HSV is the writable source of truth; RGB is a read-only
	// derived mirror (see the note in sysex.h / color_update_vmap_rgb()). ---

	async getVmapHsv(bank, enc, vmap) {
		const reply = await this.device.request(
			Cmd.GET,
			Param.VMAP_HSV,
			vmapHsvPayload(bank, enc, vmap, 0, 0, 0),
		);
		const [lo, hi, sat, val] = reply.data;
		return { hue: lo | (hi << 8), sat, val };
	}

	async setVmapHsv(bank, enc, vmap, hue, sat, val) {
		const reply = await this.device.request(
			Cmd.SET,
			Param.VMAP_HSV,
			vmapHsvPayload(bank, enc, vmap, hue, sat, val),
		);
		return reply.data[0];
	}

	/** Read-only - reflects what the device is actually driving the LEDs
	 * with (gamma-corrected BCM brightness, 0-31 per channel), not a
	 * settable color. */
	async getVmapRgb(bank, enc, vmap) {
		const reply = await this.device.request(
			Cmd.GET,
			Param.VMAP_RGB,
			vmapRgbPayload(bank, enc, vmap, 0, 0, 0),
		);
		return { r: reply.data[0], g: reply.data[1], b: reply.data[2] };
	}

	async getVmapRb(bank, enc, vmap) {
		const reply = await this.device.request(
			Cmd.GET,
			Param.VMAP_RB,
			vmapRbPayload(bank, enc, vmap, 0, 0),
		);
		return { r: reply.data[0], b: reply.data[1] };
	}

	async setVmapRb(bank, enc, vmap, r, b) {
		const reply = await this.device.request(
			Cmd.SET,
			Param.VMAP_RB,
			vmapRbPayload(bank, enc, vmap, r, b),
		);
		return reply.data[0];
	}

	// --- MIDI proto config (mode/channel/cc) per vmap ---------------------

	async getVmapProto(bank, enc, vmap) {
		// Dummy payload must be sized to match struct proto_cfg's real wire
		// length (protocol.h) - type(1) + midi_cfg{mode(1),channel(1),
		// cc-or-raw(1)} = 4 bytes, given -fpack-struct -fshort-enums (see
		// CMakeLists.txt) packs each enum field down to 1 byte with no
		// padding. A too-short dummy payload here failed the firmware's
		// packet-length check silently (this protocol NAKs by dropping the
		// message, not replying with an error - see sysex.c), surfacing as
		// a GET timeout with no other symptom.
		const reply = await this.device.request(Cmd.GET, Param.VMAP_PROTO, [
			bank,
			enc,
			vmap,
			0,
			0,
			0,
			0,
		]);
		const [type, mode, channel, ccOrRaw] = reply.data;
		return { type, mode, channel, ccOrRaw };
	}

	async setVmapProto(bank, enc, vmap, type, mode, channel, ccOrRaw) {
		const reply = await this.device.request(Cmd.SET, Param.VMAP_PROTO, [
			bank,
			enc,
			vmap,
			type,
			mode,
			channel,
			ccOrRaw,
		]);
		return reply.data[0];
	}

	// --- Side switches -----------------------------------------------------

	async getSideSwitch(swIdx) {
		const reply = await this.device.request(
			Cmd.GET,
			Param.SIDE_SWITCH,
			sideSwitchPayload(swIdx, 0),
		);
		return reply.data[0];
	}

	async setSideSwitch(swIdx, mode) {
		const reply = await this.device.request(
			Cmd.SET,
			Param.SIDE_SWITCH,
			sideSwitchPayload(swIdx, mode),
		);
		return reply.data[0];
	}

	// --- Bank -----------------------------------------------------------

	async getActiveBank() {
		const reply = await this.device.request(Cmd.GET, Param.ACTIVE_BANK, activeBankPayload(0));
		return reply.data[0];
	}

	async setActiveBank(bank) {
		const reply = await this.device.request(
			Cmd.SET,
			Param.ACTIVE_BANK,
			activeBankPayload(bank),
		);
		return reply.data[0];
	}

	// --- Reset triggers ---------------------------------------------------
	// Both reboot the device - the reply races the actual disconnect (the
	// firmware sends the ack synchronously before rebooting, but the
	// device can still vanish from the OS's port list moments later), so
	// callers should tolerate the request() promise rejecting here even
	// on a "successful" reset, same as tests/robot/lib/NeonSamuraiLibrary.py
	// does. The caller is responsible for noticing the device disconnect
	// (see midi.js/ui.js's MIDIAccess statechange handling) and
	// reconnecting - this class doesn't reconnect on its own.

	/** Soft reboot - config untouched. */
	async resetDevice() {
		try {
			await this.device.request(Cmd.SET, Param.SYSTEM_RESET, []);
		} catch {
			// Expected sometimes - see note above.
		}
	}

	/** Factory reset - wipes EEPROM back to defaults, then reboots. */
	async factoryResetDevice() {
		try {
			await this.device.request(Cmd.SET, Param.CONFIG_RESET, []);
		} catch {
			// Expected sometimes - see note above.
		}
	}
}

/** virtmap.range.{lower,upper} are i8 in firmware (src/include/virtmap/virtmap.h) - convert the raw u8 wire byte to a signed JS number. */
function toSigned8(byte) {
	return byte > 127 ? byte - 256 : byte;
}
