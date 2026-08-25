// Tests for the XMEGA DFU layer.
//
//   deno test --allow-read tools/dfu/xmega-dfu.test.js
//
// These assert on the bytes that go out, not on whether a flash succeeded.
// A device can accept a malformed write and end up holding a broken image,
// so "it flashed" is not the same as "it was right" - that is what these are
// for, and why they check field positions and padding rather than outcomes.

import { assertEquals, assertRejects, assertThrows } from "@std/assert";
import * as dfu from "./xmega-dfu.js";
import { FakeDfuDevice } from "./fake-device.js";

const image = (n, fill = 0xa5) => new Uint8Array(n).fill(fill);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ The write header ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Deno.test("the command is padded to a full 64-byte packet", async () => {
	// The whole reason this port exists. The vendored code pads to 32, which
	// is right for the AT90USB parts and wrong here - the device reports
	// bMaxPacketSize0 = 64.
	const dev = new FakeDfuDevice();
	await dfu.writeBlock(dev, 0, 0x1ff, image(0x200));

	const sent = dev.writes()[0].data;
	assertEquals(dfu.MAX_PACKET_SIZE_0, 64);
	// header + data + 16-byte DFU suffix
	assertEquals(sent.length, 64 + 0x200 + 16);
});

Deno.test("the header carries the write command and address range", async () => {
	const dev = new FakeDfuDevice();
	await dfu.writeBlock(dev, 0x0400, 0x07ff, image(0x400));

	const sent = dev.writes()[0].data;
	assertEquals(sent[0], 0x01, "write command");
	assertEquals(sent[1], 0x00, "flash, not eeprom");
	assertEquals(sent[2], 0x04, "start high");
	assertEquals(sent[3], 0x00, "start low");
	assertEquals(sent[4], 0x07, "end high");
	assertEquals(sent[5], 0xff, "end low");

	// Everything between the fields and the data must be zero, or the device
	// reads rubbish where it expects padding.
	for (let i = 6; i < 64; i++) assertEquals(sent[i], 0, `padding byte ${i}`);
});

Deno.test("eeprom writes are marked as such", async () => {
	const dev = new FakeDfuDevice();
	await dfu.writeBlock(dev, 0, 0x1f, image(0x20), true);
	assertEquals(dev.writes()[0].data[1], 0x01);
});

Deno.test("the data sits immediately after the header", async () => {
	const dev = new FakeDfuDevice();
	const payload = new Uint8Array(0x200);
	payload[0] = 0xde;
	payload[1] = 0xad;
	payload[0x1ff] = 0xbe;

	await dfu.writeBlock(dev, 0, 0x1ff, payload);

	const sent = dev.writes()[0].data;
	assertEquals(sent[64], 0xde);
	assertEquals(sent[65], 0xad);
	assertEquals(sent[64 + 0x1ff], 0xbe);
});

Deno.test("the DFU suffix follows the data", async () => {
	const dev = new FakeDfuDevice();
	await dfu.writeBlock(dev, 0, 0x1ff, image(0x200));

	const sent = dev.writes()[0].data;
	const suffix = sent.slice(64 + 0x200);

	assertEquals(suffix.length, 16);
	assertEquals(suffix[4], 0x10, "suffix length");
	// 'D' 'F' 'U'
	assertEquals([suffix[5], suffix[6], suffix[7]], [0x44, 0x46, 0x55]);
});

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Alignment ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Deno.test("a misaligned flash write is refused", () => {
	const dev = new FakeDfuDevice();
	assertThrows(
		() => dfu.writeBlock(dev, 0x100, 0x2ff, image(0x200)),
		Error,
		"page aligned",
	);
});

Deno.test("flash alignment is the page size, not the vendored 256", () => {
	// 0x100 is a legal start for the parts the vendored code targets and not
	// for this one. If this ever stops throwing, the port has regressed to
	// upstream's assumption.
	assertEquals(dfu.FLASH_PAGE_SIZE, 0x200);
	const dev = new FakeDfuDevice();
	assertThrows(() => dfu.writeBlock(dev, 0x100, 0x1ff, image(0x100)), Error);
});

Deno.test("eeprom alignment is its own page size", () => {
	const dev = new FakeDfuDevice();
	assertThrows(() => dfu.writeBlock(dev, 0x10, 0x2f, image(0x20), true), Error);
});

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Whole transfers ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Deno.test("a short image goes out in one block", async () => {
	const dev = new FakeDfuDevice();
	await dfu.writeMemory(dev, 0, 0x3ff, image(0x400));
	assertEquals(dev.writes().length, 1);
});

Deno.test("a long image is split at the transfer size", async () => {
	const dev = new FakeDfuDevice();
	const size = dfu.TRANSFER_SIZE * 3;
	await dfu.writeMemory(dev, 0, size - 1, image(size));
	assertEquals(dev.writes().length, 3);
});

Deno.test("a page is selected before writing, and only when it changes", async () => {
	const dev = new FakeDfuDevice();
	await dfu.writeMemory(dev, 0, dfu.TRANSFER_SIZE * 3 - 1, image(dfu.TRANSFER_SIZE * 3));

	// 06 03 00 <page> - one selection for three blocks in the same page.
	const selects = dev.commands().filter((c) => c.data[0] === 0x06 && c.data[1] === 0x03);
	assertEquals(selects.length, 1);
	assertEquals(selects[0].data[3], 0);
});

