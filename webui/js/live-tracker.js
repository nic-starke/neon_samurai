// Tracks a device field that the firmware pushes unsolicited while
// ENCODER_LIVE_POSITION_STREAM is enabled. `indexLength` is how many leading
// payload bytes address the field: 3 for VMAP_CURR_POS (bank/enc/vmap), 2 for
// ENCODER_VMAP_ACTIVE (bank/enc), 0 for the device-scoped ACTIVE_BANK.

import { Cmd } from "./sysex.js";

export class LivePushTracker {
	constructor(param, indexLength) {
		this.param = param;
		this.indexLength = indexLength;
		this._values = new Map();
		this._unsubscribe = null;
		this._onUpdate = null;
	}

	attach(device, onUpdate) {
		this.detach();
		this._onUpdate = onUpdate ?? null;
		this._unsubscribe = device.onSysex((msg) => {
			if (msg.cmd !== Cmd.WEBUI_PUSH || msg.param !== this.param) return;
			const key = msg.data.slice(0, this.indexLength).join(".");
			const value = msg.data[this.indexLength];
			if (this._values.get(key) === value) return;
			this._values.set(key, value);
			this._onUpdate?.(value);
		});
	}

	detach() {
		if (this._unsubscribe) this._unsubscribe();
		this._unsubscribe = null;
		this._onUpdate = null;
	}

	reset() {
		this._values.clear();
	}

	set(indices, value) {
		this._values.set(indices.join("."), value);
	}

	get(...indices) {
		return this._values.get(indices.join("."));
	}
}
