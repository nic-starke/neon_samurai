// color-utils.js - shared colour math for the design-system components.
// Standard 0-360/0-100/0-100 HSV for cosmetic material shading (plastic,
// rubber, LED glow) - not the firmware's 0-1535/0-255/0-255 model, which
// lives in webui/js/color.js and is unrelated to this file. Components
// that need to render a *live device* colour (RGB LED, cap accent) take a
// CSS colour string as a prop and let the caller (twin.js) do that
// conversion via color.js's hsvToCss(); this module only handles the
// twin's own decorative material tones.

function hexToRgb(hex) {
	const h = (hex || "#4a4d55").replace("#", "");
	const s = h.length === 3 ? h.split("").map((c) => c + c).join("") : h;
	return [parseInt(s.slice(0, 2), 16), parseInt(s.slice(2, 4), 16), parseInt(s.slice(4, 6), 16)];
}

/** Lighten (amount > 0) or darken (amount < 0) a hex colour, returning a
 * CSS rgb() string. amount is roughly -1..1. */
export function shift(hex, amount) {
	const [r, g, b] = hexToRgb(hex);
	const f = (v) =>
		Math.max(0, Math.min(255, Math.round(amount > 0 ? v + (255 - v) * amount : v * (1 + amount))));
	return `rgb(${f(r)},${f(g)},${f(b)})`;
}

/** Darken a hex/rgb colour towards --ds-led-off by `amount` (0..1), for
 * "unlit" component states that should still read as the same material
 * family rather than switching to a flat grey. */
export function dim(hex, amount) {
	return shift(hex, -Math.abs(amount));
}

/** Standard-range (h:0-360, s/v:0-100) HSV to hex, for cosmetic material
 * tones. Not the firmware colour model - see webui/js/color.js for that. */
export function hsvHex(h, s, v) {
	const S = s / 100;
	const V = v / 100;
	const c = V * S;
	const hp = (((h % 360) + 360) % 360) / 60;
	const x = c * (1 - Math.abs((hp % 2) - 1));
	const seg = [
		[c, x, 0],
		[x, c, 0],
		[0, c, x],
		[0, x, c],
		[x, 0, c],
		[c, 0, x],
	][Math.floor(hp) % 6];
	const m = V - c;
	return (
		"#" +
		seg
			.map((n) => Math.round((n + m) * 255).toString(16).padStart(2, "0"))
			.join("")
	);
}
