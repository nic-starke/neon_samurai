// Driving a firmware update from the editor.
//
// The steps are deliberately explicit rather than one long function: the UI
// shows them as a list, and a failure has to say which one it failed at.
// Anything that touches MIDI is passed in by the caller, because this module
// must not own that connection - the editor has to *release* the MIDI port
// before the device re-enumerates as a DFU device, or the browser cannot
// claim it.

import * as dfu from "../dfu/xmega-dfu.js";
import { parseFirmwareInfo, LIKELY_WITHIN } from "./fwinfo.js";
import { parseIntelHex } from "../dfu/intel-hex.js";

// Where the deployed site keeps the firmware it offers. Same origin on
// purpose: GitHub's release asset downloads send no CORS headers, so the
// browser cannot read them however the request is framed. The API does, which
// is why a version can be checked against GitHub but an image cannot be
// fetched from it.
export const MANIFEST_URL = "../firmware/index.json";

export const Step = Object.freeze({
	BOOTLOADER: "bootloader",
	WAITING: "waiting",
	CONNECT: "connect",
	ERASE: "erase",
	WRITE: "write",
	VERIFY: "verify",
	RESTART: "restart",
	RECONNECT: "reconnect",
});

export const STEP_LABELS = Object.freeze({
	[Step.BOOTLOADER]: "Enter bootloader mode",
	[Step.WAITING]: "Wait for the device",
	[Step.CONNECT]: "Connect to the bootloader",
	[Step.ERASE]: "Erase the old firmware",
	[Step.WRITE]: "Write the new firmware",
	[Step.VERIFY]: "Verify what was written",
	[Step.RESTART]: "Restart the device",
	[Step.RECONNECT]: "Reconnect and check the version",
});

export const STEP_ORDER = Object.freeze([
	Step.BOOTLOADER, Step.WAITING, Step.CONNECT, Step.ERASE,
	Step.WRITE, Step.VERIFY, Step.RESTART, Step.RECONNECT,
]);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Versions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * Compare two dotted versions.
 * @returns -1, 0 or 1, as a sort comparator would.
 */
export function compareVersions(a, b) {
	const parse = (v) =>
		String(v ?? "")
			.trim()
			.replace(/^v/i, "")
			.split(".")
			.map((n) => Number.parseInt(n, 10));

	const left = parse(a);
	const right = parse(b);

	for (let i = 0; i < Math.max(left.length, right.length); i++) {
		const l = Number.isFinite(left[i]) ? left[i] : 0;
		const r = Number.isFinite(right[i]) ? right[i] : 0;
		if (l !== r) return l < r ? -1 : 1;
	}

	return 0;
}

/**
 * Decide what to tell the user about updates.
 *
 * An unreadable or missing manifest is not an error worth shouting about -
 * the editor works perfectly well without an update available - so it comes
 * back as simply having nothing to offer.
 */
export function checkForUpdate(currentVersion, manifest, { force = false } = {}) {
	if (!manifest || !manifest.version) {
		return { available: false, reason: "no-manifest" };
	}

	// Testing affordance: offer the firmware whatever the device is running,
	// so the flow can be exercised without building a higher version each
	// time. Reached only from an explicit URL flag, never by default - see
	// isForced() in live-twin.js.
	if (force) {
		return { available: true, version: manifest.version, manifest, forced: true };
	}

	if (!currentVersion) {
		return { available: false, reason: "unknown-device-version" };
	}

	const order = compareVersions(currentVersion, manifest.version);

	if (order < 0) {
		return { available: true, version: manifest.version, manifest };
	}

	// A device ahead of the published firmware is a development build, not a
	// problem - say it is current rather than offering a downgrade.
	return { available: false, reason: order > 0 ? "device-newer" : "up-to-date" };
}

