// Tests for the update orchestration.
//
//   deno test --allow-read webui/js/firmware-update.test.js
//
// The version comparison decides whether a user is offered an update at all,
// and the step machine decides what they are told when one goes wrong. Both
// are pure enough to test without a device.

import { assertEquals, assertRejects } from "@std/assert";
import {
	compareVersions,
	checkForUpdate,
	runUpdate,
	watchForBootloader,
	findBootloader,
	probeFlash,
	identifyFlash,
	digestImage,
	Step,
	STEP_ORDER,
	STEP_LABELS,
} from "./firmware-update.js";
import { FakeDfuDevice } from "../dfu/fake-device.js";
import { parseIntelHex as parseIntelHexForTest } from "../dfu/intel-hex.js";

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Versions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Deno.test("versions compare by number, not by string", () => {
	// The string comparison trap: "0.10.0" sorts before "0.9.0" as text.
	assertEquals(compareVersions("0.9.0", "0.10.0"), -1);
	assertEquals(compareVersions("0.10.0", "0.9.0"), 1);
	assertEquals(compareVersions("1.0.0", "1.0.0"), 0);
});

Deno.test("a leading v is ignored", () => {
	assertEquals(compareVersions("v1.2.3", "1.2.3"), 0);
});

Deno.test("missing components count as zero", () => {
	assertEquals(compareVersions("1.2", "1.2.0"), 0);
	assertEquals(compareVersions("1.2", "1.2.1"), -1);
});

Deno.test("an update is offered when the published version is newer", () => {
	const result = checkForUpdate("0.1.0", { version: "0.2.0" });
	assertEquals(result.available, true);
	assertEquals(result.version, "0.2.0");
});

Deno.test("no update is offered when the versions match", () => {
	assertEquals(checkForUpdate("0.1.0", { version: "0.1.0" }).available, false);
});

Deno.test("a device ahead of the release is not offered a downgrade", () => {
	// A development build should not be quietly replaced with an older
	// released one.
	const result = checkForUpdate("0.2.0", { version: "0.1.0" });
	assertEquals(result.available, false);
	assertEquals(result.reason, "device-newer");
});

Deno.test("a missing manifest offers nothing rather than erroring", () => {
	assertEquals(checkForUpdate("0.1.0", null).available, false);
	assertEquals(checkForUpdate("0.1.0", {}).reason, "no-manifest");
});

Deno.test("an unknown device version offers nothing", () => {
	assertEquals(checkForUpdate(null, { version: "9.9.9" }).available, false);
});

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ The step machine ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Deno.test("every step has a label", () => {
	for (const step of STEP_ORDER) {
		assertEquals(typeof STEP_LABELS[step], "string");
	}
});

/*
	One flash page of data, as Intel HEX.

	Emitted in 32-byte records because the length field is a single byte - a
	record cannot carry 512 bytes, and a helper that tries produces a file the
	parser rightly rejects.
*/
function tinyHex(size = 512, fill = 0x5a) {
	const rec = (type, addr, data) => {
		const bytes = [data.length, (addr >> 8) & 0xff, addr & 0xff, type, ...data];
		const sum = bytes.reduce((a, b) => (a + b) & 0xff, 0);
		return ":" + [...bytes, (0x100 - sum) & 0xff]
			.map((b) => b.toString(16).padStart(2, "0")).join("");
	};

	const lines = [];
	for (let at = 0; at < size; at += 32) {
		lines.push(rec(0x00, at, new Array(Math.min(32, size - at)).fill(fill)));
	}
	lines.push(rec(0x01, 0, []));
	return lines.join("\n");
}

/*
	The shape protocol.js actually returns.

	Written out here rather than invented, because the first version of these
	tests used the Python library's field naming and so agreed with a bug in
	the code instead of catching it: a successful flash reported "the device
	came back but did not report a version".
*/
function deviceInfoShape(fwVersion) {
	return {
		fwVersion,
		numEncoders: 16,
		numBanks: 4,
		numVmapsPerEncoder: 2,
		numSideSwitches: 6,
	};
}

function harness(overrides = {}) {
	const device = new FakeDfuDevice();
	const steps = [];

	// The device reads back whatever was written, so verify passes.
	const written = [];
	const original = device.controlTransferOut.bind(device);
	device.controlTransferOut = async (setup, data) => {
		const bytes = new Uint8Array(data);
		if (bytes.length > 6) written.push(bytes.slice(64, bytes.length - 16));
		return original(setup, data);
	};
	device.opened = true;
	device.open = async () => {};
	device.selectConfiguration = async () => {};
	device.claimInterface = async () => {};

	const readMemory = async () => {
		const all = new Uint8Array(written.reduce((n, w) => n + w.length, 0));
		let at = 0;
		for (const w of written) { all.set(w, at); at += w.length; }
		return { data: new DataView(all.buffer) };
	};

	return {
		device,
		steps,
		readMemory,
		options: {
			hexText: tinyHex(),
			enterBootloader: async () => {},
			reconnect: async () => deviceInfoShape("0.2.0"),
			requestDevice: async () => device,
			onStep: (step, state, detail) => steps.push([step, state, detail]),
			waitForDeviceMs: 50,
			...overrides,
		},
	};
}

