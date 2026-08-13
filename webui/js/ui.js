// ui.js - DOM rendering and event wiring. The only module that touches
// the DOM directly; everything else (midi.js, protocol.js, device-model.js,
// storage.js, color.js) is DOM-agnostic and could be reused headless.

import { hsvToCss } from "./color.js";
import {
	DeviceModel,
	MidiMode,
	NUM_BANKS,
	NUM_ENCODERS,
	NUM_SIDE_SWITCHES,
	NUM_VMAPS_PER_ENCODER,
	SideSwitchMode,
	SwitchMode,
	DisplayMode,
} from "./device-model.js";
import * as midi from "./midi.js";
import { Protocol } from "./protocol.js";
import { loadPreset, savePreset } from "./storage.js";
import { buildEncoder, computeLitMask, ENC_MID } from "../design-system/components/index.js";

const model = new DeviceModel();
/** @type {import("./midi.js").Device|null} */
let device = null;
/** @type {Protocol|null} */
let protocol = null;
let selectedEncoderIdx = null;
let selectedVmapIdx = 0;
let viewingBank = 0; // which bank's config is being *browsed*, independent of model.activeBank
// Reset flows (onFactoryResetClick, and Reset Device if a UI control for it
// is added later) trigger their own explicit waitAndReconnect() call after
// the reboot they themselves caused - set this first so the Device's own
// onDisconnect handler (which fires for that same reboot, just detected
// independently via the heartbeat/statechange) doesn't also try to
// reconnect at the same time.
let expectingDisconnect = false;

const el = {
	unsupportedBanner: byId("unsupported-banner"),
	statusDot: byId("status-dot"),
	statusText: byId("status-text"),
	fwVersion: byId("fw-version"),
	btnConnect: byId("btn-connect"),
	btnLoadDevice: byId("btn-load-device"),
	btnSaveDevice: byId("btn-save-device"),
	btnSavePreset: byId("btn-save-preset"),
	inputLoadPreset: byId("input-load-preset"),
	btnFactoryReset: byId("btn-factory-reset"),
	bankTabs: Array.from(document.querySelectorAll(".bank-tab")),
	btnSetActiveBank: byId("btn-set-active-bank"),
	activeBankIndicator: byId("active-bank-indicator"),
	encoderGrid: byId("encoder-grid"),
	sideSwitchRow: byId("side-switch-row"),
	detailPanel: byId("detail-panel"),
	detailPanelTitle: byId("detail-panel-title"),
	detailPanelBody: byId("detail-panel-body"),
	btnCloseDetail: byId("btn-close-detail"),
	toastContainer: byId("toast-container"),
};

function byId(id) {
	return document.getElementById(id);
}

// --- Startup ----------------------------------------------------------

function init() {
	if (!midi.isSupported()) {
		el.unsupportedBanner.hidden = false;
		el.btnConnect.disabled = true;
	}

	el.btnConnect.addEventListener("click", onConnectClick);
	el.btnLoadDevice.addEventListener("click", onLoadFromDeviceClick);
	el.btnSaveDevice.addEventListener("click", onSaveToDeviceClick);
	el.btnSavePreset.addEventListener("click", () => savePreset(model));
	el.inputLoadPreset.addEventListener("change", onLoadPresetFileChange);
	el.btnFactoryReset.addEventListener("click", onFactoryResetClick);
	el.btnSetActiveBank.addEventListener("click", onSetActiveBankClick);
	el.btnCloseDetail.addEventListener("click", closeDetailPanel);
	for (const tab of el.bankTabs) {
		tab.addEventListener("click", () => setViewingBank(Number(tab.dataset.bank)));
	}

	renderEncoderGrid();
	renderSideSwitches();
	setViewingBank(0);
}

async function onConnectClick() {
	setStatus("connecting", "Connecting…");
	if (device) device.destroy(); // stop any stale prior connection's heartbeat
	try {
		device = await midi.connect();
		protocol = new Protocol(device);
		device.onDisconnect((reason) => onDeviceDisconnected(reason));
		const info = await protocol.getDeviceInfo();
		model.deviceInfo = info;
		setStatus("connected", `Connected: ${device.name}`);
		el.fwVersion.textContent = `fw ${info.fwVersion}`;
		el.btnLoadDevice.disabled = false;
		el.btnSaveDevice.disabled = false;
		el.btnFactoryReset.disabled = false;
		el.btnSetActiveBank.disabled = false;
		el.btnConnect.textContent = "Reconnect";

		if (info.numEncoders !== NUM_ENCODERS || info.numBanks !== NUM_BANKS) {
			toast(
				"warn",
				`Device reports ${info.numEncoders} encoders / ${info.numBanks} banks, ` +
					`but this GUI assumes ${NUM_ENCODERS}/${NUM_BANKS}. Some controls may not line up.`,
			);
		}
	} catch (e) {
		setStatus("error", "Connection failed");
		toast("error", e.message);
	}
}

