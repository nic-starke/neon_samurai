// Live device view for index.html - connect, and watch the physical
// Twister's real state render as the digital twin. No editing UI - this
// page is a passive viewer only.

import { hsvToCss } from "./color.js";
import { DeviceModel, NUM_BANKS, NUM_ENCODERS } from "./device-model.js";
import * as midi from "./midi.js";
import { Protocol } from "./protocol.js";
import { LivePositionTracker } from "./live-position.js";
import { LiveVmapActiveTracker } from "./live-vmap-active.js";
import {
	buildDeviceChassis,
	computeLitMask,
	computeLedBrightness,
	computeDetentColorOverride,
	LedDisplayMode,
	ENC_MID,
	ENC_MAX,
} from "../design-system/components/index.js";

// Matches twin.js's SPEC - duplicated rather than imported since that
// file's copy is mutable (tuning sidebar) and this page has none.
const GEOMETRY = {
	pitch: 136,
	edgeFirst: 92,
	cornerRadius: 36,
	bevelWidth: 14,
	bodySize: 112,
	knobSize: 71,
	capBaseDia: 18.5,
	capGripDiaBottom: 15,
	capGripDiaTop: 13.5,
	capRibCount: 19,
	capInnerDia: 11.3,
	lightX1: -35,
	lightY1: -45,
	lightX2: 130,
	lightY2: 145,
	ledCount: 11,
	ledRadius: 46,
	ledSize: 10,
	ledArcSpan: 270,
	arcRadius: 46,
	arcWidth: 10,
	arcLength: 32,
	sideBtnW: 6,
	sideBtnH: 39,
	sideBtnSpacing: 76,
	sideBtnOffsetY: 0,
};

const model = new DeviceModel();
let device = null;
let protocol = null;
let connected = false;
let viewingBank = 0; // which bank is currently shown - follows model.activeBank once connected
const livePosition = new LivePositionTracker();
const liveVmapActive = new LiveVmapActiveTracker();
let expectingDisconnect = false;

const el = {
	unsupportedBanner: byId("unsupported-banner"),
	statusDot: byId("status-dot"),
	statusText: byId("status-text"),
	fwVersion: byId("fw-version"),
	btnConnect: byId("btn-connect"),
	chassis: byId("twin-chassis"),
	toastContainer: byId("toast-container"),
};

function byId(id) {
	return document.getElementById(id);
}

function init() {
	if (!midi.isSupported()) {
		el.unsupportedBanner.hidden = false;
		el.btnConnect.disabled = true;
	}
	el.btnConnect.addEventListener("click", onConnectClick);
	renderChassis();
}

// No separate "Load from device" action - the full config is pulled
// immediately on every successful connect/reconnect, before live
// tracking starts.
async function onConnectClick() {
	setStatus("connecting", "Connecting…");
	el.btnConnect.disabled = true;
	if (device) device.destroy();
	try {
		device = await midi.connect();
		protocol = new Protocol(device);
		device.onDisconnect((reason) => onDeviceDisconnected(reason));

		const info = await protocol.getDeviceInfo();
		model.deviceInfo = info;
		setStatus("connecting", "Loading configuration…");

		await model.loadFromDevice(protocol, (done, total) => {
			setStatus("connecting", `Loading configuration… ${Math.round((done / total) * 100)}%`);
		});
		model.activeBank = await protocol.getActiveBank();
		viewingBank = model.activeBank;

		livePosition.reset();
		livePosition.seed(model); // last-known position from the config pull, before any live push has arrived
		livePosition.attach(device, renderChassis);
		liveVmapActive.reset();
		liveVmapActive.seed(model);
		liveVmapActive.attach(device, renderChassis);
		await protocol.setLivePositionStreaming(true);

		connected = true;
		setStatus("connected", `Connected: ${device.name}`);
		el.fwVersion.textContent = `fw ${info.fwVersion}`;
		el.btnConnect.textContent = "Reconnect";

		if (info.numEncoders !== NUM_ENCODERS || info.numBanks !== NUM_BANKS) {
			toast(
				"warn",
				`Device reports ${info.numEncoders} encoders / ${info.numBanks} banks, ` +
					`but this twin assumes ${NUM_ENCODERS}/${NUM_BANKS}. Some encoders may not line up.`,
			);
		}
		renderChassis();
	} catch (e) {
		connected = false;
		setStatus("error", "Connection failed");
		toast("error", e.message);
		renderChassis();
	} finally {
		el.btnConnect.disabled = false;
	}
}

// Fires from Device.onDisconnect() - an *unplanned* disconnect. No
// setLivePositionStreaming(false) here - the device is already gone, so
// there's nothing to send it to; the firmware's own flag resets to off
// on its next reboot regardless (see gRT's initializer in main.c).
function onDeviceDisconnected(reason) {
	if (expectingDisconnect) return;
	connected = false;
	livePosition.detach();
	liveVmapActive.detach();
	setStatus("error", "Disconnected");
	toast("error", `Device disconnected (${reason}).`);
	renderChassis();
}

