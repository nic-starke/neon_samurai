// In-memory mirror of the device's configuration:
// banks[3].encoders[16].vmaps[2] + sideSwitches[6]. Shape mirrors
// struct encoder / struct virtmap (src/include/system/hardware.h,
// src/include/virtmap/virtmap.h) closely enough to read naturally against
// the firmware source, without being a byte-exact serialization.
//
// Transfers are sequential, never parallelised: the protocol has no
// per-message request ID (see midi.js's request()), so concurrent requests
// for the same param cannot be correlated. A full load is ~205 round trips
// and suspends the device heartbeat for its duration.
//
// The enum mirrors below are the firmware's, by value - see the named
// source file against each.

import { Param } from "./sysex.js";

export const NUM_BANKS = 4;
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
// enum midi_mode (midi.h) - CC_14/REL_CC have no working transmit path in
// firmware yet.
export const MidiMode = Object.freeze({ DISABLED: 0, CC: 1, CC_14: 2, REL_CC: 3, NOTE: 4 });

// currPos is the knob's live rotation (struct virtmap.curr_pos, 0-255),
// distinct from `position`, which is the configured window this colour
// layer occupies. rb is the detent LED colour (gamma-corrected BCM,
// 0-31/channel), lit only at dead centre with detent on.
function defaultVmap() {
	return {
		range: { lower: 0, upper: 127 },
		position: { start: 0, stop: 255 },
		hsv: { hue: 0, sat: 0, val: 0 },
		proto: { mode: MidiMode.CC, channel: 0, ccOrRaw: 0 },
		currPos: 127,
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
	}

	async loadFromDevice(protocol, onProgress) {
		const total = NUM_BANKS * NUM_ENCODERS * (4 + NUM_VMAPS_PER_ENCODER * 6) + NUM_SIDE_SWITCHES;
		let done = 0;
		const tick = () => onProgress?.(++done, total);

		protocol.device.pauseHeartbeat();
		try {
			for (let bank = 0; bank < NUM_BANKS; bank++) {
				for (let enc = 0; enc < NUM_ENCODERS; enc++) {
					const model = this.banks[bank].encoders[enc];
					model.displayMode = await protocol.getEncoderParam(Param.ENCODER_DISPLAY_MODE, bank, enc);
					tick();
					model.detent = Boolean(await protocol.getEncoderParam(Param.ENCODER_DETENT, bank, enc));
					tick();
					model.vmapMode = await protocol.getEncoderParam(Param.ENCODER_VMAP_MODE, bank, enc);
					tick();
					model.vmapActive = await protocol.getEncoderParam(Param.ENCODER_VMAP_ACTIVE, bank, enc);
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
		} finally {
			protocol.device.resumeHeartbeat();
		}
	}
}