async function onLoadFromDeviceClick() {
	if (!protocol) return;
	await withButtonBusy(el.btnLoadDevice, "Loading…", async () => {
		// Heartbeat paused for the duration - a ping queued behind ~200
		// sequential bulk requests could time out purely from being busy,
		// and every one of those requests succeeding is itself much
		// stronger evidence of aliveness than a single ping would be. See
		// Device.pauseHeartbeat()'s doc comment in midi.js.
		device.pauseHeartbeat();
		try {
			await model.loadFromDevice(protocol, (done, total) => {
				el.btnLoadDevice.textContent = `Loading… ${Math.round((done / total) * 100)}%`;
			});
			model.activeBank = await protocol.getActiveBank();
			setViewingBank(model.activeBank);
			renderEncoderGrid();
			renderSideSwitches();
			toast("success", "Loaded configuration from device.");
		} finally {
			device.resumeHeartbeat();
		}
	});
}

async function onSaveToDeviceClick() {
	if (!protocol) return;
	await withButtonBusy(el.btnSaveDevice, "Saving…", async () => {
		device.pauseHeartbeat();
		try {
			await model.saveToDevice(protocol, (done, total) => {
				el.btnSaveDevice.textContent = `Saving… ${Math.round((done / total) * 100)}%`;
			});
			toast("success", "Saved configuration to device.");
		} finally {
			device.resumeHeartbeat();
		}
	});
}

async function onFactoryResetClick() {
	if (!protocol) return;
	if (!confirm("Factory reset the device? This wipes its stored configuration back to defaults.")) {
		return;
	}
	await withButtonBusy(el.btnFactoryReset, "Resetting…", async () => {
		expectingDisconnect = true;
		await protocol.factoryResetDevice();
		toast("success", "Factory reset triggered. Reconnecting…");
		await waitAndReconnect();
	});
}

/** Fires from Device.onDisconnect() - an *unplanned* disconnect (device
 * unplugged, went to sleep, or stopped responding to the heartbeat while
 * idle). Reset flows set expectingDisconnect themselves and call
 * waitAndReconnect() directly, so this only reconnects for the case
 * nothing else was already handling. */
function onDeviceDisconnected(reason) {
	if (expectingDisconnect) return;
	setStatus("error", "Disconnected");
	toast("error", `Device disconnected (${reason}). Reconnecting…`);
	waitAndReconnect();
}

async function onSetActiveBankClick() {
	if (!protocol) return;
	await withButtonBusy(el.btnSetActiveBank, "Setting…", async () => {
		await protocol.setActiveBank(viewingBank);
		model.activeBank = viewingBank;
		renderBankSelector();
		toast("success", `Bank ${viewingBank + 1} is now active on the device.`);
	});
}

async function onLoadPresetFileChange() {
	const file = el.inputLoadPreset.files?.[0];
	if (!file) return;
	try {
		await loadPreset(model, file);
		renderEncoderGrid();
		renderSideSwitches();
		if (selectedEncoderIdx !== null) renderDetailPanel();
		toast("success", `Loaded preset "${file.name}". Use "Save to device" to apply it.`);
	} catch (e) {
		toast("error", e.message);
	} finally {
		el.inputLoadPreset.value = "";
	}
}

/** Device drops off the bus after a reset (or an unplanned disconnect) and
 * re-enumerates - poll until it's reachable again rather than assuming a
 * fixed delay. */
async function waitAndReconnect() {
	if (device) device.destroy(); // stop the old instance's heartbeat/listeners
	await sleep(2000);
	for (let attempt = 0; attempt < 20; attempt++) {
		try {
			device = await midi.connect();
			protocol = new Protocol(device);
			device.onDisconnect((reason) => onDeviceDisconnected(reason));
			const info = await protocol.getDeviceInfo();
			model.deviceInfo = info;
			setStatus("connected", `Connected: ${device.name}`);
			el.fwVersion.textContent = `fw ${info.fwVersion}`;
			toast("success", "Reconnected.");
			expectingDisconnect = false;
			return;
		} catch {
			await sleep(1500);
		}
	}
	setStatus("error", "Lost connection");
	toast("error", "Device did not come back after reset. Reconnect manually.");
	expectingDisconnect = false;
}

