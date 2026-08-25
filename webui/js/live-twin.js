// Live device view for index.html. Connect, and watch the physical Twister's
// state render as the digital twin. Read-only apart from the bank selector.
//
// Renders are coalesced to one animation frame and only encoders whose props
// actually changed are rebuilt, so a knob turn touches one encoder per frame
// rather than the whole chassis per sysex push.

import { hsvToCss, hsvToRgb } from "./color.js";
import { DeviceModel, NUM_BANKS, NUM_ENCODERS } from "./device-model.js";
import * as midi from "./midi.js";
import { Param } from "./sysex.js";
import { Protocol } from "./protocol.js";
import { LivePushTracker } from "./live-tracker.js";
import { encoderSignature } from "./encoder-signature.js";
import { BankFade } from "./bank-fade.js";
import { GEOMETRY } from "../design-system/geometry.js";
import { loadManual, helpIcon } from "../design-system/components/help.js";
import { UpdateDialog } from "../design-system/components/update-dialog.js";
import {
  fetchManifest,
  checkForUpdate,
  runUpdate,
  watchForBootloader,
} from "./firmware-update.js";
import * as dfu from "../dfu/xmega-dfu.js";
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
const bankFade = new BankFade();

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
  btnUpdate: byId("btn-update"),
  btnDisconnect: byId("btn-disconnect"),
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
  el.btnUpdate.addEventListener("click", onUpdateClick);
  el.btnDisconnect.addEventListener("click", onDisconnectClick);
  bankFade.onFrame = scheduleRender;
  buildChassisOnce();
  renderBankSelector();

  // The manual is fetched rather than bundled, so the page is usable before
  // it arrives - the help icons simply appear once it has.
  loadManual().then(renderBankSelector);

  watchForBootloader({
    onPresent: onBootloaderPresent,
    onGone: onBootloaderGone,
  });
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
      setStatus(
        "connecting",
        `Loading configuration… ${Math.round((done / total) * 100)}%`
      );
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
    el.btnDisconnect.disabled = false;
    setStatus("connected", `Connected: ${device.name}`);
    el.btnConnect.textContent = "Reconnect";

    refreshUpdateButton(info.fwVersion);

    if (info.numEncoders !== NUM_ENCODERS || info.numBanks !== NUM_BANKS) {
      toast(
        "warn",
        `Device reports ${info.numEncoders} encoders / ${info.numBanks} banks, ` +
          `but this twin assumes ${NUM_ENCODERS}/${NUM_BANKS}. Some encoders may not line up.`
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

/**
 * Let go of the device.
 *
 * Worth having a button for beyond tidiness: a rawmidi node has a single
 * opener, so anything else that wants the device - another application, or
 * the browser claiming it as a DFU device - cannot have it until this page
 * releases it.
 */
async function onDisconnectClick() {
  el.btnDisconnect.disabled = true;

  if (protocol) {
    // Stop the device streaming to a page that is no longer listening.
    await protocol.setLivePositionStreaming(false).catch(() => {});
  }

  connected = false;
  pendingBank = null;
  livePosition.detach();
  liveVmapActive.detach();
  liveActiveBank.detach();
  bankFade.stop();

  if (device) device.destroy();
  device = null;
  protocol = null;

  setStatus("disconnected", "Not connected");
  el.btnConnect.textContent = "Connect";
  el.fwVersion.textContent = "";
  setUpdateButton("Update", false);

  scheduleRender();
  renderBankSelector();
}

function onDeviceDisconnected(reason) {
  connected = false;
  el.btnDisconnect.disabled = true;
  setUpdateButton("Update", false);
  pendingBank = null;
  livePosition.detach();
  liveVmapActive.detach();
  liveActiveBank.detach();
  bankFade.stop();
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

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~ Firmware updates ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

let pendingUpdate = null;

// Set while a bootloader this origin can talk to is attached. Only ever true
// after the user has granted the device once - see watchForBootloader().
let bootloaderPresent = false;
let updateRunning = false;

/*
	Offer the update whatever the device is running.

	For testing the flow without building a higher version every time. A URL
	flag rather than a constant, so it cannot be left switched on by accident
	in what gets published.
*/
function isForced() {
	return new URLSearchParams(location.search).has("forceUpdate");
}

/**
 * A bootloader turned up.
 *
 * Either an update is running - in which case the update itself is handling
 * it - or the device is sitting in DFU from an interrupted attempt or the
 * encoder gesture, and the user is offered a way to finish.
 */
async function onBootloaderPresent() {
  bootloaderPresent = true;
  if (updateRunning) return;

  const manifest = await fetchManifest();
  if (!manifest?.version) {
    setUpdateButton("Bootloader mode - no firmware available", false);
    return;
  }

  pendingUpdate = { available: true, version: manifest.version, manifest };
  setUpdateButton(`Finish update: v${manifest.version}`, true);
  setStatus("connecting", "Device is in bootloader mode");
}

function onBootloaderGone() {
  bootloaderPresent = false;
  if (updateRunning) return;

  // Leave the normal connected/disconnected handling to say what is true now.
  if (!connected) setUpdateButton("Update", false);
}

function setUpdateButton(text, available) {
  el.btnUpdate.textContent = text;
  el.btnUpdate.disabled = !available;
  el.btnUpdate.classList.toggle("fw-update-btn--available", available);
}

/**
 * Work out whether the firmware this site carries is newer than the device's.
 *
 * The manifest is fetched from this origin rather than from GitHub: release
 * asset downloads carry no CORS headers, so a browser cannot read one however
 * the request is framed.
 */
async function refreshUpdateButton(deviceVersion) {
  setUpdateButton("Checking…", false);

  const manifest = await fetchManifest();
  const result = checkForUpdate(deviceVersion, manifest, { force: isForced() });

  if (result.available) {
    pendingUpdate = result;
    setUpdateButton(
      result.forced
        ? `Reflash v${result.version}`
        : `Firmware update: v${result.version}`,
      true
    );
  } else {
    pendingUpdate = null;
    setUpdateButton(
      result.reason === "no-manifest" ? "No firmware available" : "Firmware up to date",
      false
    );
  }
}

async function onUpdateClick() {
  if (!pendingUpdate) return;

  const dialog = new UpdateDialog({
    version: pendingUpdate.version,
    onConfirm: () => startUpdate(dialog),
  });

  dialog.open();
}

async function startUpdate(dialog) {
  updateRunning = true;
  try {
    const response = await fetch(pendingUpdate.manifest.file, { cache: "no-store" });
    if (!response.ok) throw new Error(`could not fetch the firmware (${response.status})`);
    const hexText = await response.text();

    const version = await runUpdate({
      hexText,

      // Nothing to send a sysex command to if it is already there.
      skipBootloader: bootloaderPresent,

      /*
        The device has to be off the MIDI bus before the browser can claim its
        DFU interface - a rawmidi node has a single opener, and Web MIDI is
        holding it. So streaming is stopped, the command sent, and the handle
        dropped, in that order.
      */
      enterBootloader: async () => {
        await protocol.setLivePositionStreaming(false).catch(() => {});
        await protocol.enterBootloader();

        connected = false;
        livePosition.detach();
        liveVmapActive.detach();
        liveActiveBank.detach();
        if (device) device.destroy();
        device = null;
        protocol = null;

        setStatus("connecting", "Device is in bootloader mode…");
        scheduleRender();
      },

      // requestDevice() only works inside a user gesture, so the dialog puts
      // a button in front of the picker rather than calling it from here.
      requestDevice: () => dialog.requestDevicePrompt(() => dfu.requestDevice()),

      reconnect: async () => {
        // The device re-enumerates, which takes a moment, and Web MIDI needs
        // to notice before a port can be opened.
        for (let attempt = 0; attempt < 20; attempt++) {
          try {
            device = await midi.connect();
            protocol = new Protocol(device);
            return await protocol.getDeviceInfo();
          } catch {
            await new Promise((r) => setTimeout(r, 500));
          }
        }
        throw new Error("the device did not come back - unplug it and plug it in again");
      },

      onStep: (step, state, detail) => dialog.setStep(step, state, detail),
      onProgress: (done, total) => dialog.setProgress(done, total),
    });

    dialog.showComplete(version);

    // Reload the configuration from the device now it is running again.
    await onConnectClick();
  } catch (e) {
    dialog.showFailure(e.message);
    setStatus("error", "Firmware update failed");
  } finally {
    updateRunning = false;
  }
}

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
  bankFade.start(newBank);
  renderBankSelector();
}

async function switchBank(bank) {
  if (!connected || bank === liveActiveBank.get() || pendingBank !== null)
    return;
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
  return (
    -(GEOMETRY.ledArcSpan / 2) + (position / ENC_MAX) * GEOMETRY.ledArcSpan
  );
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
  const activeVmapIdx = enc.vmaps[liveActive]
    ? liveActive
    : enc.vmaps[enc.vmapActive]
    ? enc.vmapActive
    : 0;
  const activeVmap = enc.vmaps[activeVmapIdx];
  const livePos =
    livePosition.get(viewingBank, i, activeVmapIdx) ??
    activeVmap.currPos ??
    ENC_MID;
  const maskArgs = {
    position: livePos,
    displayMode: enc.displayMode,
    detent: enc.detent,
  };

  const faded = bankFade.sample(
    i,
    hsvToRgb(activeVmap.hsv.hue, activeVmap.hsv.sat, activeVmap.hsv.val)
  );
  const rgbColor = faded
    ? `rgb(${faded.r}, ${faded.g}, ${faded.b})`
    : hsvToCss(activeVmap.hsv.hue, activeVmap.hsv.sat, activeVmap.hsv.val);

  return {
    ...ENCODER_GEOMETRY_PROPS,
    knobRotation: positionToKnobRotation(livePos),
    litMask: computeLitMask(maskArgs),
    ledBrightness:
      enc.displayMode === LedDisplayMode.MULTI_PWM
        ? computeLedBrightness(maskArgs)
        : undefined,
    rgbColor,
    rgbOff: false,
    ledColorOverride: computeDetentColorOverride({
      position: livePos,
      detent: enc.detent,
      rb: activeVmap.rb,
    }),
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
    () => ({ pressed: false })
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
      })
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
    helpIcon("switching-bank")
  );
}

init();
