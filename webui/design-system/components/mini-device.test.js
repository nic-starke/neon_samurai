// Tests for the sidebar miniature's rendering logic.
//
// The DOM-building half (buildMiniDevice itself) needs an SVG document, which
// this environment does not have - see webui/test.html for that, run in a
// real browser as part of the self-test. What is tested here is the part a
// past bug actually lived in: deciding whether an encoder is lit, powered, or
// has no data at all.
//
//   deno test --allow-read webui/design-system/components/mini-device.test.js

import { assertEquals } from "@std/assert";
import { isPowered, ledColor } from "./mini-device.js";

// A connected encoder's real shape - see encoderPropsFor() in js/editor.js.
// It never sets `powered` at all; only the disconnected chassis does.
function connectedEncoderProps({ lit = [], colorOverride = null, brightness = null } = {}) {
	return {
		litMask: Array.from({ length: 11 }, (_, i) => lit.includes(i)),
		ledColorOverride: colorOverride,
		ledBrightness: brightness,
		rgbColor: "hsl(140 90% 55%)",
		rgbOff: false,
	};
}

Deno.test("no props at all is unpowered", () => {
	// The placeholder for a detected-but-not-connected device.
	assertEquals(isPowered(null), false);
	assertEquals(isPowered(undefined), false);
});

Deno.test("a connected encoder is powered though it never says so", () => {
	// Regression: this was read the other way round, so every indicator LED
	// on a live device rendered dark - only the RGB arc, which does not use
	// this check, still showed colour.
	assertEquals(isPowered(connectedEncoderProps()), true);
});

Deno.test("the disconnected chassis is still respected when it says so", () => {
	assertEquals(isPowered({ powered: false }), false);
});

Deno.test("ledColor never touches its argument when there is no device", () => {
	// Reading anything off `props` here previously threw, because the early
	// unpowered check and the object having fields were conflated.
	assertEquals(ledColor(null, 0), "var(--ds-led-powered-off)");
});

Deno.test("a lit position is coloured on", () => {
	const props = connectedEncoderProps({ lit: [3] });
	assertEquals(ledColor(props, 3), "var(--ds-led-on)");
	assertEquals(ledColor(props, 4), "var(--ds-led-off)");
});

Deno.test("a detent colour override wins at its index only", () => {
	const props = connectedEncoderProps({ colorOverride: { index: 5, color: "#ff0000" } });
	assertEquals(ledColor(props, 5), "#ff0000");
	assertEquals(ledColor(props, 6), "var(--ds-led-off)");
});

Deno.test("brightness takes over from the plain lit mask when present", () => {
	const props = connectedEncoderProps({ brightness: Array(11).fill(0).map((_, i) => (i === 2 ? 255 : 0)) });
	assertEquals(ledColor(props, 2).includes("100%"), true);
	assertEquals(ledColor(props, 3).includes("0%"), true);
});
