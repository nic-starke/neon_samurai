// Reading the firmware's own record of itself out of flash.
//
// The device reports its version over sysex, but only while it is running.
// One sitting in the bootloader has no MIDI interface, and the bootloader can
// do nothing but read and write memory - so this is the only way to ask a
// stopped device what it is.
//
// Mirrors struct fwinfo in src/include/system/fwinfo.h. Keep the layout and
// the identifier in step with it.

export const FWINFO_ID = "NEON_SAMURAI";

// struct fwinfo, in declaration order. Every field is a byte or an array of
// them, so the compiler inserts no padding and these offsets are the layout.
const FORMAT_AT = 0;
const ID_AT = 1;
const ID_LEN = 16;
const COMMIT_AT = ID_AT + ID_LEN;
const COMMIT_LEN = 12;
const VERSION_AT = COMMIT_AT + COMMIT_LEN;
const RECORD_LEN = VERSION_AT + 3;

// The record lands just after the interrupt vector table, so this finds it in
// one transfer in practice. It is a hint, not a promise - see readFirmwareInfo.
export const LIKELY_WITHIN = 0x400;

const text = (bytes) =>
	new TextDecoder().decode(bytes).replace(/\0+$/, "");

/** Locate the record in a block of flash. @returns its offset, or -1. */
export function findRecord(bytes) {
	const needle = new TextEncoder().encode(FWINFO_ID);

	// The identifier is the second field, so a match points one byte past the
	// start of the record.
	outer: for (let i = ID_AT; i + (RECORD_LEN - ID_AT) <= bytes.length; i++) {
		for (let j = 0; j < needle.length; j++) {
			if (bytes[i + j] !== needle[j]) continue outer;
		}
		return i - ID_AT;
	}

	return -1;
}

/**
 * Decode the record at an offset.
 * @returns {{id, format, version, commit, dirty}}
 */
export function decodeRecord(bytes, at) {
	const field = (off, len) => text(bytes.slice(at + off, at + off + len));

	const major = bytes[at + VERSION_AT];
	const minor = bytes[at + VERSION_AT + 1];
	const patch = bytes[at + VERSION_AT + 2];
	const commit = field(COMMIT_AT, COMMIT_LEN);

	return {
		id: field(ID_AT, ID_LEN),
		format: bytes[at + FORMAT_AT],
		version: `${major}.${minor}.${patch}`,
		// A trailing marker means the tree had uncommitted changes, so the
		// commit names roughly what was built, not exactly.
		commit: commit.replace(/\+$/, ""),
		dirty: commit.endsWith("+"),
	};
}

/** Parse a whole flash image. @returns the record, or null. */
export function parseFirmwareInfo(bytes) {
	const at = findRecord(bytes);
	return at < 0 ? null : { ...decodeRecord(bytes, at), offset: at };
}

/**
 * Read the record off a device in the bootloader.
 *
 * Tries a short read first, because in practice the record sits just past the
 * vector table; only if that misses does it read further. A firmware built
 * before this record existed simply has none, which is not an error - it is
 * the answer.
 *
 * @param readMemory dfu.readMemory, passed in so this stays testable.
 */
export async function readFirmwareInfo(dev, readMemory, fullLength = 0x8000) {
	const quick = await readMemory(dev, 0, LIKELY_WITHIN - 1);
	const found = parseFirmwareInfo(quick);
	if (found) return found;

	if (fullLength <= LIKELY_WITHIN) return null;

	const all = await readMemory(dev, 0, fullLength - 1);
	return parseFirmwareInfo(all);
}
