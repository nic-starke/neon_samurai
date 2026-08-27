// Web MIDI access, connection, and sysex request/response correlation.
//
// Requires sysex access ({sysex: true}), which is Chrome/Edge only - check
// isSupported() before anything else.
//
// Liveness is watched two independent ways: each port's native statechange
// event (fires on real unplug/re-enumeration) and a periodic DEVICE_INFO
// ping (catches firmware that has hung while the OS still shows the port).
// The ping is single-flight and suspended during bulk transfers - see
// pauseHeartbeat().

import { decode, encode, Cmd, Param } from "./sysex.js";

const DEFAULT_PORT_SUBSTRING = "SAMURAI";
const DEFAULT_TIMEOUT_MS = 2000;

const HEARTBEAT_INTERVAL_MS = 2000;
const HEARTBEAT_TIMEOUT_MS = 1500;
const HEARTBEAT_MAX_MISSES = 2;

export function isSupported() {
	return typeof navigator !== "undefined" && "requestMIDIAccess" in navigator;
}

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
		this._heartbeatInFlight = false;
		this._pauseDepth = 0;

		this._onMessageBound = (e) => this._onMessage(e);
		this._onStateChangeBound = (e) => this._onStateChange(e);
		this.input.addEventListener("midimessage", this._onMessageBound);
		this.input.addEventListener("statechange", this._onStateChangeBound);
		this.output.addEventListener("statechange", this._onStateChangeBound);

		this._startHeartbeat();
	}

	get name() {
		return this.output.name ?? this.input.name ?? "unknown device";
	}

	get isAlive() {
		return this._alive;
	}

	onDisconnect(listener) {
		this._disconnectListeners.push(listener);
		return () => {
			this._disconnectListeners = this._disconnectListeners.filter((l) => l !== listener);
		};
	}

	// The MIDI port objects survive a reconnect, so a Device that cleared
	// only its timer would keep decoding traffic for the life of the page.
	destroy() {
		if (!this._alive && !this._onMessageBound) return;
		this._alive = false;
		if (this._heartbeatTimer) clearInterval(this._heartbeatTimer);
		this._heartbeatTimer = null;
		this.input.removeEventListener("midimessage", this._onMessageBound);
		this.input.removeEventListener("statechange", this._onStateChangeBound);
		this.output.removeEventListener("statechange", this._onStateChangeBound);
		this._onMessageBound = null;
		this._onStateChangeBound = null;
		this._listeners = [];
		this._disconnectListeners = [];
	}

	// Suspends the ping around a bulk transfer, where it would time out from
	// congestion rather than death. Nests; statechange detection is unaffected.
	pauseHeartbeat() {
		this._pauseDepth++;
	}

	resumeHeartbeat() {
		this._pauseDepth = Math.max(0, this._pauseDepth - 1);
		this._heartbeatMisses = 0;
	}

	_onStateChange(event) {
		if (!this._alive) return;
		const port = event.port;
		if (port.state === "disconnected") {
			this._declareDead(`${port.name ?? "port"} disconnected`);
		}
	}

	// Single-flight: overlapping pings would race on _heartbeatMisses, where a
	// late success can clear a miss it never answered for.
	_startHeartbeat() {
		this._heartbeatTimer = setInterval(async () => {
			if (!this._alive || this._pauseDepth > 0 || this._heartbeatInFlight) return;
			this._heartbeatInFlight = true;
			try {
				await this.request(Cmd.GET, Param.DEVICE_INFO, [], HEARTBEAT_TIMEOUT_MS);
				this._heartbeatMisses = 0;
			} catch {
				if (!this._alive || this._pauseDepth > 0) return;
				this._heartbeatMisses++;
				if (this._heartbeatMisses >= HEARTBEAT_MAX_MISSES) {
					this._declareDead(`no response to ${HEARTBEAT_MAX_MISSES} consecutive heartbeat pings`);
				}
			} finally {
				this._heartbeatInFlight = false;
			}
		}, HEARTBEAT_INTERVAL_MS);
	}

	_declareDead(reason) {
		if (!this._alive) return;
		this._alive = false;
		if (this._heartbeatTimer) clearInterval(this._heartbeatTimer);
		this._heartbeatTimer = null;
		for (const listener of this._disconnectListeners) {
			listener(reason);
		}
	}

	_onMessage(event) {
		if (!this._alive) return;
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

	// For observing traffic without driving a request. Most callers want request().
	onSysex(listener) {
		this._listeners.push(listener);
		return () => {
			this._listeners = this._listeners.filter((l) => l !== listener);
		};
	}

	// Resolves with the first reply whose param matches. The protocol has no
	// per-message request ID, so concurrent requests for one param cannot be
	// told apart - callers must await each. WEBUI_PUSH shares params with
	// replies but is never one, so it is excluded.
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
				reject(new Error(`No sysex reply for param ${param} within ${timeoutMs}ms (cmd=${cmd})`));
			}, timeoutMs);

			const unsubscribe = this.onSysex((msg) => {
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

/**
 * Raised when the ports exist but cannot be opened.
 *
 * On Linux a rawmidi node has a single opener, so a DAW or Mixxx holding the
 * device blocks the editor entirely. That is a different situation from the
 * device being absent and has to be said differently - see spec 4.4.
 */
export class PortBusyError extends Error {
	constructor(name) {
		super(
			`Another application is using ${name}. Close it and scan again.`,
		);
		this.name = "PortBusyError";
	}
}

/**
 * Ask for MIDI access.
 *
 * Chrome prompts the first time and remembers the answer for the origin.
 * Nothing here opens a port or sends a byte.
 */
export async function requestAccess() {
	if (!isSupported()) {
		throw new Error(
			"Web MIDI is not available in this browser. Use Chrome or Edge - " +
				"Firefox does not implement Web MIDI by default, and Safari's " +
				"support is partial.",
		);
	}

	try {
		return await navigator.requestMIDIAccess({ sysex: true });
	} catch (e) {
		throw new Error(
			`Web MIDI access was denied or failed: ${e.message}. This page must ` +
				"be served over http(s):// or from localhost - opening it via a " +
				"file:// URL can block sysex permission in some Chromium versions.",
		);
	}
}

/**
 * Every attached device whose ports match, paired input to output.
 *
 * Enumeration only - no port is opened and no MIDI is sent, so this is safe to
 * run on load and on every hot-plug. Identity comes from the output port's id,
 * which is stable for as long as the device is attached.
 *
 * @returns [{id, name, input, output}]
 */
export function listDevices(access, portSubstring = DEFAULT_PORT_SUBSTRING) {
	const needle = portSubstring.toLowerCase();
	const matches = (port) => (port?.name ?? "").toLowerCase().includes(needle);

	const inputs = [...access.inputs.values()].filter(matches);
	const found = [];

	for (const output of access.outputs.values()) {
		if (!matches(output) || output.state !== "connected") continue;

		// Ports are paired by name. Two identical Twisters would be
		// indistinguishable here; telling them apart needs a serial from the
		// firmware, which it does not report yet.
		const input =
			inputs.find((i) => i.name === output.name && i.state === "connected") ??
			inputs.find((i) => i.state === "connected");

		if (input) found.push({ id: output.id, name: output.name, input, output });
	}

	return found;
}

/**
 * Open a detected device's ports and start talking to it.
 *
 * This is the first thing with side effects: the ports become exclusively
 * ours, and Device begins its heartbeat.
 */
export async function openDevice(entry) {
	// port.state is presence; port.connection is whether it is open. Testing
	// the former skips open() for any attached device, which left the port to
	// be opened implicitly by the first send - and hid a busy port until
	// something failed later for a different-looking reason.
	try {
		if (entry.input.connection !== "open") await entry.input.open();
		if (entry.output.connection !== "open") await entry.output.open();
	} catch {
		throw new PortBusyError(entry.name);
	}

	return new Device(entry.input, entry.output);
}

export async function connect(portSubstring = DEFAULT_PORT_SUBSTRING) {
	const access = await requestAccess();
	const [entry] = listDevices(access, portSubstring);

	if (!entry) {
		const available = [...access.outputs.values()].map((p) => p.name).join(", ") || "(none)";
		throw new Error(
			`No MIDI device matching "${portSubstring}" found. ` +
				`Available output ports: ${available}. Is the device connected?`,
		);
	}

	return openDevice(entry);
}

export { Param };
