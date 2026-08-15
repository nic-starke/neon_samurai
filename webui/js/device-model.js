// In-memory mirror of the device's configuration:
// banks[3].encoders[16].vmaps[2] + sideSwitches[6]. Shape mirrors
// struct encoder / struct virtmap (src/include/system/hardware.h,
// src/include/virtmap/virtmap.h) closely enough to read naturally
// against the firmware source, without being a byte-exact serialization.

import { Param } from "./sysex.js";

export const NUM_BANKS = 3;
export const NUM_ENCODERS = 16;
export const NUM_VMAPS_PER_ENCODER = 2;
export const NUM_SIDE_SWITCHES = 6;

// enum display_mode (hardware.h)
export const DisplayMode = Object.freeze({ SINGLE: 0, MULTI: 1, MULTI_PWM: 2 });
// enum virtmap_mode (virtmap.h)
export const VmapMode = Object.freeze({ TOGGLE: 0, OVERLAY: 1 });
// enum switch_mode (hardware.h)
export const SwitchMode = Object.freeze({
	NONE: 0,
	VMAP_CYCLE: 1,
	VMAP_HOLD: 2,
	RESET_ON_PRESS: 3,
	RESET_ON_RELEASE: 4,
	FINE_ADJUST_TOGGLE: 5,
	FINE_ADJUST_HOLD: 6,
	MIDI: 7,
});
// enum side_switch_mode (hardware.h)
export const SideSwitchMode = Object.freeze({
	NONE: 0,
	ALL_VMAP_CYCLE: 1,
	ALL_VMAP_HOLD: 2,
	BANK_PREV: 3,
	BANK_NEXT: 4,
	RESERVED: 5,
});
// enum midi_mode (midi.h) - CC_14/REL_CC exist in firmware but have no
// working transmit implementation yet (see module-architecture skill's
// notes and the plan this GUI was built from) - don't offer them in v1 UI.
export const MidiMode = Object.freeze({ DISABLED: 0, CC: 1, CC_14: 2, REL_CC: 3, NOTE: 4 });
// enum protocol_type (protocol.h) - only PROTOCOL_MIDI is implemented;
// PROTOCOL_OSC is a stub in the firmware (see module-architecture skill).
export const ProtocolType = Object.freeze({ NONE: 0, MIDI: 1, OSC: 2 });

function defaultVmap() {
	return {
		range: { lower: 0, upper: 127 },
		position: { start: 0, stop: 255 },
		hsv: { hue: 0, sat: 0, val: 0 },
		proto: { mode: MidiMode.CC, channel: 0, ccOrRaw: 0 },
		// Live knob rotation (struct virtmap.curr_pos, 0-255) - distinct
		// from `position` above, which is the *configured window* this
		// layer occupies. Populated by loadFromDevice() below and kept
		// current after that by live-position.js's sysex push (see
		// live-twin.js) - not round-tripped by saveToDevice(), which has
		// nothing meaningful to write back for a live-only value.
		currPos: 127,
		// Detent LED colour (struct virtmap.rb - gamma-corrected BCM,
		// 0-31/channel, same representation as the derived `rgb` field but
		// independently settable, not derived from `hsv`) - see
		// mf_draw_encoder() in src/led/led.c: only actually lit while
		// detent is on and the knob is at dead centre.
		rb: { r: 0, b: 0 },
	};
}

function defaultEncoder(idx) {
	return {
		idx,
		displayMode: DisplayMode.SINGLE,
		vmapDisplayMode: 0,
		detent: false,
		vmapMode: VmapMode.TOGGLE,
		vmapActive: 0,
		vmaps: [defaultVmap(), defaultVmap()],
		switchMode: SwitchMode.NONE,
		switchProto: { mode: MidiMode.CC, channel: 0, ccOrRaw: 0 },
	};
}

function defaultBank() {
	return { encoders: Array.from({ length: NUM_ENCODERS }, (_, i) => defaultEncoder(i)) };
}

export class DeviceModel {
	constructor() {
		this.banks = Array.from({ length: NUM_BANKS }, () => defaultBank());
		this.sideSwitches = Array.from({ length: NUM_SIDE_SWITCHES }, () => SideSwitchMode.NONE);
		this.activeBank = 0;
		this.deviceInfo = null;
		// "bank.enc" or "bank.enc.vmap.field" keys touched since the last
		// load/save - for a future dirty-indicator UI, not yet wired up.
		this.dirty = new Set();
	}

	markDirty(key) {
		this.dirty.add(key);
	}

	clearDirty() {
		this.dirty.clear();
	}

