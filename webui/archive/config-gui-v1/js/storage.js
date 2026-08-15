// storage.js - local preset save/load, purely client-side. Never touches
// the device - a preset is a snapshot of DeviceModel.toJSON(); pushing it
// to hardware is a separate, explicit "Save to device" action in ui.js.

/**
 * Trigger a browser download of `model` as a JSON file.
 * @param {import("./device-model.js").DeviceModel} model
 * @param {string} [filename]
 */
export function savePreset(model, filename = defaultFilename()) {
	const json = JSON.stringify(model.toJSON(), null, 2);
	const blob = new Blob([json], { type: "application/json" });
	const url = URL.createObjectURL(blob);
	try {
		const a = document.createElement("a");
		a.href = url;
		a.download = filename;
		a.click();
	} finally {
		// Revoke on a delay rather than immediately - some browsers cancel
		// the download if the object URL is revoked before the click's
		// navigation has actually started.
		setTimeout(() => URL.revokeObjectURL(url), 1000);
	}
}

/**
 * Read a user-selected preset file and load it into `model`. Call this
 * from a `<input type=file>` change handler with `input.files[0]`.
 * @param {import("./device-model.js").DeviceModel} model
 * @param {File} file
 * @returns {Promise<void>}
 */
export async function loadPreset(model, file) {
	const text = await file.text();
	let obj;
	try {
		obj = JSON.parse(text);
	} catch (e) {
		throw new Error(`"${file.name}" is not valid JSON: ${e.message}`);
	}
	model.loadFromJSON(obj);
}

function defaultFilename() {
	const now = new Date();
	const pad = (n) => String(n).padStart(2, "0");
	const stamp = `${now.getFullYear()}-${pad(now.getMonth() + 1)}-${pad(now.getDate())}_${pad(now.getHours())}${pad(now.getMinutes())}`;
	return `neosam-preset-${stamp}.json`;
}