function sleep(ms) {
	return new Promise((resolve) => setTimeout(resolve, ms));
}

async function withButtonBusy(button, busyText, fn) {
	const original = button.textContent;
	button.disabled = true;
	button.textContent = busyText;
	try {
		await fn();
	} catch (e) {
		toast("error", e.message);
	} finally {
		button.disabled = false;
		button.textContent = original;
	}
}

function setStatus(state, text) {
	el.statusDot.className = `status-dot status-dot--${state === "connecting" ? "disconnected" : state}`;
	el.statusText.textContent = text;
}

function toast(kind, message) {
	const t = document.createElement("div");
	t.className = `toast toast--${kind === "warn" ? "error" : kind}`;
	t.textContent = message;
	el.toastContainer.appendChild(t);
	setTimeout(() => t.remove(), 6000);
}

// --- Bank selector -----------------------------------------------------

function setViewingBank(bank) {
	viewingBank = bank;
	renderBankSelector();
	renderEncoderGrid();
	if (selectedEncoderIdx !== null) renderDetailPanel();
}

function renderBankSelector() {
	for (const tab of el.bankTabs) {
		tab.classList.toggle("is-active", Number(tab.dataset.bank) === viewingBank);
	}
	el.activeBankIndicator.textContent =
		model.activeBank === viewingBank
			? "(active on device)"
			: `(device is on Bank ${model.activeBank + 1})`;
}

// --- Encoder grid --------------------------------------------------------
// 4x4 grid, laid out in reading order (top-left to bottom-right) - but the
// firmware's own encoder index runs the opposite direction physically on
// the actual hardware (confirmed directly against a real device: the
// wiki/Technical.md docs describing index 0 as top-left describe the
// bootloader corner-gesture mapping, which is a different, unrelated
// index space from the physical left-to-right/top-to-bottom reading order
// the rest of this GUI assumes). visualPositionToFirmwareIndex() is the
// single place that translates between "the cell drawn at grid position
// p, reading order" and "the encoder index every sysex call and
// model.banks[].encoders[] array actually uses" - the data model itself
// stays in firmware-index order throughout; only rendering and click
// handling need to know about the visual reversal.
function visualPositionToFirmwareIndex(position) {
	return NUM_ENCODERS - 1 - position;
}

// Renders each cell with the design-system's buildEncoder() (see
// ../design-system/README.md) instead of the flat CSS ring this grid
// used before - same component the standalone twin.html preview uses,
// driven here by real DeviceModel state instead of demo sliders.
//
// One live-state gap: the sysex protocol has no "read the knob's current
// rotation" param (only VMAP_POSITION, the *configured window* the vmap
// occupies - see device-model.js's doc comment and MF_SYSEX_PARAM_VMAP_*
// in src/include/midi/sysex.h) and this app doesn't listen for regular
// (non-sysex) MIDI CC/note messages either, so there is no live position
// to render. Indicator LEDs are drawn at ENC_MID (dead centre) as a
// neutral "not turned" default rather than fabricating motion - only
// colour, detent, and display-mode genuinely reflect the device/model.
function renderEncoderGrid() {
	el.encoderGrid.innerHTML = "";
	const bank = model.banks[viewingBank];
	for (let position = 0; position < NUM_ENCODERS; position++) {
		const i = visualPositionToFirmwareIndex(position);
		const enc = bank.encoders[i];
		const activeVmap = enc.vmaps[enc.vmapActive] ?? enc.vmaps[0];

		const cell = document.createElement("div");
		cell.className = "encoder-cell encoder-cell--twin";
		cell.dataset.encoderIdx = String(i);

		const litMask = computeLitMask({
			position: ENC_MID,
			displayMode: enc.displayMode,
			detent: enc.detent,
		});

		cell.appendChild(
			buildEncoder({
				bodySize: 96,
				knobSize: 60,
				ledCount: 11,
				ledRadius: 39,
				ledSize: 8,
				ledArcSpan: 270,
				arcRadius: 39,
				arcWidth: 8,
				arcLength: 26,
				litMask,
				rgbColor: hsvToCss(activeVmap.hsv.hue, activeVmap.hsv.sat, activeVmap.hsv.val),
				selected: i === selectedEncoderIdx,
				showLabel: true,
				label: String(i),
				onSelect: () => selectEncoder(i),
			}),
		);

		el.encoderGrid.appendChild(cell);
	}
}

