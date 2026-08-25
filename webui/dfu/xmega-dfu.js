// XMEGA support for Atmel's DFU bootloader, over WebUSB.
//
// The protocol layer in vendor/AtmelDFU.js is already right for this part -
// tmk wrote it from the same AVR4023 document, and the short commands
// (select, erase, blank check, launch, read) carry no packet-size assumption.
// Only two things differ on the ATxmega128A4U, and both are in the one path
// that sends a command *with* a data payload:
//
//   bMaxPacketSize0 is 64, not 32. AVR4023 figure 6-3 pads the command out to
//   a full packet before the data follows, so the header is 64 bytes here
//   where the AT90USB and ATmega parts use 32. Measured off the device:
//
//       idProduct 0x2fde ATXMEGA128A4U DFU Bootloader
//       bMaxPacketSize0 64
//
//   The flash page is larger. The vendored writeBlock rejects a start address
//   that is not a multiple of 256; this part's page is bigger than that, so
//   the check would pass addresses the bootloader cannot honour.
//
// Everything else is re-exported from the vendored module unchanged, so the
// two stay in step and the difference between them remains obvious.

import {
	bRequest,
	bStatus,
	bState,
	getStatus,
	parseStatus,
	selectPage,
	chipErase,
	readBlock,
	launch,
	clearStatus,
	abort,
} from "./vendor/AtmelDFU.js";

export {
	bRequest,
	bStatus,
	bState,
	getStatus,
	parseStatus,
	selectPage,
	chipErase,
	readBlock,
	launch,
	clearStatus,
	abort,
};

// Measured from the device rather than assumed - see the note above.
export const MAX_PACKET_SIZE_0 = 64;

// Bytes per USB transfer. The bootloader accepts more than one flash page at
// a time; this is a transfer size, not a page size.
export const TRANSFER_SIZE = 0x400;

// The bootloader addresses flash in 64 KB pages, selected separately, because
// the command's address fields are 16 bits. 128 KB of flash is two of them.
export const SELECT_PAGE_SIZE = 0x10000;

// Page sizes for this part. XMEGA A4U datasheet (8387) table 7-2 gives the
// ATxmega128A4U a 128-word flash page - words, so 256 bytes - and table 7-3
// gives 32-byte EEPROM pages. 128K in 512 application pages agrees.
// Only writeBlock uses them, to reject a start the bootloader would refuse.
export const FLASH_PAGE_SIZE = 0x100;
export const EEPROM_PAGE_SIZE = 0x20;

// Every block after the first begins where the last one ended, so a transfer
// that is not a whole number of pages leaves the next one misaligned.
if (TRANSFER_SIZE % FLASH_PAGE_SIZE !== 0) {
	throw new Error("TRANSFER_SIZE must be a whole number of flash pages");
}

/*
	Sections as the part actually lays them out.

	Named appSize rather than flashSize on purpose. The vendored table uses
	flashSize with the boot section carved out of its top, which is how the
	AT90USB and ATmega parts are arranged. XMEGA is not: BOOT_SECTION_START is
	0x20000 and APP_SECTION_SIZE is the whole 0x20000 below it, so subtracting
	the boot size from the flash size - as the AT90USB convention would - wrongly
	rejects the top 8 KB of perfectly valid application space.
*/
export const deviceInfo = [
	{
		name: "atxmega128a4u",
		vendorId: 0x03eb,
		productId: 0x2fde,
		appSize: 0x20000, // APP_SECTION_SIZE
		bootStart: 0x20000, // BOOT_SECTION_START
		bootSize: 0x2000, // BOOT_SECTION_SIZE
		eepromSize: 0x800,
	},
];

/** The entry for a connected device, or null if it is not one we know. */
export function identify(dev) {
	return (
		deviceInfo.find(
			(d) => d.vendorId === dev.vendorId && d.productId === dev.productId,
		) ?? null
	);
}

/**
 * Ask for the DFU device and claim it.
 *
 * Filtered to Atmel's vendor id rather than the exact product, so a device
 * that is not supported still reaches identify() and can be reported by name
 * instead of simply not appearing in the picker.
 */
export async function requestDevice() {
	const dev = await navigator.usb.requestDevice({
		filters: [{ vendorId: 0x03eb }],
	});

	await dev.open();
	if (dev.configuration === null) await dev.selectConfiguration(1);
	await dev.claimInterface(0);

	return dev;
}

/**
 * Write one block of a memory.
 *
 * Addresses are relative to the selected 64 KB page, which is why the caller
 * masks them before getting here.
 */
