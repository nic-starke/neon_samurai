// midi.js - Web MIDI transport: device discovery, connection, and raw
// sysex send/receive with request/response correlation.
//
// Browser support note: Web MIDI with sysex access (`{sysex: true}`) is
// Chrome/Edge (Chromium) only as of this writing - Firefox does not
// implement it by default, Safari's support is partial/recent. Callers
// should check `isSupported()` before anything else and show a clear
// message rather than failing silently - see ui.js.

import { decode, encode, Cmd, Param } from "./sysex.js";

const DEFAULT_PORT_SUBSTRING = "SAMURAI";
const DEFAULT_TIMEOUT_MS = 2000;

// Heartbeat: periodically GET DEVICE_INFO (cheap - static 7-byte reply, no
// EEPROM/gENCODERS traversal on the firmware side) to detect a device that
// has hung, gone to sleep, or otherwise stopped responding while still
// showing as "connected" at the OS/driver level. This is the second of two
// independent disconnect signals Device watches - the other is each port's
// native `statechange` event, which fires when the device actually
// unplugs/re-enumerates (instant, free, no traffic of its own) but says
// nothing about whether the firmware behind it is still alive.
const HEARTBEAT_INTERVAL_MS = 200;
const HEARTBEAT_TIMEOUT_MS = 3000;
const HEARTBEAT_MAX_MISSES = 2; // consecutive misses before declaring disconnected

/** @returns {boolean} whether this browser exposes navigator.requestMIDIAccess at all. */
export function isSupported() {
	return typeof navigator !== "undefined" && "requestMIDIAccess" in navigator;
}

/**
 * A connected device's input+output MIDI port pair, plus the raw
 * request/response sysex plumbing. One instance per connected device.
 */
export class Device {
	/**
	 * @param {MIDIInput} input
	 * @param {MIDIOutput} output
	 */
	constructor(input, output) {
		this.input = input;
		this.output = output;
		/** @type {((msg: import("./sysex.js").SysexMessage) => void)[]} */
		this._listeners = [];
		/** @type {((reason: string) => void)[]} */
		this._disconnectListeners = [];
		this._alive = true;
		this._heartbeatMisses = 0;
		this._heartbeatTimer = null;
		this._pauseDepth = 0;

		this.input.addEventListener("midimessage", (e) => this._onMessage(e));
		// Native port-level disconnect signal - fires on actual unplug/
		// re-enumeration, independent of the heartbeat below.
		this.input.addEventListener("statechange", (e) => this._onStateChange(e));
		this.output.addEventListener("statechange", (e) => this._onStateChange(e));

		this._startHeartbeat();
	}

	get name() {
		return this.output.name ?? this.input.name ?? "unknown device";
	}

	/** @returns {boolean} whether this Device still believes the connection is live - false once any disconnect signal has fired. */
	get isAlive() {
		return this._alive;
	}

	/**
	 * Subscribe to disconnect - fires at most once per Device instance, the
	 * first time either the native port statechange or the heartbeat
	 * declares the connection gone. Returns an unsubscribe function.
	 * @param {(reason: string) => void} listener
	 */
	onDisconnect(listener) {
		this._disconnectListeners.push(listener);
		return () => {
			this._disconnectListeners = this._disconnectListeners.filter((l) => l !== listener);
		};
	}

	/** Stop the heartbeat and mark this Device dead without waiting for a
	 * disconnect signal - call when intentionally disconnecting so a stale
	 * heartbeat timer doesn't fire after the fact. */
	destroy() {
		this._alive = false;
		if (this._heartbeatTimer) clearInterval(this._heartbeatTimer);
	}

	/**
	 * Suspend the heartbeat ping for the duration of a long bulk operation
	 * (e.g. DeviceModel.loadFromDevice()/saveToDevice(), ~200 sequential
	 * requests) - a ping queued behind that traffic could time out purely
	 * from being busy, not actually dead, and every one of those ~200
	 * requests succeeding is itself much stronger evidence of aliveness
	 * than a single heartbeat ping would be. Nests safely: paused while
	 * `_pauseDepth > 0`, so overlapping bulk operations don't have one's
	 * completion prematurely resume the heartbeat for the other still in
	 * flight. Native statechange disconnect detection (an actual unplug)
	 * keeps working regardless - only the periodic ping is suspended.
	 */
	pauseHeartbeat() {
		this._pauseDepth++;
	}

	resumeHeartbeat() {
		this._pauseDepth = Math.max(0, this._pauseDepth - 1);
	}

	_onStateChange(event) {
		if (!this._alive) return;
		const port = event.port;
		if (port.state === "disconnected") {
			this._declareDead(`${port.name ?? "port"} disconnected`);
		}
	}

