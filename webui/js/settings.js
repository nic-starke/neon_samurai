// Editor settings, kept in localStorage.
//
// Every setting has a default here, and reads fall back to it - a browser with
// storage disabled, or a private window, gets working defaults rather than an
// exception. Nothing here is device configuration; that lives on the device.

const KEY = "neosam.settings";

export const DEFAULTS = Object.freeze({
	// Off by default. Connecting opens the device's ports exclusively, and on
	// Linux that takes it away from anything else already using it - so the
	// editor does not do it behind the user's back until they ask.
	autoConnect: false,
});

let cache = null;
const listeners = new Set();

function read() {
	if (cache) return cache;

	try {
		cache = { ...DEFAULTS, ...JSON.parse(localStorage.getItem(KEY) ?? "{}") };
	} catch {
		cache = { ...DEFAULTS };
	}

	return cache;
}

export function get(name) {
	return read()[name];
}

export function set(name, value) {
	cache = { ...read(), [name]: value };

	try {
		localStorage.setItem(KEY, JSON.stringify(cache));
	} catch {
		// Storage can be unavailable or full. The setting still applies for
		// this session, which is better than refusing the change.
	}

	for (const listener of listeners) listener(name, value);
}

export function onChange(listener) {
	listeners.add(listener);
	return () => listeners.delete(listener);
}

/** Test seam - drops the cached copy so the next read hits storage. */
export function reset() {
	cache = null;
}