	// Sequential, not parallelized - this protocol has no per-message
	// request ID (see midi.js's request()), so concurrent requests for the
	// same param can't be reliably correlated. ~205 round trips total.
	async loadFromDevice(protocol, onProgress) {
		this.deviceInfo = await protocol.getDeviceInfo();
		this.activeBank = await protocol.getActiveBank();

		const total =
			NUM_BANKS * NUM_ENCODERS * (4 + NUM_VMAPS_PER_ENCODER * 6) + NUM_SIDE_SWITCHES;
		let done = 0;
		const tick = () => onProgress?.(++done, total);

		for (let bank = 0; bank < NUM_BANKS; bank++) {
			for (let enc = 0; enc < NUM_ENCODERS; enc++) {
				const model = this.banks[bank].encoders[enc];
				model.displayMode = await protocol.getEncoderParam(
					Param.ENCODER_DISPLAY_MODE,
					bank,
					enc,
				);
				tick();
				model.detent = Boolean(
					await protocol.getEncoderParam(Param.ENCODER_DETENT, bank, enc),
				);
				tick();
				// Which colour layer is actually driving output right now -
				// without this, live position pushes (which the firmware
				// only sends for the vmap(s) it's actually updating; see
				// vmap_update() in src/io/input_manager.c) can arrive on a
				// vmap index the twin never reads, since it'd otherwise
				// always assume layer 0.
				model.vmapMode = await protocol.getEncoderParam(
					Param.ENCODER_VMAP_MODE,
					bank,
					enc,
				);
				tick();
				model.vmapActive = await protocol.getEncoderParam(
					Param.ENCODER_VMAP_ACTIVE,
					bank,
					enc,
				);
				tick();

				for (let vmap = 0; vmap < NUM_VMAPS_PER_ENCODER; vmap++) {
					const v = model.vmaps[vmap];
					v.range = await protocol.getVmapRange(bank, enc, vmap);
					tick();
					v.position = await protocol.getVmapPosition(bank, enc, vmap);
					tick();
					v.hsv = await protocol.getVmapHsv(bank, enc, vmap);
					tick();
					v.proto = await protocol.getVmapProto(bank, enc, vmap);
					tick();
					v.currPos = await protocol.getVmapCurrPos(bank, enc, vmap);
					tick();
					v.rb = await protocol.getVmapRb(bank, enc, vmap);
					tick();
				}
			}
		}

		for (let sw = 0; sw < NUM_SIDE_SWITCHES; sw++) {
			this.sideSwitches[sw] = await protocol.getSideSwitch(sw);
			tick();
		}

		this.clearDirty();
	}

	// Same sequential-round-trip caveat as loadFromDevice().
	async saveToDevice(protocol, onProgress) {
		const total =
			NUM_BANKS * NUM_ENCODERS * (2 + NUM_VMAPS_PER_ENCODER * 4) + NUM_SIDE_SWITCHES + 1;
		let done = 0;
		const tick = () => onProgress?.(++done, total);

		for (let bank = 0; bank < NUM_BANKS; bank++) {
			for (let enc = 0; enc < NUM_ENCODERS; enc++) {
				const model = this.banks[bank].encoders[enc];
				await protocol.setEncoderParam(
					Param.ENCODER_DISPLAY_MODE,
					bank,
					enc,
					model.displayMode,
				);
				tick();
				await protocol.setEncoderParam(
					Param.ENCODER_DETENT,
					bank,
					enc,
					model.detent ? 1 : 0,
				);
				tick();

				for (let vmap = 0; vmap < NUM_VMAPS_PER_ENCODER; vmap++) {
					const v = model.vmaps[vmap];
					await protocol.setVmapRange(bank, enc, vmap, v.range.lower, v.range.upper);
					tick();
					await protocol.setVmapPosition(bank, enc, vmap, v.position.start, v.position.stop);
					tick();
					await protocol.setVmapHsv(bank, enc, vmap, v.hsv.hue, v.hsv.sat, v.hsv.val);
					tick();
					await protocol.setVmapProto(
						bank,
						enc,
						vmap,
						ProtocolType.MIDI,
						v.proto.mode,
						v.proto.channel,
						v.proto.ccOrRaw,
					);
					tick();
				}
			}
		}

		for (let sw = 0; sw < NUM_SIDE_SWITCHES; sw++) {
			await protocol.setSideSwitch(sw, this.sideSwitches[sw]);
			tick();
		}

		await protocol.setActiveBank(this.activeBank);
		tick();

		this.clearDirty();
	}

	toJSON() {
		return {
			formatVersion: 1,
			deviceInfo: this.deviceInfo,
			activeBank: this.activeBank,
			banks: this.banks,
			sideSwitches: this.sideSwitches,
		};
	}

	/** Overwrite this model's state from a previously-saved snapshot. */
	loadFromJSON(obj) {
		if (obj.formatVersion !== 1) {
			throw new Error(
				`Unsupported preset format version ${obj.formatVersion} (expected 1)`,
			);
		}
		this.activeBank = obj.activeBank ?? 0;
		this.banks = obj.banks;
		this.sideSwitches = obj.sideSwitches;
		this.markDirty("*"); // Loaded state hasn't been pushed to the device yet
	}
}
