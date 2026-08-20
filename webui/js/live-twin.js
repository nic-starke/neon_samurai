// Live device view for index.html. Connect, and watch the physical Twister's
// state render as the digital twin. Read-only apart from the bank selector.
//
// Renders are coalesced to one animation frame and only encoders whose props
// actually changed are rebuilt, so a knob turn touches one encoder per frame
// rather than the whole chassis per sysex push.

import { hsvToCss } from "./color.js";
import { DeviceModel, NUM_BANKS, NUM_ENCODERS } from "./device-model.js";
import * as midi from "./midi.js";
import { Param } from "./sysex.js";
import { Protocol } from "./protocol.js";
import { LivePushTracker } from "./live-tracker.js";
import { encoderSignature } from "./encoder-signature.js";
import { BankFlicker } from "./bank-flicker.js";
import { GEOMETRY } from "../design-system/geometry.js";
import {
	buildDeviceChassis,
	buildEncoder,
	buildBankSelector,
	computeLitMask,
	computeLedBrightness,
	computeDetentColorOverride,
	LedDisplayMode,
	ENC_MID,
	ENC_MAX,
} from "../design-system/components/index.js";

const model = new DeviceModel();
const livePosition = new LivePushTracker(Param.VMAP_CURR_POS, 3);
const liveVmapActive = new LivePushTracker(Param.ENCODER_VMAP_ACTIVE, 2);
const liveActiveBank = new LivePushTracker(Param.ACTIVE_BANK, 0);
const bankFlicker = new BankFlicker();

let device = null;
let protocol = null;
let connected = false;
let viewingBank = 0;
let pendingBank = null;
let chassis = null;
let renderPending = false;
const signatures = new Array(NUM_ENCODERS).fill(null);

const el = {
	unsupportedBanner: byId("unsupported-banner"),
	statusDot: byId("status-dot"),
	statusText: byId("status-text"),
	fwVersion: byId("fw-version"),
	btnConnect: byId("btn-connect"),
	chassis: byId("twin-chassis"),
	bankSelector: byId("twin-bank-selector"),
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
	bankFlicker.onFrame = scheduleRender;
	buildChassisOnce();
	renderBankSelector();
}

function scheduleRender() {
	if (renderPending) return;
	renderPending = true;
	requestAnimationFrame(() => {
		renderPending = false;
		renderChassis();
	});
}

async function onConnectClick() {
	setStatus("connecting", "Connecting…");
	el.btnConnect.disabled = true;
	if (device) device.destroy();
	try {
		device = await midi.connect();
		protocol = new Protocol(device);
		device.onDisconnect(onDeviceDisconnected);

		const info = await protocol.getDeviceInfo();
		model.deviceInfo = info;
		el.fwVersion.textContent = `fw ${info.fwVersion}`;
		setStatus("connecting", "Loading configuration…");

		await model.loadFromDevice(protocol, (done, total) => {
			setStatus("connecting", `Loading configuration… ${Math.round((done / total) * 100)}%`);
		});
		model.activeBank = await protocol.getActiveBank();
		viewingBank = model.activeBank;
		pendingBank = null;

		seedTrackers();
		livePosition.attach(device, scheduleRender);
		liveVmapActive.attach(device, scheduleRender);
		liveActiveBank.attach(device, onBankChanged);
		await protocol.setLivePositionStreaming(true);

		connected = true;
		setStatus("connected", `Connected: ${device.name}`);
		el.btnConnect.textContent = "Reconnect";

		if (info.numEncoders !== NUM_ENCODERS || info.numBanks !== NUM_BANKS) {
			toast(
				"warn",
				`Device reports ${info.numEncoders} encoders / ${info.numBanks} banks, ` +
					`but this twin assumes ${NUM_ENCODERS}/${NUM_BANKS}. Some encoders may not line up.`,
			);
		}
	} catch (e) {
		connected = false;
		setStatus("error", "Connection failed");
		toast("error", e.message);
	} finally {
		el.btnConnect.disabled = false;
		scheduleRender();
		renderBankSelector();
	}
}

function onDeviceDisconnected(reason) {
	connected = false;
	pendingBank = null;
	livePosition.detach();
	liveVmapActive.detach();
	liveActiveBank.detach();
	bankFlicker.stop();
	setStatus("error", "Disconnected");
	toast("error", `Device disconnected (${reason}). Reconnect to resume.`);
	scheduleRender();
	renderBankSelector();
}

window.addEventListener("beforeunload", () => {
	if (connected && protocol) {
		protocol.setLivePositionStreaming(false).catch(() => {});
	}
});

