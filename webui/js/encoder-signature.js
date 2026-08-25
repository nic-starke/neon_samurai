// Compact fingerprint of the visually-significant parts of a buildEncoder()
// prop object. editor.js compares this per frame and only rebuilds the
// encoders whose signature actually changed.

export function encoderSignature(p) {
	return [
		p.powered === false ? 0 : 1,
		(p.knobRotation ?? 0).toFixed(2),
		(p.litMask ?? []).map((v) => (v ? 1 : 0)).join(""),
		p.ledBrightness ? p.ledBrightness.join(",") : "",
		p.rgbColor ?? "",
		p.rgbOff ? 1 : 0,
		p.ledColorOverride ? p.ledColorOverride.color : "",
		p.vmapCount ?? "",
		p.vmapActive ?? "",
	].join("|");
}