	_startHeartbeat() {
		this._heartbeatTimer = setInterval(async () => {
			if (!this._alive || this._pauseDepth > 0) return;
			try {
				await this.request(Cmd.GET, Param.DEVICE_INFO, [], HEARTBEAT_TIMEOUT_MS);
				this._heartbeatMisses = 0;
			} catch {
				this._heartbeatMisses++;
				if (this._heartbeatMisses >= HEARTBEAT_MAX_MISSES) {
					this._declareDead(
						`no response to ${HEARTBEAT_MAX_MISSES} consecutive heartbeat pings`,
					);
				}
			}
		}, HEARTBEAT_INTERVAL_MS);
	}

	_declareDead(reason) {
		if (!this._alive) return;
		this._alive = false;
		if (this._heartbeatTimer) clearInterval(this._heartbeatTimer);
		for (const listener of this._disconnectListeners) {
			listener(reason);
		}
	}

	_onMessage(event) {
		const data = event.data;
		if (!data || data[0] !== 0xf0) {
			return; // Not sysex - this GUI only cares about sysex traffic.
		}
		let msg;
		try {
			msg = decode(data);
		} catch (e) {
			console.warn("midi.js: dropped unparseable sysex message", e, data);
			return;
		}
		for (const listener of this._listeners) {
			listener(msg);
		}
	}

	/**
	 * Subscribe to every decoded sysex reply from this device. Returns an
	 * unsubscribe function. Most callers should use `request()` instead;
	 * this is for cases that need to observe traffic without driving a
	 * specific request (e.g. a live encoder-turn indicator).
	 * @param {(msg: import("./sysex.js").SysexMessage) => void} listener
	 */
	onSysex(listener) {
		this._listeners.push(listener);
		return () => {
			this._listeners = this._listeners.filter((l) => l !== listener);
		};
	}

	/**
	 * Send a sysex request and resolve with the first reply whose `param`
	 * matches. Rejects on timeout. Concurrent requests for *different*
	 * params are safe (each `request()` call only watches for its own
	 * param); concurrent requests for the *same* param are not
	 * correlated beyond "first matching reply wins" - this protocol has
	 * no per-message request ID, only positional ordering on a single
	 * pipe (see the note in sysex.h), so callers that need strict
	 * ordering should await each request before sending the next.
	 * @param {number} cmd one of Cmd
	 * @param {number} param one of Param
	 * @param {number[]} [data]
	 * @param {number} [timeoutMs]
	 * @returns {Promise<import("./sysex.js").SysexMessage>}
	 */
	request(cmd, param, data = [], timeoutMs = DEFAULT_TIMEOUT_MS) {
		if (!this._alive) {
			return Promise.reject(new Error("Device is disconnected"));
		}
		return new Promise((resolve, reject) => {
			let settled = false;
			const timer = setTimeout(() => {
				if (settled) return;
				settled = true;
				unsubscribe();
				reject(
					new Error(
						`No sysex reply for param ${param} within ${timeoutMs}ms (cmd=${cmd})`,
					),
				);
			}, timeoutMs);

			const unsubscribe = this.onSysex((msg) => {
				if (msg.param !== param || settled) return;
				settled = true;
				clearTimeout(timer);
				unsubscribe();
				resolve(msg);
			});

			try {
				this.output.send(encode(cmd, param, data));
			} catch (e) {
				settled = true;
				clearTimeout(timer);
				unsubscribe();
				reject(e);
			}
		});
	}
}

/**
 * Request Web MIDI access (with sysex) and return the first connected
 * input+output pair whose name contains `portSubstring`. Throws with a
 * descriptive message if permission is denied, the API isn't supported, or
 * no matching device is found - callers should catch and surface this to
 * the user rather than silently failing.
 * @param {string} [portSubstring]
 * @returns {Promise<Device>}
 */
export async function connect(portSubstring = DEFAULT_PORT_SUBSTRING) {
	if (!isSupported()) {
		throw new Error(
			"Web MIDI is not available in this browser. Use Chrome or Edge - " +
				"Firefox does not implement Web MIDI by default, and Safari's " +
				"support is partial.",
		);
	}

	let access;
	try {
		access = await navigator.requestMIDIAccess({ sysex: true });
	} catch (e) {
		throw new Error(
			`Web MIDI access was denied or failed: ${e.message}. This page must ` +
				"be served over http(s):// or from localhost - opening it via a " +
				"file:// URL can block sysex permission in some Chromium versions.",
		);
	}

	const input = findPort(access.inputs, portSubstring);
	const output = findPort(access.outputs, portSubstring);
	if (!input || !output) {
		const available = [...access.outputs.values()].map((p) => p.name).join(", ") || "(none)";
		throw new Error(
			`No MIDI device matching "${portSubstring}" found. ` +
				`Available output ports: ${available}. Is the device connected?`,
		);
	}

	if (input.state !== "connected") {
		await input.open();
	}
	if (output.state !== "connected") {
		await output.open();
	}

	return new Device(input, output);
}

/**
 * @param {MIDIInputMap|MIDIOutputMap} portMap
 * @param {string} substring
 */
function findPort(portMap, substring) {
	const needle = substring.toLowerCase();
	for (const port of portMap.values()) {
		if ((port.name ?? "").toLowerCase().includes(needle)) {
			return port;
		}
	}
	return null;
}

export { Param };