export function writeBlock(dev, start, end, data, eeprom = false) {
	const page = eeprom ? EEPROM_PAGE_SIZE : FLASH_PAGE_SIZE;

	if (start % page !== 0) {
		throw new Error(`Not page aligned: ${start} is not a multiple of ${page}`);
	}

	// 4.6.1.1 Write Command, padded out to a full control packet before the
	// data begins - this is the part that differs from the AT90USB parts.
	const header = new Uint8Array(MAX_PACKET_SIZE_0);
	header[0] = 0x01;
	header[1] = eeprom ? 0x01 : 0x00;
	header[2] = (start >> 8) & 0xff;
	header[3] = start & 0xff;
	header[4] = (end >> 8) & 0xff;
	header[5] = end & 0xff;

	// A DFU suffix the bootloader expects but does not check - the CRC and ids
	// are all-ones precisely because it ignores them.
	const footer = new Uint8Array([
		0x00, 0x00, 0x00, 0x00, 0x10, 0x44, 0x46, 0x55, 0x01, 0x00, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff,
	]);

	const msg = new Uint8Array(header.length + data.length + footer.length);
	msg.set(header, 0);
	msg.set(data, header.length);
	msg.set(footer, header.length + data.length);

	return dev.controlTransferOut(
		{
			requestType: "class",
			recipient: "interface",
			request: bRequest.DFU_DNLOAD,
			value: 0,
			index: 0,
		},
		msg,
	);
}

/**
 * Write a range of memory, selecting 64 KB pages and chunking as needed.
 *
 * @param onProgress Called with (bytesWritten, total) after each block.
 */
export async function writeMemory(
	dev,
	start,
	end,
	data,
	eeprom = false,
	onProgress = null,
) {
	if (start > end) throw new Error("Memory range error");

	const total = end - start + 1;
	let page = -1;
	let at = start;
	let written = 0;

	while (at < end) {
		if (page !== Math.floor(at / SELECT_PAGE_SIZE)) {
			page = Math.floor(at / SELECT_PAGE_SIZE);

			const selected = await selectPage(dev, page);
			if (selected.status !== "ok") {
				throw new Error(`selectPage(${page}) failed: ${selected.status}`);
			}
			await getStatus(dev);
		}

		let last = at + TRANSFER_SIZE - 1;
		if (last > end) last = end;

		/*
			A block may not straddle a 64 KB page, since the addresses in the
			command are relative to whichever one is selected.

			The last byte of a page is one below where the next one starts.
			Clamping to the boundary itself lands on offset zero of the *next*
			page, which masks back to 0 and produces a block running from
			0xFE00 to 0x0000 - a reversed range the device cannot honour, and
			every block after it misaligned by the leftover byte.
		*/
		const pageEnd = (page + 1) * SELECT_PAGE_SIZE - 1;
		if (last > pageEnd) last = pageEnd;

		const chunk = data.slice(at - start, last - start + 1);
		const result = await writeBlock(
			dev,
			at % SELECT_PAGE_SIZE,
			last % SELECT_PAGE_SIZE,
			chunk,
			eeprom,
		);

		if (result.status !== "ok") {
			throw new Error(`writeBlock at ${at} failed: ${result.status}`);
		}

		// An error latches dfuERROR and every later command fails until it is
		// cleared, so a failure is worth reporting where it happened rather
		// than as a pile of failures further along.
		const status = parseStatus((await getStatus(dev)).data);
		if (status.bStatus !== bStatus.OK) {
			throw new Error(
				`Device reported status ${status.bStatus} in state ${status.bState} at ${at}`,
			);
		}

		written += result.bytesWritten;
		at = last + 1;

		if (onProgress) onProgress(Math.min(written, total), total);
	}

	return written;
}

/**
 * Read a range of memory back.
 *
 * Ported rather than re-exported for the same reason as writeMemory: the
 * vendored version clamps a block to the page boundary itself rather than to
 * the last byte below it, which produces a reversed range. Verifying with a
 * broken reader would report a good write as a mismatch.
 *
 * @returns {Uint8Array} The bytes read.
 */
export async function readMemory(dev, start, end, eeprom = false) {
	if (start > end) throw new Error("Memory range error");

	const buf = new Uint8Array(end - start + 1);
	let page = -1;
	let at = start;

	while (at < end) {
		if (page !== Math.floor(at / SELECT_PAGE_SIZE)) {
			page = Math.floor(at / SELECT_PAGE_SIZE);

			const selected = await selectPage(dev, page);
			if (selected.status !== "ok") {
				throw new Error(`selectPage(${page}) failed: ${selected.status}`);
			}
			await getStatus(dev);
		}

		let last = at + TRANSFER_SIZE - 1;
		if (last > end) last = end;

		const pageEnd = (page + 1) * SELECT_PAGE_SIZE - 1;
		if (last > pageEnd) last = pageEnd;

		const result = await readBlock(
			dev,
			at % SELECT_PAGE_SIZE,
			last % SELECT_PAGE_SIZE,
			eeprom,
		);

		if (result.status !== "ok") {
			throw new Error(`readBlock at ${at} failed: ${result.status}`);
		}

		buf.set(new Uint8Array(result.data.buffer), at - start);
		at = last + 1;
	}

	return buf;
}
