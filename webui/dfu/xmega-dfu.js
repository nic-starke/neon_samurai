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
	readMemory,
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
	readMemory,
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

// Flash page for this part. Writes here start at zero in TRANSFER_SIZE steps,
// so alignment holds regardless, but a partial write would depend on it.
export const FLASH_PAGE_SIZE = 0x200;
export const EEPROM_PAGE_SIZE = 0x20;

export const deviceInfo = [
	{
		name: "atxmega128a4u",
		vendorId: 0x03eb,
		productId: 0x2fde,
		flashSize: 0x20000,
		bootSize: 0x2000,
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

		// A block may not straddle a 64 KB page, since the addresses in the
		// command are relative to whichever one is selected.
		if (last > (page + 1) * SELECT_PAGE_SIZE) {
			last = (page + 1) * SELECT_PAGE_SIZE;
		}

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
