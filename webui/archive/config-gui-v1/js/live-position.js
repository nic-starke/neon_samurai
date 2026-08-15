// live-position.js - tracks each encoder's live knob position from
// incoming MIDI CC messages, for rendering the digital twin's indicator
// LEDs against real hardware movement instead of a fixed neutral
// position (see ui.js's renderEncoderGrid()).
//
// This is the missing half of what device-model.js's loadFromDevice()
// gives you: that's the *configured* state (colour, range, the position
// *window* a vmap occupies) pulled via sysex GET; this is the *live*
// state (where the knob actually is right now), which has no sysex GET
// at all - the only way to observe it is to listen for the MIDI CC
// traffic the encoder itself transmits as it turns, then invert the
// same range/position mapping firmware uses to go the other way.
//
// Ported directly from firmware's own inverse mapping - see
// src/io/input_manager.c's midi_in_handler() (MIDI_EVENT_CC case) and
// convert_range_i16() in src/include/system/utility.h. Deliberately a
// faithful port, not an approximation, including the integer-truncating
// division and the "range.lower > range.upper reverses direction" case
// virtmap.h documents.
//
// CC-only: see midi.js's parseChannelMessage() doc comment - NOTE mode
// has no working firmware transmit implementation, so there is nothing
// to listen for from a vmap configured that way.

import { MidiMode } from "./device-model.js";

// Mirrors src/include/system/utility.h's convert_range_i16() exactly,
// including truncating (not rounding) integer division. Returns null
// instead of dividing by zero when omax === omin (a degenerate/
// unconfigured range) - firmware's own behaviour there is undefined
// (integer division by zero), so there is no "faithful" answer to port;
// null just means "can't derive a position from this config".
function convertRangeI16(c, omin, omax, nmin, nmax) {
	const or_ = omax - omin;
	if (or_ === 0) return null;
	const nr = nmax - nmin;
	return Math.trunc(((c - omin) * nr) / or_) + nmin;
}

/**
 * Tracks live encoder positions for one connected Device, derived from
 * its regular (non-sysex) CC traffic. Does not itself talk to the
 * device model's config state directly - callers pass the relevant
 * vmap's {range, position, proto} in on each lookup/update pass, since
 * this module has no opinion about which bank/vmap is "active" (that's
 * DeviceModel's job).
 */
export class LivePositionTracker {
	constructor() {
		/** @type {Map<string, number>} "bank.enc" -> last-known curr_pos (0-255) */
		this._positions = new Map();
		this._unsubscribe = null;
		/** @type {(() => void)|null} set by callers that want a re-render on every position change - see attach() */
		this.onUpdate = null;
	}

	/**
	 * Start listening on `device` for CC messages, resolving each one
	 * against `model`'s current config to figure out which encoder (if
	 * any) it belongs to.
	 * @param {import("./midi.js").Device} device
	 * @param {import("./device-model.js").DeviceModel} model
	 * @param {() => void} [onUpdate] - called after any position(s) actually change, for a re-render
	 */
	attach(device, model, onUpdate) {
		this.detach();
		this.onUpdate = onUpdate ?? null;
		this._unsubscribe = device.onChannelMessage((msg) => {
			if (msg.type !== "cc") return;
			this._handleCC(model, msg);
		});
	}

	detach() {
		if (this._unsubscribe) this._unsubscribe();
		this._unsubscribe = null;
		this.onUpdate = null;
	}

	/** Forget all tracked positions (e.g. on disconnect or model reload,
	 * so stale positions from a previous session/config don't linger). */
	reset() {
		this._positions.clear();
	}

	/**
	 * @param {number} bank
	 * @param {number} enc
	 * @returns {number|undefined} last-known live position (0-255), or
	 *   undefined if nothing has been observed for this encoder yet.
	 */
	getPosition(bank, enc) {
		return this._positions.get(`${bank}.${enc}`);
	}

	_handleCC(model, msg) {
		let changed = false;
		for (let bank = 0; bank < model.banks.length; bank++) {
			const encoders = model.banks[bank].encoders;
			for (let e = 0; e < encoders.length; e++) {
				const enc = encoders[e];
				for (const vmap of enc.vmaps) {
					const proto = vmap.proto;
					if (!proto || proto.mode !== MidiMode.CC) continue;
					if (proto.channel !== msg.channel) continue;
					if (proto.ccOrRaw !== msg.data1) continue;

					const newPos = convertRangeI16(
						msg.data2,
						vmap.range.lower,
						vmap.range.upper,
						vmap.position.start,
						vmap.position.stop,
					);
					if (newPos === null) continue;
					const clamped = Math.max(0, Math.min(255, newPos));

					const key = `${bank}.${e}`;
					if (this._positions.get(key) !== clamped) {
						this._positions.set(key, clamped);
						changed = true;
					}
				}
			}
		}
		if (changed) this.onUpdate?.();
	}
}
