// Web MIDI with sysex access (`{sysex: true}`) is Chrome/Edge only as of
// this writing - Firefox doesn't implement it by default, Safari's
// support is partial. Check isSupported() before anything else.

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

export function isSupported() {
	return typeof navigator !== "undefined" && "requestMIDIAccess" in navigator;
}

// A connected device's input+output MIDI port pair, plus the raw
// request/response sysex plumbing. One instance per connected device.
export class Device {
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

	get isAlive() {
		return this._alive;
	}

	// Fires at most once per Device instance, the first time either the
	// native port statechange or the heartbeat declares the connection gone.
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
		if (!data || data.length === 0 || data[0] !== 0xf0) return;

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

	// Most callers should use request() instead; this is for observing
	// traffic without driving a specific request (e.g. live position).
	onSysex(listener) {
		this._listeners.push(listener);
		return () => {
			this._listeners = this._listeners.filter((l) => l !== listener);
		};
	}

	// Resolves with the first reply whose `param` matches; rejects on
	// timeout. Concurrent requests for the *same* param are not correlated
	// beyond "first matching reply wins" - this protocol has no per-message
	// request ID, only positional ordering on a single pipe (see sysex.h) -
	// so callers needing strict ordering should await each request first.
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
				// WEBUI_PUSH shares a param with some request/response pairs
				// (e.g. VMAP_CURR_POS - see live-position.js) but is never a
				// reply to this request; without this check a request could
				// resolve against an unrelated unsolicited push for a
				// different encoder instead of timing out or getting its own
				// real reply.
				if (msg.param !== param || msg.cmd === Cmd.WEBUI_PUSH || settled) return;
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