function selectEncoder(idx) {
	selectedEncoderIdx = idx;
	selectedVmapIdx = 0;
	renderEncoderGrid();
	renderDetailPanel();
	el.detailPanel.hidden = false;
}

function closeDetailPanel() {
	el.detailPanel.hidden = true;
	selectedEncoderIdx = null;
	renderEncoderGrid();
}

// --- Detail panel --------------------------------------------------------

function renderDetailPanel() {
	if (selectedEncoderIdx === null) return;
	const enc = model.banks[viewingBank].encoders[selectedEncoderIdx];
	el.detailPanelTitle.textContent = `Encoder ${selectedEncoderIdx} (Bank ${viewingBank + 1})`;
	el.detailPanelBody.innerHTML = "";

	el.detailPanelBody.appendChild(buildEncoderSettingsGroup(enc));
	el.detailPanelBody.appendChild(buildVmapTabs(enc));
	el.detailPanelBody.appendChild(buildVmapGroup(enc, enc.vmaps[selectedVmapIdx]));
}

function buildEncoderSettingsGroup(enc) {
	const group = document.createElement("div");
	group.className = "field-group";
	group.appendChild(h3("Encoder"));

	group.appendChild(
		selectField("Display mode", enc.displayMode, DisplayMode, (value) => {
			enc.displayMode = value;
			model.markDirty(`${viewingBank}.${selectedEncoderIdx}.displayMode`);
		}),
	);

	group.appendChild(
		checkboxField("Detent", enc.detent, (checked) => {
			enc.detent = checked;
			model.markDirty(`${viewingBank}.${selectedEncoderIdx}.detent`);
		}),
	);

	group.appendChild(
		selectField("Switch mode", enc.switchMode, SwitchMode, (value) => {
			enc.switchMode = value;
			model.markDirty(`${viewingBank}.${selectedEncoderIdx}.switchMode`);
		}),
	);

	return group;
}

function buildVmapTabs(enc) {
	const wrap = document.createElement("div");
	wrap.className = "vmap-tabs";
	for (let v = 0; v < NUM_VMAPS_PER_ENCODER; v++) {
		const tab = document.createElement("button");
		tab.type = "button";
		tab.className = "vmap-tab";
		tab.textContent = `Layer ${String.fromCharCode(65 + v)}`; // A, B
		if (v === selectedVmapIdx) tab.classList.add("is-active");
		if (v === enc.vmapActive) tab.textContent += " •"; // bullet marks the device-active layer
		tab.addEventListener("click", () => {
			selectedVmapIdx = v;
			renderDetailPanel();
		});
		wrap.appendChild(tab);
	}
	return wrap;
}

function buildVmapGroup(enc, vmap) {
	const group = document.createElement("div");
	group.className = "field-group";
	group.appendChild(h3(`Layer ${String.fromCharCode(65 + selectedVmapIdx)}`));

	const swatch = document.createElement("div");
	swatch.className = "color-swatch";
	const updateSwatch = () => {
		swatch.style.background = hsvToCss(vmap.hsv.hue, vmap.hsv.sat, vmap.hsv.val);
	};
	updateSwatch();
	group.appendChild(swatch);

	group.appendChild(
		rangeField("Hue", vmap.hsv.hue, 0, 1535, (v) => {
			vmap.hsv.hue = v;
			updateSwatch();
			markVmapDirty("hsv");
		}),
	);
	group.appendChild(
		rangeField("Saturation", vmap.hsv.sat, 0, 255, (v) => {
			vmap.hsv.sat = v;
			updateSwatch();
			markVmapDirty("hsv");
		}),
	);
	group.appendChild(
		rangeField("Value", vmap.hsv.val, 0, 255, (v) => {
			vmap.hsv.val = v;
			updateSwatch();
			markVmapDirty("hsv");
		}),
	);

	group.appendChild(
		numberField("Range lower", vmap.range.lower, -128, 127, (v) => {
			vmap.range.lower = v;
			markVmapDirty("range");
		}),
	);
	group.appendChild(
		numberField("Range upper", vmap.range.upper, -128, 127, (v) => {
			vmap.range.upper = v;
			markVmapDirty("range");
		}),
	);
	group.appendChild(
		numberField("Position start", vmap.position.start, 0, 255, (v) => {
			vmap.position.start = v;
			markVmapDirty("position");
		}),
	);
	group.appendChild(
		numberField("Position stop", vmap.position.stop, 0, 255, (v) => {
			vmap.position.stop = v;
			markVmapDirty("position");
		}),
	);

	group.appendChild(
		selectField(
			"MIDI mode",
			vmap.proto.mode,
			// CC_14/REL_CC excluded - firmware doesn't actually transmit them
			// yet (see module-architecture skill). Don't offer a control that
			// silently does nothing.
			{ DISABLED: MidiMode.DISABLED, CC: MidiMode.CC, NOTE: MidiMode.NOTE },
			(v) => {
				vmap.proto.mode = v;
				markVmapDirty("proto");
			},
		),
	);
	group.appendChild(
		numberField("MIDI channel", vmap.proto.channel, 0, 15, (v) => {
			vmap.proto.channel = v;
			markVmapDirty("proto");
		}),
	);
	group.appendChild(
		numberField("CC / note number", vmap.proto.ccOrRaw, 0, 127, (v) => {
			vmap.proto.ccOrRaw = v;
			markVmapDirty("proto");
		}),
	);

	return group;
}

