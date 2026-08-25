// Intel HEX, enough of it to load a firmware image.
//
// The vendored flasher has its own parser but does not export it, and this is
// small enough that reaching into a copy that must stay pristine is not worth
// it.
//
// Records are ':' LL AAAA TT <data> CC, all hex, checksummed with a two's
// complement byte sum. Only the record types an avr-gcc image actually
// contains are handled - anything else is an error rather than something to
// skip past quietly, because silently ignoring a record means flashing an
// image with a hole in it.

const RECORD = {
	DATA: 0x00,
	EOF: 0x01,
	EXTENDED_SEGMENT_ADDRESS: 0x02,
	EXTENDED_LINEAR_ADDRESS: 0x04,
};

/**
 * @param text The .hex file contents.
 * @returns {{data: Uint8Array, start: number, end: number}} A flat image, and
 * the address range it covers. Gaps between records read as 0xFF, which is
 * what erased flash holds.
 */
export function parseIntelHex(text) {
	const chunks = [];
	let extended = 0;
	let lowest = Infinity;
	let highest = -1;
	let sawEof = false;

	const lines = text.split(/\r?\n/);

	for (let i = 0; i < lines.length; i++) {
		const line = lines[i].trim();
		if (line === "") continue;

		const where = `line ${i + 1}`;

		if (line[0] !== ":") throw new Error(`${where}: record does not start with ':'`);
		if (line.length < 11) throw new Error(`${where}: record too short`);
		if ((line.length - 1) % 2 !== 0) throw new Error(`${where}: odd number of hex digits`);

		const bytes = new Uint8Array((line.length - 1) / 2);
		for (let b = 0; b < bytes.length; b++) {
			const pair = line.substr(1 + b * 2, 2);
			if (!/^[0-9a-fA-F]{2}$/.test(pair)) {
				throw new Error(`${where}: '${pair}' is not a hex byte`);
			}
			bytes[b] = parseInt(pair, 16);
		}

		const length = bytes[0];
		const address = (bytes[1] << 8) | bytes[2];
		const type = bytes[3];

		if (bytes.length !== length + 5) {
			throw new Error(`${where}: length byte says ${length}, record carries ${bytes.length - 5}`);
		}

		// Two's complement of the sum of everything before it.
		const sum = bytes.slice(0, -1).reduce((a, b) => (a + b) & 0xff, 0);
		const checksum = (0x100 - sum) & 0xff;
		if (checksum !== bytes[bytes.length - 1]) {
			throw new Error(`${where}: checksum mismatch`);
		}

		const payload = bytes.slice(4, 4 + length);

		switch (type) {
			case RECORD.DATA: {
				const at = extended + address;
				chunks.push({ at, payload });
				if (at < lowest) lowest = at;
				if (at + length - 1 > highest) highest = at + length - 1;
				break;
			}

			case RECORD.EOF:
				sawEof = true;
				break;

			case RECORD.EXTENDED_SEGMENT_ADDRESS:
				extended = ((payload[0] << 8) | payload[1]) * 16;
				break;

			case RECORD.EXTENDED_LINEAR_ADDRESS:
				extended = ((payload[0] << 8) | payload[1]) * 65536;
				break;

			default:
				throw new Error(`${where}: unsupported record type 0x${type.toString(16)}`);
		}

		if (sawEof) break;
	}

	if (!sawEof) throw new Error("no end-of-file record - the image is truncated");
	if (highest < 0) throw new Error("no data records - nothing to flash");

	// 0xFF rather than 0x00 for the gaps: that is what an erased cell reads as,
	// so a gap costs nothing to write and the image matches a verify.
	const data = new Uint8Array(highest - lowest + 1).fill(0xff);
	for (const { at, payload } of chunks) {
		data.set(payload, at - lowest);
	}

	return { data, start: lowest, end: highest };
}