Deno.test("a failing step is reported as the step that failed", async () => {
	const h = harness({
		enterBootloader: async () => { throw new Error("device did not respond"); },
	});

	await assertRejects(() => runUpdate(h.options), Error, "device did not respond");

	const failed = h.steps.filter(([, state]) => state === "failed");
	assertEquals(failed.length, 1);
	assertEquals(failed[0][0], Step.BOOTLOADER);
});

Deno.test("later steps do not run once one has failed", async () => {
	const h = harness({
		enterBootloader: async () => { throw new Error("nope"); },
	});

	await assertRejects(() => runUpdate(h.options));

	// Nothing beyond the first step should have been attempted - carrying on
	// after a failure is how a half-written image happens.
	const touched = new Set(h.steps.map(([step]) => step));
	assertEquals(touched.has(Step.ERASE), false);
	assertEquals(touched.has(Step.WRITE), false);
});

Deno.test("a device that is not ours is refused before anything is written", async () => {
	const wrong = new FakeDfuDevice({ productId: 0x2ff4 });
	wrong.opened = true;
	wrong.open = async () => {};
	wrong.selectConfiguration = async () => {};
	wrong.claimInterface = async () => {};

	const h = harness({ requestDevice: async () => wrong });

	await assertRejects(() => runUpdate(h.options), Error, "not a NEON_SAMURAI");

	// Critically: nothing was erased.
	assertEquals(wrong.commands().some((c) => c.data[0] === 0x04 && c.data[1] === 0x00), false);
});

Deno.test("refusing to select a device is a clean failure", async () => {
	const h = harness({ requestDevice: async () => null });
	await assertRejects(() => runUpdate(h.options), Error, "no bootloader device");
});

Deno.test("a mismatch on read-back fails rather than reporting success", async () => {
	// The whole point of the verify step. If the device says something
	// different came back, the update must not be called complete.
	const h = harness();
	h.device.controlTransferIn = async (setup) => {
		if (setup.request === 3 /* GETSTATUS */) {
			return { status: "ok", data: new DataView(new Uint8Array([0, 0, 0, 0, 2, 0]).buffer) };
		}
		return { status: "ok", data: new DataView(new Uint8Array(512).fill(0x00).buffer) };
	};

	await assertRejects(() => runUpdate(h.options), Error, "differs");
});

Deno.test("the version comes from the field protocol.js actually returns", async () => {
	// Guards the naming mismatch directly: protocol.js returns fwVersion, the
	// Python library returns fw_version, and reading the wrong one turns a
	// successful update into a reported failure.
	const h = harness();
	const version = await runUpdate(h.options);
	assertEquals(version, "0.2.0");
});

Deno.test("a device that reports no version at all is a failure", async () => {
	const h = harness({ reconnect: async () => ({}) });
	await assertRejects(() => runUpdate(h.options), Error, "did not report a version");
});

Deno.test("forcing offers the firmware whatever the device runs", () => {
	// The testing path. Same version, older version, no version at all.
	for (const current of ["0.1.0", "9.9.9", null]) {
		const r = checkForUpdate(current, { version: "0.1.0" }, { force: true });
		assertEquals(r.available, true);
		assertEquals(r.forced, true);
	}
});

Deno.test("forcing still offers nothing when there is no firmware", () => {
	// Nothing to flash is nothing to flash, however much it is forced.
	assertEquals(checkForUpdate("0.1.0", null, { force: true }).available, false);
	assertEquals(checkForUpdate("0.1.0", {}, { force: true }).available, false);
});

Deno.test("forcing is off unless asked for", () => {
	assertEquals(checkForUpdate("0.1.0", { version: "0.1.0" }).available, false);
});

Deno.test("a device already in the bootloader skips entering it", async () => {
	// Nothing to send the command to, and asking a device to enter a mode it
	// is already in just fails.
	const h = harness({
		skipBootloader: true,
		enterBootloader: async () => { throw new Error("should not be called"); },
	});

	const version = await runUpdate(h.options);
	assertEquals(version, "0.2.0");

	const step = h.steps.find(([s, state]) => s === Step.BOOTLOADER && state === "done");
	assertEquals(step[2], "already in bootloader mode");
});

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~ Auto-detection ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Stand in for navigator.usb, which does not exist under Deno.
function fakeUsb(devices = []) {
	const listeners = {};
	return {
		devices,
		getDevices: async () => devices,
		addEventListener: (type, fn) => {
			(listeners[type] ??= []).push(fn);
		},
		removeEventListener: (type, fn) => {
			listeners[type] = (listeners[type] ?? []).filter((f) => f !== fn);
		},
		emit: (type) => (listeners[type] ?? []).forEach((f) => f()),
		listenerCount: (type) => (listeners[type] ?? []).length,
	};
}