function markVmapDirty(field) {
	model.markDirty(`${viewingBank}.${selectedEncoderIdx}.${selectedVmapIdx}.${field}`);
	renderEncoderGrid(); // ring color may have changed
}

// --- Side switches ---------------------------------------------------

function renderSideSwitches() {
	el.sideSwitchRow.innerHTML = "";
	for (let i = 0; i < NUM_SIDE_SWITCHES; i++) {
		const wrap = document.createElement("div");
		wrap.className = "side-switch";

		const label = document.createElement("label");
		label.textContent = `SW ${i + 1}`;
		label.htmlFor = `side-switch-${i}`;
		wrap.appendChild(label);

		const select = document.createElement("select");
		select.id = `side-switch-${i}`;
		for (const [name, value] of Object.entries(SideSwitchMode)) {
			const opt = document.createElement("option");
			opt.value = String(value);
			opt.textContent = toTitleCase(name);
			select.appendChild(opt);
		}
		select.value = String(model.sideSwitches[i]);
		select.addEventListener("change", () => {
			model.sideSwitches[i] = Number(select.value);
			model.markDirty(`sideSwitch.${i}`);
		});
		wrap.appendChild(select);

		el.sideSwitchRow.appendChild(wrap);
	}
}

// --- Small field builders ----------------------------------------------

function h3(text) {
	const h = document.createElement("h3");
	h.textContent = text;
	return h;
}

function selectField(labelText, value, enumObj, onChange) {
	const field = document.createElement("div");
	field.className = "field";
	const label = document.createElement("label");
	label.textContent = labelText;
	field.appendChild(label);

	const select = document.createElement("select");
	for (const [name, v] of Object.entries(enumObj)) {
		const opt = document.createElement("option");
		opt.value = String(v);
		opt.textContent = toTitleCase(name);
		select.appendChild(opt);
	}
	select.value = String(value);
	select.addEventListener("change", () => onChange(Number(select.value)));
	field.appendChild(select);
	return field;
}

function checkboxField(labelText, checked, onChange) {
	const field = document.createElement("div");
	field.className = "field";
	const label = document.createElement("label");
	label.textContent = labelText;
	field.appendChild(label);

	const input = document.createElement("input");
	input.type = "checkbox";
	input.checked = checked;
	input.addEventListener("change", () => onChange(input.checked));
	field.appendChild(input);
	return field;
}

function numberField(labelText, value, min, max, onChange) {
	const field = document.createElement("div");
	field.className = "field";
	const label = document.createElement("label");
	label.textContent = labelText;
	field.appendChild(label);

	const input = document.createElement("input");
	input.type = "number";
	input.min = String(min);
	input.max = String(max);
	input.value = String(value);
	input.addEventListener("change", () => {
		const v = clamp(Number(input.value), min, max);
		input.value = String(v);
		onChange(v);
	});
	field.appendChild(input);
	return field;
}

function rangeField(labelText, value, min, max, onChange) {
	const field = document.createElement("div");
	field.className = "field";
	const label = document.createElement("label");
	label.textContent = labelText;
	field.appendChild(label);

	const input = document.createElement("input");
	input.type = "range";
	input.min = String(min);
	input.max = String(max);
	input.value = String(value);
	input.addEventListener("input", () => onChange(Number(input.value)));
	field.appendChild(input);
	return field;
}

function toTitleCase(constName) {
	return constName
		.toLowerCase()
		.split("_")
		.map((w) => w[0].toUpperCase() + w.slice(1))
		.join(" ");
}

function clamp(v, lo, hi) {
	return Math.min(hi, Math.max(lo, v));
}

init();
