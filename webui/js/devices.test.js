// Tests for detection and the connection state machine.
//
//   deno test --allow-read webui/js/devices.test.js

import { assertEquals, assertRejects } from "@std/assert";
import { DeviceRegistry, DeviceState } from "./devices.js";

class FakeAccess {
	constructor() {
		this.onstatechange = null;
	}
}

function entry(id, name = "NEON_SAMURAI") {
	return { id, name, input: { id: `${id}-in` }, output: { id } };
}

/**
 * A registry with detection and opening stubbed.
 * @param p.entries  what detection finds
 * @param p.open     stands in for midi.openDevice
 */
function registry(p = {}) {
	const opened = [];
	const destroyed = [];

	const reg = new DeviceRegistry({
		requestAccess: async () => new FakeAccess(),
		listDevices: () => p.entries ?? [entry("a")],
		openDevice: async (e) => {
			opened.push(e.id);
			if (p.open) return p.open(e);
			return { id: e.id, destroy: () => destroyed.push(e.id) };
		},
	});

	return { reg, opened, destroyed };
}

function busy() {
	const e = new Error("Another application is using it.");
	e.name = "PortBusyError";
	return e;
}

Deno.test("detection opens nothing", async () => {
	const { reg, opened } = registry();
	await reg.start();

	assertEquals(reg.list().map((d) => d.state), [DeviceState.DETECTED]);
	assertEquals(opened, [], "enumerating must not open a port");
	assertEquals(reg.connected, false);
});

Deno.test("denied permission is recorded, not thrown", async () => {
	// An editor that cannot see devices should still load.
	const reg = new DeviceRegistry({
		requestAccess: async () => { throw new Error("denied"); },
	});

	assertEquals(await reg.start(), false);
	assertEquals(reg.error, "denied");
});

Deno.test("connecting opens the port and marks the device connected", async () => {
	const { reg, opened } = registry();
	await reg.start();
	await reg.connect("a");

	assertEquals(opened, ["a"]);
	assertEquals(reg.list()[0].state, DeviceState.CONNECTED);
	assertEquals(reg.connected, true);
});

Deno.test("a busy port is its own state, not a failure", async () => {
	// The device is there; something else is holding it. Saying "not found"
	// would send the user looking for a cable problem.
	const { reg } = registry({ open: () => { throw busy(); } });
	await reg.start();

	await assertRejects(() => reg.connect("a"));
	assertEquals(reg.list()[0].state, DeviceState.BUSY);
	assertEquals(reg.connected, false);
});

Deno.test("any other open failure is distinguishable from busy", async () => {
	const { reg } = registry({ open: () => { throw new Error("boom"); } });
	await reg.start();

	await assertRejects(() => reg.connect("a"));
	assertEquals(reg.list()[0].state, DeviceState.FAILED);
});

Deno.test("connecting a second device releases the first", async () => {
	const { reg, destroyed } = registry({ entries: [entry("a"), entry("b")] });
	await reg.start();
	await reg.connect("a");
	await reg.connect("b");

	assertEquals(destroyed, ["a"], "the old connection must be closed");
	assertEquals(reg.list().map((d) => d.state), [DeviceState.DETECTED, DeviceState.CONNECTED]);
});

Deno.test("unplugging the connected device drops the connection", async () => {
	let entries = [entry("a")];
	const destroyed = [];
	const reg = new DeviceRegistry({
		requestAccess: async () => new FakeAccess(),
		listDevices: () => entries,
		openDevice: async (e) => ({ destroy: () => destroyed.push(e.id) }),
	});

	await reg.start();
	await reg.connect("a");

	entries = [];
	reg.rescan();

	assertEquals(reg.connected, false);
	assertEquals(destroyed, ["a"]);
	assertEquals(reg.list(), []);
});

Deno.test("disconnecting returns the device to detected", async () => {
	const { reg } = registry();
	await reg.start();
	await reg.connect("a");
	await reg.disconnect();

	assertEquals(reg.list()[0].state, DeviceState.DETECTED);
	assertEquals(reg.connected, false);
});

Deno.test("auto-connect takes the only device", async () => {
	const { reg, opened } = registry();
	await reg.start();

	await reg.autoConnect();
	assertEquals(opened, ["a"]);
});

Deno.test("auto-connect declines to choose between several", async () => {
	// With more than one attached there is no basis for picking, so it waits.
	const { reg, opened } = registry({ entries: [entry("a"), entry("b")] });
	await reg.start();

	await reg.autoConnect();
	assertEquals(opened, []);
	assertEquals(reg.connected, false);
});

Deno.test("auto-connect does not disturb an existing connection", async () => {
	const { reg, opened } = registry();
	await reg.start();
	await reg.connect("a");

	await reg.autoConnect();
	assertEquals(opened, ["a"], "should not have opened it twice");
});

Deno.test("auto-connect swallows a busy port rather than interrupting", async () => {
	const { reg } = registry({ open: () => { throw busy(); } });
	await reg.start();

	await reg.autoConnect();
	assertEquals(reg.list()[0].state, DeviceState.BUSY);
});

Deno.test("changes are announced to listeners", async () => {
	const { reg } = registry();
	let seen = 0;
	reg.onChange(() => seen++);

	await reg.start();
	await reg.connect("a");

	assertEquals(seen > 0, true);
});