/** Fetch the manifest describing the firmware this site offers. */
export async function fetchManifest(url = MANIFEST_URL) {
	try {
		const response = await fetch(url, { cache: "no-store" });
		if (!response.ok) return null;
		return await response.json();
	} catch {
		return null;
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~ Reading the flash ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*
	The bootloader can read memory back as well as write it - CMD_GROUP_UPLOAD,
	the same thing the verify step uses. That is the only way to learn anything
	about a device sitting in DFU, because it has no MIDI interface to ask.

	What can be recovered is limited. The firmware carries its version as three
	byte constants in a struct, not as text, so there is no string to search
	for. Comparing a digest against the image being offered is both simpler and
	more honest: it answers "is this the firmware I have" exactly, and "which
	other firmware is it" not at all.
*/

// How much to read when probing. Enough to cover an image several times the
// present size without reading the whole 128 KB, which is slow over control
// transfers and tells us nothing extra - everything past the image is erased.
export const PROBE_LENGTH = 0x8000;

/** Erased flash reads as 0xFF, so trailing 0xFF is absence, not content. */
function usedLength(bytes) {
	let end = bytes.length;
	while (end > 0 && bytes[end - 1] === 0xff) end--;
	return end;
}

async function sha256(bytes) {
	const digest = await crypto.subtle.digest("SHA-256", bytes);
	return [...new Uint8Array(digest)].map((b) => b.toString(16).padStart(2, "0")).join("");
}

/**
 * Read the start of the application section and describe what is there.
 *
 * @returns {{blank: boolean, usedBytes: number, digest: string|null}}
 */
export async function probeFlash(dev, length = PROBE_LENGTH) {
	const bytes = await dfu.readMemory(dev, 0, length - 1);
	const used = usedLength(bytes);

	if (used === 0) return { blank: true, usedBytes: 0, digest: null };

	return {
		blank: false,
		usedBytes: used,
		digest: await sha256(bytes.slice(0, used)),
		// Free: these are bytes we have already read.
		info: parseFirmwareInfo(bytes),
	};
}

/**
 * Ask a device in the bootloader what firmware is on it.
 *
 * Reads only far enough to find the record, rather than the whole probe
 * length, because this runs on detection - before the user has asked for
 * anything - and a 32 KB read over control transfers is slow enough to notice.
 *
 * Returns null when there is nothing to report: firmware built before the
 * record existed, someone else's firmware, or a blank device. That is an
 * answer, not a failure, so this never throws.
 */
export async function inspectBootloader(dev) {
	const opened = dev.opened;

	try {
		if (!opened) await dev.open();
		if (dev.configuration === null) await dev.selectConfiguration(1);
		await dev.claimInterface(0);

		const status = dfu.parseStatus((await dfu.getStatus(dev)).data);
		if (status.bState === dfu.bState.dfuERROR) await dfu.clearStatus(dev);

		return parseFirmwareInfo(await dfu.readMemory(dev, 0, LIKELY_WITHIN - 1));
	} catch {
		return null;
	} finally {
		// Leave the device as it was found, so the update can claim it.
		try {
			await dev.releaseInterface(0);
			if (!opened) await dev.close();
		} catch {
			/* the device may have gone; nothing to release */
		}
	}
}

/**
 * Digest an image the same way probeFlash digests the device, so the two can
 * be compared.
 */
export async function digestImage(image) {
	const used = usedLength(image.data);
	return used === 0 ? null : await sha256(image.data.slice(0, used));
}

/**
 * Say what is on a device, relative to the firmware being offered.
 *
 * A device that matches is not one to reflash; a blank one badly needs it;
 * anything else is some other build, which is worth saying plainly rather
 * than guessing at.
 */
export async function identifyFlash(dev, hexText, length = PROBE_LENGTH) {
	const probe = await probeFlash(dev, length);

	if (probe.blank) return { ...probe, matches: false, state: "blank" };

	const expected = await digestImage(parseIntelHex(hexText));
	const matches = expected !== null && probe.digest === expected;

	return { ...probe, matches, state: matches ? "matches" : "different" };
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ The update ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

/**
 * Find the bootloader without prompting, if this origin has already been
 * granted access to it. Only the first update needs the picker.
 */
async function findGrantedDevice() {
	if (!navigator.usb?.getDevices) return null;

	const devices = await navigator.usb.getDevices();
	return devices.find((d) => dfu.identify(d) !== null) ?? null;
}

/**
 * Whether a bootloader this origin may talk to is connected right now.
 *
 * Needs no user gesture, and so can be called on load or from an event.
 */
export async function findBootloader() {
	return findGrantedDevice();
}

/**
 * Watch for the bootloader appearing and disappearing.
 *
 * WebUSB raises connect and disconnect for devices this origin has been
 * granted, without a gesture, so once the user has picked the bootloader once
 * the editor notices it every time afterwards - including a device put into
 * DFU by the encoder gesture while the page sits open.
 *
 * The first grant cannot be automated: requestDevice() requires a gesture,
 * and until it has been called the origin cannot see the device at all. So
 * this is silent until the user has been through one update.
 *
 * @returns A function that stops watching.
 */
export function watchForBootloader({ onPresent, onGone }) {
	if (!navigator.usb) return () => {};

	const check = async () => {
		const dev = await findGrantedDevice();
		if (dev) onPresent?.(dev);
		else onGone?.();
	};

	// The events carry the device, but re-checking keeps one path for "what is
	// attached now" rather than trusting an event for a device we may not be
	// permitted to see.
	const onConnect = () => check();
	const onDisconnect = () => check();

	navigator.usb.addEventListener("connect", onConnect);
	navigator.usb.addEventListener("disconnect", onDisconnect);

	// Something may already be attached when the page loads.
	check();

	return () => {
		navigator.usb.removeEventListener("connect", onConnect);
		navigator.usb.removeEventListener("disconnect", onDisconnect);
	};
}

/**
 * Wait for the bootloader to turn up among the devices already granted.
 * Resolves null if it does not, which means the picker is needed.
 *
 * Kept short. A device this origin has been granted appears within a second
 * or two of re-enumerating; one it has not will never appear no matter how
 * long the wait, and every second spent here is a second the user watches a
 * step that is not going to succeed.
 */
async function waitForGrantedDevice(timeoutMs) {
	const deadline = Date.now() + timeoutMs;

	while (Date.now() < deadline) {
		const dev = await findGrantedDevice();
		if (dev) return dev;
		await sleep(250);
	}

	return null;
}

/**
 * Run a firmware update.
 *
 * @param options.hexText        The .hex to write.
 * @param options.enterBootloader  Called to put the device into DFU. Must also
 *   release the MIDI port - the browser cannot claim the DFU interface while
 *   ALSA or the Web MIDI stack still holds the device.
 * @param options.reconnect      Called after the restart. Should return the
 *   device info, so the new version can be confirmed.
 * @param options.requestDevice  Called when the bootloader has not been
 *   granted to this origin yet. Must run inside a user gesture, so the UI
 *   supplies a button rather than this module calling the picker itself.
 * @param options.onStep         (step, state, detail) as each step changes.
 * @param options.onProgress     (done, total) during the write.
 */
export async function runUpdate({
	hexText,
	enterBootloader,
	reconnect,
	requestDevice,
	onStep = () => {},
	onProgress = () => {},
	skipBootloader = false,
	waitForDeviceMs = 5000,
}) {
	const image = parseIntelHex(hexText);
	let dev = null;

	/*
		A device already sitting in the bootloader - because a previous attempt
		stopped part way, or somebody used the gesture - has no MIDI interface
		to send the command to. Asking it to enter a mode it is already in just
		fails, so that step is skipped instead.
	*/
	const alreadyInBootloader = skipBootloader || (await findGrantedDevice()) !== null;

	const begin = (step) => onStep(step, "active");
	const done = (step, detail) => onStep(step, "done", detail);
	const fail = (step, error) => onStep(step, "failed", error.message);

	const run = async (step, fn) => {
		begin(step);
		try {
			const detail = await fn();
			done(step, detail);
			return detail;
		} catch (e) {
			fail(step, e);
			throw e;
		}
	};

	try {
		await run(Step.BOOTLOADER, async () => {
			if (alreadyInBootloader) return "already in bootloader mode";

			await enterBootloader();
			return "device left the MIDI bus";
		});

		// Already-granted devices are found silently; only the first update
		// needs the picker, which the next step handles.
		dev = await run(Step.WAITING, async () => {
			dev = await waitForGrantedDevice(waitForDeviceMs);
			return dev ? "found" : "needs selecting";
		}).then(() => dev);

		await run(Step.CONNECT, async () => {
			// Only prompt if this origin has not been granted the device
			// before. After the first update it is silent.
			if (!dev) {
				dev = await requestDevice();
			}

			if (!dev) throw new Error("no bootloader device was selected");

			const info = dfu.identify(dev);
			if (!info) throw new Error("that device is not a NEON_SAMURAI bootloader");

			if (!dev.opened) await dev.open();
			if (dev.configuration === null) await dev.selectConfiguration(1);
			await dev.claimInterface(0);

			if (image.end >= info.appSize) {
				throw new Error("the firmware image is too large for the application section");
			}

			// A previous failure latches dfuERROR, and everything after it
			// fails until it is cleared.
			const status = dfu.parseStatus((await dfu.getStatus(dev)).data);
			if (status.bState === dfu.bState.dfuERROR) await dfu.clearStatus(dev);

			return info.name;
		});

		await run(Step.ERASE, async () => {
			// Not optional: with the security bits set the bootloader accepts
			// nothing else until an erase has run.
			await dfu.chipErase(dev);

			let status;
			do {
				status = dfu.parseStatus((await dfu.getStatus(dev)).data);
			} while (status.bState === dfu.bState.dfuDNBUSY);

			if (status.bStatus !== dfu.bStatus.OK) {
				throw new Error(`the device reported status ${status.bStatus} while erasing`);
			}
		});

		await run(Step.WRITE, async () => {
			await dfu.writeMemory(dev, image.start, image.end, image.data, false, onProgress);
			return `${image.data.length} bytes`;
		});

		await run(Step.VERIFY, async () => {
			const read = await dfu.readMemory(dev, image.start, image.end);
			const got = new Uint8Array(read.data ? read.data.buffer : read);

			for (let i = 0; i < image.data.length; i++) {
				if (got[i] !== image.data[i]) {
					throw new Error(
						`what was read back differs at 0x${i.toString(16)} - the write did not take`,
					);
				}
			}

			return `${image.data.length} bytes match`;
		});

		await run(Step.RESTART, async () => {
			await dfu.launch(dev);
			dev = null;
		});

		return await run(Step.RECONNECT, async () => {
			const info = await reconnect();

			// The field name is protocol.js's, not the Python test library's -
			// the two use different conventions for the same value, and mixing
			// them reads as "the device came back but did not report a version"
			// after a perfectly good flash.
			if (!info?.fwVersion) {
				throw new Error("the device came back but did not report a version");
			}

			return info.fwVersion;
		});
	} catch (e) {
		// Leave the bootloader in a state the next attempt can use. The
		// bootloader itself is never written, so retrying is always safe -
		// which is worth the UI being able to say.
		if (dev) {
			try {
				await dfu.clearStatus(dev);
			} catch {
				/* the device may already be gone */
			}
		}
		throw e;
	}
}
