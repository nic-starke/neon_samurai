// Tracks each encoder's live knob position (struct virtmap.curr_pos, 0-255)
// from the firmware's own unsolicited sysex push - see
// MF_SYSEX_PARAM_VMAP_CURR_POS/ENCODER_LIVE_POSITION_STREAM in sysex.h and
// vmap_update() in src/io/input_manager.c, which sends a GET_RESPONSE-
// shaped {bank, enc, vmap, curr_pos} message on every encoder movement
// while streaming is enabled. Call enable()/disable() (via Protocol) to
// turn that streaming on/off - it defaults off, so a client that never
// asks for it sees zero extra sysex traffic.

import { Cmd, Param } from "./sysex.js";

export class LivePositionTracker {
	constructor() {
		this._positions = new Map(); // "bank.enc.vmap" -> last-known curr_pos (0-255)
		this._unsubscribe = null;
		this.onUpdate = null;
	}

	attach(device, onUpdate) {
		this.detach();
		this.onUpdate = onUpdate ?? null;
		this._unsubscribe = device.onSysex((msg) => {
			if (msg.cmd !== Cmd.GET_RESPONSE || msg.param !== Param.VMAP_CURR_POS) return;
			const [bank, enc, vmap, currPos] = msg.data;
			const key = `${bank}.${enc}.${vmap}`;
			if (this._positions.get(key) !== currPos) {
				this._positions.set(key, currPos);
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
		this._positions.clear();
	}

	// Bulk-populates from an already-loaded DeviceModel's vmap.currPos
	// fields (see device-model.js's loadFromDevice(), which pulls each
	// vmap's currPos via an explicit GET) - without this, the twin shows
	// the neutral fallback for every encoder until each one has moved at
	// least once since streaming was enabled, even though the real
	// position was available all along from the initial config pull.
	seed(model) {
		for (let bank = 0; bank < model.banks.length; bank++) {
			const encoders = model.banks[bank].encoders;
			for (let enc = 0; enc < encoders.length; enc++) {
				encoders[enc].vmaps.forEach((v, vmap) => {
					this._positions.set(`${bank}.${enc}.${vmap}`, v.currPos);
				});
			}
		}
	}

	getPosition(bank, enc, vmap) {
		return this._positions.get(`${bank}.${enc}.${vmap}`);
	}
}
