// A stand-in for a WebUSB device, for testing the DFU layer without hardware.
//
// It records every control transfer so a test can assert on what actually went
// out. That matters more here than in most places: the wire format is the
// thing that differs between this part and the ones the vendored code was
// written for, and getting it wrong writes a broken image to somebody's
// device. Hardware can tell us a flash worked; only this can tell us the
// header was 64 bytes with the right fields in it.

import { bRequest } from "./vendor/AtmelDFU.js";

export class FakeDfuDevice {
	/**
	 * @param options.vendorId,productId  Reported identity.
	 * @param options.statuses  Status packets to return in order, each
	 *   {bStatus, bState}. The last is repeated once exhausted, so a test only
	 *   has to describe the interesting part.
	 * @param options.failWriteAt  Byte offset whose write returns a failure,
	 *   for exercising the error path.
	 */
	constructor(options = {}) {
		this.vendorId = options.vendorId ?? 0x03eb;
		this.productId = options.productId ?? 0x2fde;
		this.productName = options.productName ?? "ATXMEGA128A4U DFU Bootloader";
		this.configuration = { configurationValue: 1 };

		this._statuses = options.statuses ?? [{ bStatus: 0, bState: 2 }];
		this._statusIndex = 0;
		this._failWriteAt = options.failWriteAt ?? null;
		this._written = 0;

		// Everything that went out, in order.
		this.transfers = [];

		// A model of the memory, so a read returns what was written and the
		// success path can be tested rather than only the failure paths.
		this.memory = new Uint8Array(options.memorySize ?? 0x20000).fill(0xff);
		this._selectedPage = 0;
		this._pendingRead = null;
	}

	async controlTransferOut(setup, data) {
		const bytes = new Uint8Array(data);
		this.transfers.push({ dir: "out", request: setup.request, data: bytes });

		if (setup.request === bRequest.DFU_DNLOAD) {
			// 06 03 00 <page>: which 64 KB page the addresses below refer to.
			if (bytes.length <= 6 && bytes[0] === 0x06 && bytes[1] === 0x03) {
				this._selectedPage = bytes[3];
			}

			// 03 00 <start> <end>: a read, answered by the UPLOAD that follows.
			if (bytes.length <= 6 && bytes[0] === 0x03 && bytes[1] === 0x00) {
				const start = (bytes[2] << 8) | bytes[3];
				const end = (bytes[4] << 8) | bytes[5];
				const base = this._selectedPage * 0x10000;
				this._pendingRead = { from: base + start, to: base + end };
			}

			// 04 00 ff: chip erase.
			if (bytes.length <= 6 && bytes[0] === 0x04 && bytes[1] === 0x00) {
				this.memory.fill(0xff);
			}

			if (bytes.length > 6) {
				const before = this._written;
				this._written += bytes.length;

				if (this._failWriteAt !== null && before <= this._failWriteAt &&
						this._failWriteAt < this._written) {
					return { status: "stall", bytesWritten: 0 };
				}

				// Header is one packet, then the data, then a 16-byte suffix.
				const start = (bytes[2] << 8) | bytes[3];
				const payload = bytes.slice(64, bytes.length - 16);
				this.memory.set(payload, this._selectedPage * 0x10000 + start);
			}
		}

		return { status: "ok", bytesWritten: bytes.length };
	}

	async controlTransferIn(setup, length) {
		this.transfers.push({ dir: "in", request: setup.request, length });

		if (setup.request === bRequest.DFU_GETSTATUS) {
			const i = Math.min(this._statusIndex, this._statuses.length - 1);
			const { bStatus, bState } = this._statuses[i];
			this._statusIndex++;

			const buf = new Uint8Array([bStatus, 0, 0, 0, bState, 0]);
			return { status: "ok", data: new DataView(buf.buffer) };
		}

		if (setup.request === bRequest.DFU_UPLOAD && this._pendingRead) {
			const { from, to } = this._pendingRead;
			this._pendingRead = null;
			const slice = this.memory.slice(from, to + 1);
			return { status: "ok", data: new DataView(slice.buffer) };
		}

		return { status: "ok", data: new DataView(new Uint8Array(length).buffer) };
	}

	/** Just the DFU_DNLOAD transfers that carried a data payload. */
	writes() {
		return this.transfers.filter(
			(t) => t.dir === "out" && t.request === bRequest.DFU_DNLOAD && t.data.length > 6,
		);
	}

	/** Just the short commands - select, erase, launch and so on. */
	commands() {
		return this.transfers.filter(
			(t) => t.dir === "out" && t.request === bRequest.DFU_DNLOAD && t.data.length <= 6,
		);
	}
}
