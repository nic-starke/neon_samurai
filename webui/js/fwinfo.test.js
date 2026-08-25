// Tests for reading the firmware's record of itself.
//
//   deno test --allow-read webui/js/fwinfo.test.js

import { assertEquals } from "@std/assert";
import {
	FWINFO_ID,
	findRecord,
	parseFirmwareInfo,
	readFirmwareInfo,
	LIKELY_WITHIN,
} from "./fwinfo.js";

// Build a record the way the firmware lays it out.
function record({ id = FWINFO_ID, format = 1, version = [0, 1, 0], commit = "" } = {}) {
	const enc = (str, n) => new TextEncoder().encode(str).slice(0, n);
	const buf = new Uint8Array(32);
	buf[0] = format;
	buf.set(enc(id, 16), 1);
	buf.set(enc(commit, 12), 17);
	buf[29] = version[0];
	buf[30] = version[1];
	buf[31] = version[2];
	return buf;
}

function flashWith(rec, at = 0x1fc, size = 0x400) {
	const flash = new Uint8Array(size).fill(0xff);
	flash.set(rec, at);
	return flash;
}

Deno.test("the record is found where the firmware puts it", () => {
	const info = parseFirmwareInfo(flashWith(record({ version: [1, 2, 3] })));
	assertEquals(info.version, "1.2.3");
	assertEquals(info.offset, 0x1fc);
});

Deno.test("the identifier is read back", () => {
	assertEquals(parseFirmwareInfo(flashWith(record())).id, FWINFO_ID);
});

Deno.test("a commit is read, and a dirty tree is flagged", () => {
	const clean = parseFirmwareInfo(flashWith(record({ commit: "ec901494" })));
	assertEquals(clean.commit, "ec901494");
	assertEquals(clean.dirty, false);

	// The firmware appends '+' when the tree had uncommitted changes.
	const dirty = parseFirmwareInfo(flashWith(record({ commit: "ec901494+" })));
	assertEquals(dirty.commit, "ec901494");
	assertEquals(dirty.dirty, true);
});

Deno.test("a build with no git information reports no commit", () => {
	assertEquals(parseFirmwareInfo(flashWith(record({ commit: "" }))).commit, "");
});

Deno.test("flash without a record returns nothing rather than nonsense", () => {
	// Firmware built before this existed. Not an error - it is the answer.
	assertEquals(parseFirmwareInfo(new Uint8Array(0x400).fill(0xff)), null);
});

Deno.test("erased flash is not mistaken for a record", () => {
	assertEquals(findRecord(new Uint8Array(0x1000).fill(0xff)), -1);
});

Deno.test("a truncated record at the very end is not decoded", () => {
	// Half a record is not a record; decoding it would invent a version.
	const flash = new Uint8Array(0x100).fill(0xff);
	flash.set(new TextEncoder().encode(FWINFO_ID), 0xf0 + 1);
	assertEquals(findRecord(flash), -1);
});

Deno.test("the short read is used when the record is near the start", async () => {
	const flash = flashWith(record({ version: [2, 0, 0] }), 0x1fc, 0x8000);
	const reads = [];

	const readMemory = async (_dev, start, end) => {
		reads.push(end - start + 1);
		return flash.slice(start, end + 1);
	};

	const info = await readFirmwareInfo(null, readMemory);
	assertEquals(info.version, "2.0.0");
	// One transfer, not a scan of the whole application section.
	assertEquals(reads, [LIKELY_WITHIN]);
});

Deno.test("a record further in is still found", async () => {
	const flash = flashWith(record({ version: [3, 0, 0] }), 0x4000, 0x8000);
	const reads = [];

	const readMemory = async (_dev, start, end) => {
		reads.push(end - start + 1);
		return flash.slice(start, end + 1);
	};

	const info = await readFirmwareInfo(null, readMemory);
	assertEquals(info.version, "3.0.0");
	assertEquals(reads.length, 2, "should fall back to the wider read");
});