/*
	Run fn with navigator.usb standing in for the real thing.

	Async on purpose: a synchronous try/finally around an async callback tears
	the stub down before the awaited body has run, which shows up as the
	watcher seeing no devices at all.
*/
async function withUsb(usb, fn) {
	const had = "usb" in navigator;
	Object.defineProperty(navigator, "usb", { value: usb, configurable: true });
	try {
		return await fn();
	} finally {
		if (!had) delete navigator.usb;
	}
}

Deno.test("an attached bootloader is noticed without any user action", async () => {
	const usb = fakeUsb([new FakeDfuDevice()]);
	let seen = null;

	await withUsb(usb, async () => {
		watchForBootloader({ onPresent: (d) => (seen = d) });
		// The watcher checks on start, which is asynchronous.
		await new Promise((r) => setTimeout(r, 0));
	});

	assertEquals(seen !== null, true);
});

Deno.test("a bootloader appearing later is noticed", async () => {
	const usb = fakeUsb([]);
	let present = 0;

	await withUsb(usb, async () => {
		watchForBootloader({ onPresent: () => present++ });
		await new Promise((r) => setTimeout(r, 0));

		usb.devices.push(new FakeDfuDevice());
		usb.emit("connect");
		await new Promise((r) => setTimeout(r, 0));
	});

	assertEquals(present, 1);
});

Deno.test("a device that is not ours does not count as a bootloader", async () => {
	const usb = fakeUsb([new FakeDfuDevice({ productId: 0x2ff4 })]);
	let gone = 0;

	await withUsb(usb, async () => {
		watchForBootloader({ onGone: () => gone++ });
		await new Promise((r) => setTimeout(r, 0));
	});

	assertEquals(gone, 1);
});

Deno.test("the watcher can be stopped", async () => {
	const usb = fakeUsb([]);

	await withUsb(usb, async () => {
		const stop = watchForBootloader({});
		await new Promise((r) => setTimeout(r, 0));
		assertEquals(usb.listenerCount("connect"), 1);
		stop();
		assertEquals(usb.listenerCount("connect"), 0);
	});
});

Deno.test("watching where WebUSB does not exist is harmless", async () => {
	// Firefox and Safari. The editor still works; it simply cannot offer this.
	const stop = await withUsb(undefined, () => watchForBootloader({}));
	assertEquals(typeof stop, "function");
	stop();
});

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~ Probing the flash ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Deno.test("an erased device reads as blank", async () => {
	// Every byte 0xFF. Saying "blank" rather than "some unknown firmware"
	// matters: a blank device is one that badly needs flashing.
	const dev = new FakeDfuDevice();
	const probe = await probeFlash(dev, 0x1000);

	assertEquals(probe.blank, true);
	assertEquals(probe.usedBytes, 0);
	assertEquals(probe.digest, null);
});

Deno.test("trailing erased space is not counted as content", async () => {
	const dev = new FakeDfuDevice();
	dev.memory.set([1, 2, 3, 4], 0);

	const probe = await probeFlash(dev, 0x1000);
	assertEquals(probe.blank, false);
	assertEquals(probe.usedBytes, 4);
});

Deno.test("a device holding the offered firmware is recognised", async () => {
	const dev = new FakeDfuDevice();
	const hex = tinyHex(512, 0x5a);

	// Put exactly that image on the device.
	dev.memory.fill(0xff);
	dev.memory.set(new Uint8Array(512).fill(0x5a), 0);

	const result = await identifyFlash(dev, hex, 0x1000);
	assertEquals(result.state, "matches");
	assertEquals(result.matches, true);
});

Deno.test("a device holding something else is not mistaken for a match", async () => {
	const dev = new FakeDfuDevice();
	dev.memory.fill(0xff);
	dev.memory.set(new Uint8Array(512).fill(0x11), 0);

	const result = await identifyFlash(dev, tinyHex(512, 0x5a), 0x1000);
	assertEquals(result.state, "different");
	assertEquals(result.matches, false);
});

Deno.test("a blank device is reported as blank, not as a mismatch", async () => {
	const dev = new FakeDfuDevice();
	const result = await identifyFlash(dev, tinyHex(512, 0x5a), 0x1000);
	assertEquals(result.state, "blank");
});

Deno.test("the digest of an image matches the digest read back from flash", async () => {
	// The two must trim erased space the same way, or a device holding exactly
	// the offered image would still look different.
	const dev = new FakeDfuDevice();
	dev.memory.fill(0xff);
	dev.memory.set(new Uint8Array(512).fill(0x5a), 0);

	const probe = await probeFlash(dev, 0x1000);
	const expected = await digestImage(parseIntelHexForTest(tinyHex(512, 0x5a)));
	assertEquals(probe.digest, expected);
});