// Best-effort: stop the device streaming before the tab goes away, so a
// device that stays powered (rather than being unplugged) doesn't keep
// pushing sysex to a client that's no longer listening. Not guaranteed
// to complete - the page may already be gone before the request lands -
// but harmless to attempt, and correct behaviour when it does land in
// time (most browsers give beforeunload handlers a brief grace window).
window.addEventListener("beforeunload", () => {
	if (connected && protocol) {
		protocol.setLivePositionStreaming(false).catch(() => {});
	}
});

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

// Grid is laid out in reading order (top-left to bottom-right), but the
// firmware's encoder index runs the opposite direction on the actual
// hardware (confirmed against a real device). This is the single place
// that translates between the two.
function visualPositionToFirmwareIndex(position) {
	return NUM_ENCODERS - 1 - position;
}

// Maps curr_pos (0-255) onto the same angular sweep the indicator LED
// ring uses (see led-ring.js: LEDs run -span/2..+span/2 across the
// count), so the cap's visual rotation lines up with which indicator
// LEDs are lit rather than sweeping a different arc.
function positionToKnobRotation(position) {
	return -(GEOMETRY.ledArcSpan / 2) + (position / ENC_MAX) * GEOMETRY.ledArcSpan;
}

const ENCODER_GEOMETRY_PROPS = {
	knobSize: GEOMETRY.knobSize,
	capBaseDia: GEOMETRY.capBaseDia,
	capGripDiaBottom: GEOMETRY.capGripDiaBottom,
	capGripDiaTop: GEOMETRY.capGripDiaTop,
	capRibCount: GEOMETRY.capRibCount,
	capInnerDia: GEOMETRY.capInnerDia,
	ledCount: GEOMETRY.ledCount,
	ledSize: GEOMETRY.ledSize,
	ledRadius: GEOMETRY.ledRadius,
	ledArcSpan: GEOMETRY.ledArcSpan,
	arcRadius: GEOMETRY.arcRadius,
	arcWidth: GEOMETRY.arcWidth,
	arcLength: GEOMETRY.arcLength,
	showLabel: false,
};

function renderChassis() {
	const bank = model.banks[viewingBank];

	const { el: chassisEl } = buildDeviceChassis(
		GEOMETRY,
		(position) => {
			const i = visualPositionToFirmwareIndex(position);

			if (!connected) {
				return {
					...ENCODER_GEOMETRY_PROPS,
					knobRotation: 0,
					litMask: new Array(GEOMETRY.ledCount).fill(false),
					rgbOff: true,
					powered: false,
				};
			}

			const enc = bank.encoders[i];
			const liveActive = liveVmapActive.getActive(viewingBank, i);
			const activeVmapIdx = enc.vmaps[liveActive] ? liveActive : enc.vmaps[enc.vmapActive] ? enc.vmapActive : 0;
			const activeVmap = enc.vmaps[activeVmapIdx];
			const livePos = livePosition.getPosition(viewingBank, i, activeVmapIdx) ?? activeVmap.currPos ?? ENC_MID;
			const maskArgs = { position: livePos, displayMode: enc.displayMode, detent: enc.detent };

			return {
				...ENCODER_GEOMETRY_PROPS,
				knobRotation: positionToKnobRotation(livePos),
				// MULTI_PWM's leading-LED brightness needs the continuous
				// per-LED value (computeLedBrightness); every other mode
				// only needs on/off, so litMask stays the cheaper path -
				// led-ring.js prefers `brightness` over `litMask` when both
				// are given, so passing both here is safe/redundant, not
				// conflicting.
				litMask: computeLitMask(maskArgs),
				ledBrightness: enc.displayMode === LedDisplayMode.MULTI_PWM ? computeLedBrightness(maskArgs) : undefined,
				rgbColor: hsvToCss(activeVmap.hsv.hue, activeVmap.hsv.sat, activeVmap.hsv.val),
				rgbOff: false,
				ledColorOverride: computeDetentColorOverride({ position: livePos, detent: enc.detent, rb: activeVmap.rb }),
				vmapCount: enc.vmaps.length,
				vmapActive: activeVmapIdx,
			};
		},
		(side, i) => {
			const swIdx = (side === "L" ? 0 : 3) + i;
			return { pressed: connected && sideSwitchPressed(swIdx) };
		},
	);

	el.chassis.replaceChildren(chassisEl);
}

// No sysex param exposes live side-switch press state (only SIDE_SWITCH
// mode config) - always false until the protocol adds one.
function sideSwitchPressed(_swIdx) {
	return false;
}

init();
