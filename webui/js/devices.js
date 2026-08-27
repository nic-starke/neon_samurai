// Which devices are attached, and which one the editor is talking to.
//
// Detection and connection are deliberately separate. Detection enumerates
// MIDI ports and matches on name: no port is opened and no byte is sent, so it
// runs on load and on every hot-plug without touching the hardware. Connecting
// opens the ports exclusively and starts the heartbeat, which is why it is a
// decision rather than something that happens on its own.
//
// One device is connected at a time. The rest are listed and left closed.

import * as midi from "./midi.js";

/** Mirrors the unit lifecycle in webui/spec.md 11.1, as far as it is built. */
export const DeviceState = Object.freeze({
	DETECTED: "detected",
	IDENTIFYING: "identifying",
	CONNECTED: "connected",
	BUSY: "busy",
	FAILED: "failed",
});

export class DeviceRegistry {
	/**
	 * @param p.requestAccess  injectable for tests; defaults to real Web MIDI
	 * @param p.openDevice     injectable for tests
	 */
	constructor(p = {}) {
		this._requestAccess = p.requestAccess ?? midi.requestAccess;
		this._openDevice = p.openDevice ?? midi.openDevice;
		this._listDevices = p.listDevices ?? midi.listDevices;

		this._access = null;
		this._entries = [];
		this._states = new Map();
		this._listeners = new Set();

		this.selectedId = null;
		this.device = null;
		this.error = null;
	}

	onChange(listener) {
		this._listeners.add(listener);
		return () => this._listeners.delete(listener);
	}

	_emit() {
		for (const listener of this._listeners) listener(this.list());
	}

	/**
	 * Ask for access and start watching.
	 *
	 * The permission prompt happens here. Denial is recorded rather than
	 * thrown, because an editor that cannot see devices should still load.
	 */
	async start() {
		try {
			this._access = await this._requestAccess();
		} catch (e) {
			this.error = e.message;
			this._emit();
			return false;
		}

		this._access.onstatechange = () => this.rescan();
		this.rescan();
		return true;
	}

	/** Re-enumerate. Free - no ports opened, no traffic. */
	rescan() {
		if (!this._access) return;

		this._entries = this._listDevices(this._access);
		const present = new Set(this._entries.map((e) => e.id));

		for (const id of [...this._states.keys()]) {
			if (!present.has(id)) this._states.delete(id);
		}

		for (const entry of this._entries) {
			if (!this._states.has(entry.id)) this._states.set(entry.id, DeviceState.DETECTED);
		}

		// A device that has gone while connected takes the connection with it.
		if (this.selectedId && !present.has(this.selectedId)) this._clearConnection();

		this._emit();
	}

	list() {
		return this._entries.map((entry) => ({
			id: entry.id,
			name: entry.name,
			state: this._states.get(entry.id) ?? DeviceState.DETECTED,
			selected: entry.id === this.selectedId,
		}));
	}

	get connected() {
		return this.device !== null;
	}

	_clearConnection() {
		if (this.device) this.device.destroy();
		this.device = null;
		this.selectedId = null;
	}

	/**
	 * Open one device, closing whichever was open before.
	 *
	 * A busy port is the common failure on Linux and gets its own state, so
	 * the row can say the device is there but held by something else rather
	 * than looking absent or broken.
	 */
	async connect(id) {
		const entry = this._entries.find((e) => e.id === id);
		if (!entry) return null;

		if (this.selectedId && this.selectedId !== id) await this.disconnect();

		this._states.set(id, DeviceState.IDENTIFYING);
		this.selectedId = id;
		this._emit();

		try {
			this.device = await this._openDevice(entry);
			this._states.set(id, DeviceState.CONNECTED);
			this.error = null;
			this._emit();
			return this.device;
		} catch (e) {
			this._states.set(id, e.name === "PortBusyError" ? DeviceState.BUSY : DeviceState.FAILED);
			this.selectedId = null;
			this.device = null;
			this.error = e.message;
			this._emit();
			throw e;
		}
	}

	async disconnect() {
		const id = this.selectedId;
		this._clearConnection();
		if (id) this._states.set(id, DeviceState.DETECTED);
		this._emit();
	}

	/**
	 * Connect to the only attached device.
	 *
	 * Only acts when exactly one is present and nothing is connected - with
	 * several attached there is no basis for choosing, so the user does.
	 */
	async autoConnect() {
		if (this.connected || this._entries.length !== 1) return null;

		const [entry] = this._entries;
		if (this._states.get(entry.id) !== DeviceState.DETECTED) return null;

		try {
			return await this.connect(entry.id);
		} catch {
			// Already recorded on the row; auto-connect failing is not worth
			// interrupting anyone over.
			return null;
		}
	}
}
