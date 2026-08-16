// Tracks each encoder's live active vmap index (struct encoder.vmap_active,
// 0 or 1 - see NUM_VMAPS_PER_ENCODER in device-model.js) from the
// firmware's own unsolicited sysex push - see
// MF_SYSEX_PARAM_ENCODER_VMAP_ACTIVE in sysex.h and set_vmap_active() in
// src/io/input_manager.c, which sends a WEBUI_PUSH-shaped {bank, enc,
// active} message whenever a switch press changes it, while live
// streaming is enabled. Same pattern as live-position.js.

import { Cmd, Param } from "./sysex.js";

export class LiveVmapActiveTracker {
	constructor() {
		this._active = new Map(); // "bank.enc" -> last-known vmap_active index
		this._unsubscribe = null;
		this.onUpdate = null;
	}

	attach(device, onUpdate) {
		this.detach();
		this.onUpdate = onUpdate ?? null;
		this._unsubscribe = device.onSysex((msg) => {
			if (msg.cmd !== Cmd.WEBUI_PUSH || msg.param !== Param.ENCODER_VMAP_ACTIVE) return;
			const [bank, enc, active] = msg.data;
			const key = `${bank}.${enc}`;
			if (this._active.get(key) !== active) {
				this._active.set(key, active);
				this.onUpdate?.();
			}
		});
	}

	detach() {
		if (this._unsubscribe) this._unsubscribe();
		this._unsubscribe = null;
		this.onUpdate = null;
	}

	reset() {
		this._active.clear();
	}

	// Bulk-populates from an already-loaded DeviceModel's enc.vmapActive
	// fields (see device-model.js's loadFromDevice()).
	seed(model) {
		for (let bank = 0; bank < model.banks.length; bank++) {
			const encoders = model.banks[bank].encoders;
			for (let enc = 0; enc < encoders.length; enc++) {
				this._active.set(`${bank}.${enc}`, encoders[enc].vmapActive);
			}
		}
	}

	getActive(bank, enc) {
		return this._active.get(`${bank}.${enc}`);
	}
}