function seedTrackers() {
	livePosition.reset();
	liveVmapActive.reset();
	liveActiveBank.reset();
	model.banks.forEach((bank, b) => {
		bank.encoders.forEach((enc, e) => {
			liveVmapActive.set([b, e], enc.vmapActive);
			enc.vmaps.forEach((vmap, m) => livePosition.set([b, e, m], vmap.currPos));
		});
	});
	liveActiveBank.set([], model.activeBank);
}

function onBankChanged(newBank) {
	viewingBank = newBank;
	model.activeBank = newBank;
	pendingBank = null;
	bankFlicker.start(newBank);
	renderBankSelector();
}

async function switchBank(bank) {
	if (!connected || bank === liveActiveBank.get() || pendingBank !== null) return;
	pendingBank = bank;
	renderBankSelector();
	try {
		const rc = await protocol.setActiveBank(bank);
		if (rc !== 0) {
			throw new Error(`device rejected bank change (code ${rc})`);
		}
		if (liveActiveBank.get() !== bank) {
			liveActiveBank.set([], bank);
			onBankChanged(bank);
		}
	} catch (e) {
		pendingBank = null;
		toast("error", `Could not switch to bank ${bank + 1}: ${e.message}`);
		renderBankSelector();
	}
}

function setStatus(state, text) {
	el.statusDot.className = `status-dot status-dot--${state}`;
	el.statusText.textContent = text;
}

function toast(kind, message) {
	const t = document.createElement("div");
	t.className = `toast toast--${kind}`;
	t.setAttribute("role", kind === "error" ? "alert" : "status");
	t.textContent = message;
	el.toastContainer.appendChild(t);
	setTimeout(() => t.remove(), 6000);
}

function visualPositionToFirmwareIndex(position) {
	return NUM_ENCODERS - 1 - position;
}

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

const UNPOWERED_LIT_MASK = new Array(GEOMETRY.ledCount).fill(false);

function encoderPropsFor(position) {
	if (!connected) {
		return {
			...ENCODER_GEOMETRY_PROPS,
			knobRotation: 0,
			litMask: UNPOWERED_LIT_MASK,
			rgbOff: true,
			powered: false,
		};
	}

	const i = visualPositionToFirmwareIndex(position);
	const enc = model.banks[viewingBank].encoders[i];
	const liveActive = liveVmapActive.get(viewingBank, i);
	const activeVmapIdx = enc.vmaps[liveActive] ? liveActive : enc.vmaps[enc.vmapActive] ? enc.vmapActive : 0;
	const activeVmap = enc.vmaps[activeVmapIdx];
	const livePos = livePosition.get(viewingBank, i, activeVmapIdx) ?? activeVmap.currPos ?? ENC_MID;
	const maskArgs = { position: livePos, displayMode: enc.displayMode, detent: enc.detent };

	const flickering = bankFlicker.isFlickering(i);
	const rgbColor = flickering ? "#ffffff" : hsvToCss(activeVmap.hsv.hue, activeVmap.hsv.sat, activeVmap.hsv.val);

	return {
		...ENCODER_GEOMETRY_PROPS,
		knobRotation: positionToKnobRotation(livePos),
		litMask: computeLitMask(maskArgs),
		ledBrightness: enc.displayMode === LedDisplayMode.MULTI_PWM ? computeLedBrightness(maskArgs) : undefined,
		rgbColor,
		rgbOff: flickering ? !bankFlicker.isWhite(i) : false,
		ledColorOverride: computeDetentColorOverride({ position: livePos, detent: enc.detent, rb: activeVmap.rb }),
		vmapCount: enc.vmaps.length,
		vmapActive: activeVmapIdx,
	};
}

function buildChassisOnce() {
	// No sysex param exposes live side-switch press state, only its mode config.
	chassis = buildDeviceChassis(
		GEOMETRY,
		(position) => {
			const props = encoderPropsFor(position);
			signatures[position] = encoderSignature(props);
			return props;
		},
		() => ({ pressed: false }),
	);
	el.chassis.replaceChildren(chassis.el);
}

function renderChassis() {
	for (let position = 0; position < NUM_ENCODERS; position++) {
		const props = encoderPropsFor(position);
		const signature = encoderSignature(props);
		if (signature === signatures[position]) continue;
		signatures[position] = signature;
		const light = chassis.knurlLights[position];
		chassis.encoderCells[position].replaceChildren(
			buildEncoder({
				bodySize: chassis.bodySize,
				capLightAngle: light.angle,
				capLightOffset: light.offset,
				...props,
			}),
		);
	}
}

function renderBankSelector() {
	el.bankSelector.replaceChildren(
		buildBankSelector({
			count: NUM_BANKS,
			active: viewingBank,
			pending: pendingBank,
			onSelect: connected ? switchBank : undefined,
		}),
	);
}

init();