Deno.test("crossing 64 KB selects the next page", async () => {
	// The address fields in the command are 16 bits, so anything past 64 KB is
	// reached by selecting a page first. Getting this wrong writes the upper
	// half of an image over the lower half.
	const dev = new FakeDfuDevice();
	const start = dfu.SELECT_PAGE_SIZE - dfu.TRANSFER_SIZE;
	const end = dfu.SELECT_PAGE_SIZE + dfu.TRANSFER_SIZE - 1;

	await dfu.writeMemory(dev, start, end, image(end - start + 1));

	const selects = dev.commands().filter((c) => c.data[0] === 0x06 && c.data[1] === 0x03);
	assertEquals(selects.map((s) => s.data[3]), [0, 1]);
});

Deno.test("addresses are relative to the selected page", async () => {
	const dev = new FakeDfuDevice();
	const start = dfu.SELECT_PAGE_SIZE;

	await dfu.writeMemory(dev, start, start + 0x3ff, image(0x400));

	// First block of page 1 addresses offset zero, not 0x10000.
	const sent = dev.writes()[0].data;
	assertEquals([sent[2], sent[3]], [0, 0]);
});

Deno.test("progress is reported and reaches the total", async () => {
	const dev = new FakeDfuDevice();
	const size = dfu.TRANSFER_SIZE * 3;
	const seen = [];

	await dfu.writeMemory(dev, 0, size - 1, image(size), false,
		(done, total) => seen.push([done, total]));

	assertEquals(seen.length, 3);
	assertEquals(seen[seen.length - 1][0], seen[seen.length - 1][1]);
});

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Failure paths ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Deno.test("a device error status stops the write", async () => {
	// An error latches dfuERROR and everything after it fails too, so the
	// first failure is the one worth reporting - carrying on would bury it.
	const dev = new FakeDfuDevice({
		statuses: [{ bStatus: 0, bState: 2 }, { bStatus: 3, bState: 10 }],
	});

	await assertRejects(
		() => dfu.writeMemory(dev, 0, dfu.TRANSFER_SIZE * 3 - 1, image(dfu.TRANSFER_SIZE * 3)),
		Error,
		"status 3",
	);

	// Stopped rather than ploughing on through the remaining blocks.
	assertEquals(dev.writes().length < 3, true);
});

Deno.test("a stalled transfer stops the write", async () => {
	const dev = new FakeDfuDevice({ failWriteAt: 0 });
	await assertRejects(
		() => dfu.writeMemory(dev, 0, 0x3ff, image(0x400)),
		Error,
		"writeBlock",
	);
});

Deno.test("a reversed range is refused", async () => {
	const dev = new FakeDfuDevice();
	await assertRejects(
		() => dfu.writeMemory(dev, 0x400, 0, image(4)),
		Error,
		"Memory range",
	);
});

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Identity ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Deno.test("the device this port targets is recognised", () => {
	const info = dfu.identify(new FakeDfuDevice());
	assertEquals(info?.name, "atxmega128a4u");
	assertEquals(info?.bootSize, 0x2000);
});

Deno.test("the boot section sits above the application section", () => {
	// Not carved out of the top of it, as it is on the parts the vendored
	// table describes. Getting this backwards rejects the top 8 KB of valid
	// application space.
	const info = dfu.identify(new FakeDfuDevice());
	assertEquals(info.appSize, 0x20000);
	assertEquals(info.bootStart, 0x20000);
	assertEquals(info.bootStart >= info.appSize, true);
});

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~ Crossing a page ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

Deno.test("a block that would straddle a page is cut at the page end", async () => {
	// From address zero a transfer never straddles, because 0x10000 divides
	// evenly by 0x400 - which is why neither this port nor upstream noticed.
	// Starting part way in, it does.
	const dev = new FakeDfuDevice();
	const start = 0x200;
	const end = 0x10400;

	await dfu.writeMemory(dev, start, end, image(end - start + 1));

	for (const w of dev.writes()) {
		const s = (w.data[2] << 8) | w.data[3];
		const e = (w.data[4] << 8) | w.data[5];
		assertEquals(s <= e, true, `block 0x${s.toString(16)}..0x${e.toString(16)} runs backwards`);
	}
});

Deno.test("every block stays flash-page aligned across a page boundary", async () => {
	// The straddle bug also left the next block starting one byte past a page,
	// so everything after it was misaligned.
	const dev = new FakeDfuDevice();
	const start = 0x200;
	const end = 0x10400;

	await dfu.writeMemory(dev, start, end, image(end - start + 1));

	for (const w of dev.writes()) {
		const s = (w.data[2] << 8) | w.data[3];
		assertEquals(s % dfu.FLASH_PAGE_SIZE, 0, `block starts at 0x${s.toString(16)}`);
	}
});

Deno.test("a transfer is a whole number of flash pages", () => {
	// Otherwise the block after the first begins mid-page.
	assertEquals(dfu.TRANSFER_SIZE % dfu.FLASH_PAGE_SIZE, 0);
});

Deno.test("another Atmel device is not mistaken for it", () => {
	// The picker filters on Atmel's vendor id, so an unsupported Atmel
	// bootloader can reach us. Flashing an XMEGA image into one would be bad.
	assertEquals(dfu.identify(new FakeDfuDevice({ productId: 0x2ff4 })), null);
});
