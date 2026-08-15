// Standard 0-360/0-100/0-100 HSV for cosmetic material shading - NOT the
// firmware's 0-1535/0-255/0-255 model in webui/js/color.js. A live
// device colour is converted via color.js's hsvToCss() by the caller and
// passed in as a CSS string; this module only handles decorative tones.

function hexToRgb(hex) {
	const h = (hex || "#4a4d55").replace("#", "");
	const s = h.length === 3 ? h.split("").map((c) => c + c).join("") : h;
	return [parseInt(s.slice(0, 2), 16), parseInt(s.slice(2, 4), 16), parseInt(s.slice(4, 6), 16)];
}

export function shift(hex, amount) {
	const [r, g, b] = hexToRgb(hex);
	const f = (v) =>
		Math.max(0, Math.min(255, Math.round(amount > 0 ? v + (255 - v) * amount : v * (1 + amount))));
	return `rgb(${f(r)},${f(g)},${f(b)})`;
}

export function dim(hex, amount) {
	return shift(hex, -Math.abs(amount));
}

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
