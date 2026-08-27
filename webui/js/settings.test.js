// Tests for the settings store.
//
//   deno test --allow-read webui/js/settings.test.js

import { assertEquals } from "@std/assert";
import * as settings from "./settings.js";

/** Runs body with a localStorage stand-in, then puts the real one back. */
function withStorage(impl, body) {
	const had = "localStorage" in globalThis;
	const original = globalThis.localStorage;
	Object.defineProperty(globalThis, "localStorage", { value: impl, configurable: true });
	settings.reset();

	try {
		body();
	} finally {
		if (had) {
			Object.defineProperty(globalThis, "localStorage", { value: original, configurable: true });
		} else {
			delete globalThis.localStorage;
		}
		settings.reset();
	}
}

function memoryStorage(initial = {}) {
	const data = { ...initial };
	return {
		getItem: (k) => (k in data ? data[k] : null),
		setItem: (k, v) => { data[k] = String(v); },
		data,
	};
}

Deno.test("auto-connect is off unless it has been turned on", () => {
	// Connecting takes the device's ports exclusively, so it is not something
	// to do behind the user's back.
	withStorage(memoryStorage(), () => {
		assertEquals(settings.get("autoConnect"), false);
	});
});

Deno.test("a setting survives being written and read back", () => {
	withStorage(memoryStorage(), () => {
		settings.set("autoConnect", true);
		assertEquals(settings.get("autoConnect"), true);
	});
});

Deno.test("a stored setting is loaded", () => {
	withStorage(memoryStorage({ "neosam.settings": '{"autoConnect":true}' }), () => {
		assertEquals(settings.get("autoConnect"), true);
	});
});

Deno.test("unknown keys in storage do not displace defaults", () => {
	withStorage(memoryStorage({ "neosam.settings": '{"somethingElse":1}' }), () => {
		assertEquals(settings.get("autoConnect"), false);
	});
});

Deno.test("corrupt storage falls back to defaults", () => {
	withStorage(memoryStorage({ "neosam.settings": "not json" }), () => {
		assertEquals(settings.get("autoConnect"), false);
	});
});

Deno.test("storage that throws does not break the editor", () => {
	// Private windows and blocked site data both do this.
	const hostile = {
		getItem: () => { throw new Error("blocked"); },
		setItem: () => { throw new Error("blocked"); },
	};

	withStorage(hostile, () => {
		assertEquals(settings.get("autoConnect"), false);
		settings.set("autoConnect", true);
		assertEquals(settings.get("autoConnect"), true, "should still apply for this session");
	});
});

Deno.test("changes are announced", () => {
	withStorage(memoryStorage(), () => {
		const seen = [];
		const off = settings.onChange((name, value) => seen.push([name, value]));

		settings.set("autoConnect", true);
		off();
		settings.set("autoConnect", false);

		assertEquals(seen, [["autoConnect", true]], "should stop after unsubscribing");
	});
});
