// Tests for the Intel HEX parser.
//
//   deno test --allow-read tools/dfu/intel-hex.test.js
//
// A malformed image that parses is worse than one that does not: it gets
// flashed. So most of these check that bad input is refused rather than
// quietly producing something short or full of holes.

import { assertEquals, assertThrows } from "@std/assert";
import { parseIntelHex } from "./intel-hex.js";

// ':' LL AAAA TT <data> CC, checksum being the two's complement byte sum.
function record(type, address, data = []) {
	const bytes = [data.length, (address >> 8) & 0xff, address & 0xff, type, ...data];
	const sum = bytes.reduce((a, b) => (a + b) & 0xff, 0);
	const hex = [...bytes, (0x100 - sum) & 0xff]
		.map((b) => b.toString(16).padStart(2, "0").toUpperCase())
		.join("");
	return ":" + hex;
}

const EOF_RECORD = record(0x01, 0);

Deno.test("reads a single data record", () => {
	const img = parseIntelHex([record(0x00, 0, [1, 2, 3, 4]), EOF_RECORD].join("\n"));
	assertEquals(img.start, 0);
	assertEquals(img.end, 3);
	assertEquals([...img.data], [1, 2, 3, 4]);
});

Deno.test("gaps between records read as erased flash", () => {
	// Anything not covered by a record must come back 0xFF, not 0x00 - an
	// erased cell reads as 0xFF, so writing that costs nothing and a verify
	// still matches.
	const img = parseIntelHex(
		[record(0x00, 0, [0xaa]), record(0x00, 4, [0xbb]), EOF_RECORD].join("\n"),
	);
	assertEquals([...img.data], [0xaa, 0xff, 0xff, 0xff, 0xbb]);
});

Deno.test("extended linear address moves the base", () => {
	const img = parseIntelHex(
		[record(0x04, 0, [0x00, 0x01]), record(0x00, 0, [0x42]), EOF_RECORD].join("\n"),
	);
	assertEquals(img.start, 0x10000);
});

Deno.test("carriage returns and blank lines are tolerated", () => {
	const img = parseIntelHex(
		`${record(0x00, 0, [1, 2])}\r\n\r\n${EOF_RECORD}\r\n`,
	);
	assertEquals([...img.data], [1, 2]);
});

Deno.test("records after the end-of-file record are ignored", () => {
	const img = parseIntelHex(
		[record(0x00, 0, [1]), EOF_RECORD, record(0x00, 8, [9, 9, 9])].join("\n"),
	);
	assertEquals(img.end, 0);
});

Deno.test("a bad checksum is refused", () => {
	const good = record(0x00, 0, [1, 2, 3, 4]);
	const bad = good.slice(0, -2) + "00";
	assertThrows(() => parseIntelHex([bad, EOF_RECORD].join("\n")), Error, "checksum");
});

Deno.test("a truncated file is refused", () => {
	// No end-of-file record: the image may be missing its tail, and flashing
	// most of a firmware is worse than flashing none of it.
	assertThrows(() => parseIntelHex(record(0x00, 0, [1, 2, 3, 4])), Error, "truncated");
});

Deno.test("a file with nothing to flash is refused", () => {
	assertThrows(() => parseIntelHex(EOF_RECORD), Error, "no data");
});

Deno.test("non-hex characters are refused", () => {
	assertThrows(
		() => parseIntelHex([":04000000ZZ010203F6", EOF_RECORD].join("\n")), Error, "hex byte",
	);
});

Deno.test("a length byte that disagrees with the record is refused", () => {
	assertThrows(
		() => parseIntelHex([":100000000001F6", EOF_RECORD].join("\n")), Error, "length",
	);
});

Deno.test("an unsupported record type is refused, not skipped", () => {
	// Skipping one would mean flashing an image with a hole where it was.
	assertThrows(
		() => parseIntelHex([record(0x03, 0, [0, 0, 0, 0]), EOF_RECORD].join("\n")),
		Error,
		"unsupported record type",
	);
});

Deno.test("the real firmware image parses", async () => {
	const text = await Deno.readTextFile("build/Release/neosam.hex");
	const img = parseIntelHex(text);

	assertEquals(img.start, 0);
	// The vector table is the first thing in an avr-gcc image, and its first
	// entry is a jmp - 0x0C 0x94 on this part.
	assertEquals(img.data[0], 0x0c);
	assertEquals(img.data[1], 0x94);
	// Application section only; the boot section starts at 0x20000.
	assertEquals(img.end < 0x20000, true);
});
