// The editor - index.html's application shell and everything wired into it.
//
// Regions follow webui/spec.md 3.1: header, unit sidebar, canvas, inspector.
// The configuration protocol is still read-only, so the canvas shows the
// device live and the inspector describes it; nothing here writes config.
//
// Renders are coalesced to one animation frame and only encoders whose props
// actually changed are rebuilt, so a knob turn touches one encoder per frame
// rather than the whole chassis per sysex push.

import { hsvToCss, hsvToRgb } from "./color.js";
import { DeviceModel, NUM_BANKS, NUM_ENCODERS } from "./device-model.js";
import * as midi from "./midi.js";
import * as settings from "./settings.js";
import { DeviceRegistry, DeviceState } from "./devices.js";
import { Param } from "./sysex.js";
import { Protocol } from "./protocol.js";
import { LivePushTracker } from "./live-tracker.js";
import { encoderSignature } from "./encoder-signature.js";
import { BankFade } from "./bank-fade.js";
import { GEOMETRY } from "../design-system/geometry.js";
import { UpdateDialog } from "../design-system/components/update-dialog.js";
import { buildFirmwareUpdateNotice } from "../design-system/components/firmware-update-notice.js";
import {
  fetchManifest,
  checkForUpdate,
  runUpdate,
  watchForBootloader,
  inspectBootloader,
} from "./firmware-update.js";
import * as dfu from "../dfu/xmega-dfu.js";
import {
  buildDeviceChassis,
  buildEncoder,
  buildBankSelector,
  buildDeviceList,
  buildInspector,
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
const registry = new DeviceRegistry();

let device = null;
let protocol = null;
let connected = false;
let viewingBank = 0;
let pendingBank = null;
let chassis = null;
let renderPending = false;
let inspectorTab = "device";
let bootloaderInfo = null;
let connectFraction = null;
let expandedId = null;
const signatures = new Array(NUM_ENCODERS).fill(null);

const el = {
  unsupportedBanner: byId("unsupported-banner"),
  chassis: byId("twin-chassis"),
  bankSelector: byId("bank-selector"),
  deviceList: byId("device-list"),
  inspector: byId("inspector"),
  canvas: byId("canvas"),
  deviceViewport: byId("device-viewport"),
  deviceScaler: byId("device-scaler"),
  canvasEmpty: byId("canvas-empty"),
  canvasBar: byId("canvas-bar"),
  toastContainer: byId("toast-container"),
};

function byId(id) {
  return document.getElementById(id);
}

async function init() {
  if (!midi.isSupported()) {
    el.unsupportedBanner.hidden = false;
  }
  bankFade.onFrame = scheduleRender;
  buildChassisOnce();
  renderShell();
  fitDevice();
  new ResizeObserver(fitDevice).observe(el.deviceViewport);

  watchForBootloader({
    onPresent: onBootloaderPresent,
    onGone: onBootloaderGone,
  });

  // The list redraws itself as devices come and go; nothing here opens a port.
  registry.onChange(renderShell);

  if (!midi.isSupported()) return;

  // Asks for MIDI permission, which prompts the first time. Enumeration after
  // that is free, so hot-plug costs nothing.
  await registry.start();

  if (settings.get("autoConnect")) await autoConnect();
}

/**
 * Open the only attached device, if the user has asked for that.
 *
 * Kept quiet on failure - the row shows why, and someone who turned this on
 * did not ask to be interrupted when it does not work.
 */
async function autoConnect() {
  const dev = await registry.autoConnect();
  if (dev) await loadDevice(dev);
}

function scheduleRender() {
  if (renderPending) return;
  renderPending = true;
  requestAnimationFrame(() => {
    renderPending = false;
    renderChassis();
  });
}

/**
 * Clicking a row connects it, or releases it if it is the one already open.
 *
 * Releasing matters beyond tidiness: a rawmidi node has a single opener, so
 * nothing else - another application, or the browser claiming the device for
 * DFU - can have it until this page lets go.
 */
async function onDeviceRowClick(id) {
  if (id.startsWith(MOCK_ID_PREFIX)) return;

  if (connected && registry.selectedId === id) {
    await onDisconnectClick();
    return;
  }

  // registry.connect() releases a previously-open device on its own, but the
  // trackers and module state below are tied to that Device object and would
  // otherwise go on pointing at it after it is destroyed.
  if (connected) await releaseCurrentDevice();

  connectFraction = 0.05;

  try {
    const dev = await registry.connect(id);
    if (dev) await loadDevice(dev);
  } catch (e) {
    connected = false;
    connectFraction = null;
    toast("error", e.message);
    renderShell();
  }
}

async function onConnectClick() {
  const [first] = registry.list();
  if (first) await onDeviceRowClick(first.id);
}

/** Read the configuration off a freshly opened device and start tracking it. */
async function loadDevice(dev) {
  try {
    device = dev;
    protocol = new Protocol(device);
    device.onDisconnect(onDeviceDisconnected);

    setConnectFraction(0.15);
    const info = await protocol.getDeviceInfo();
    model.deviceInfo = info;

    const LOAD_START = 0.2;
    const LOAD_SPAN = 0.75;
    setConnectFraction(LOAD_START);
    await model.loadFromDevice(protocol, (done, total) => {
      setConnectFraction(LOAD_START + (done / total) * LOAD_SPAN);
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
    expandedId = registry.selectedId;

    await refreshPendingUpdate(info.fwVersion);
    scheduleUpdateNotice();

    if (info.numEncoders !== NUM_ENCODERS || info.numBanks !== NUM_BANKS) {
      toast(
        "warn",
        `Device reports ${info.numEncoders} encoders / ${info.numBanks} banks, ` +
          `but this twin assumes ${NUM_ENCODERS}/${NUM_BANKS}. Some encoders may not line up.`
      );
    }
  } catch (e) {
    // The port opened fine - registry still thinks the device is connected -
    // but reading its configuration failed partway through. Left tracking a
    // half-loaded device, a retry would call registry.connect() again on
    // ports already open, leaving the previous Device's listeners and
    // heartbeat orphaned on the same port rather than replaced.
    await releaseCurrentDevice();
    toast("error", e.message);
  } finally {
    connectFraction = null;
    scheduleRender();
    renderShell();
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
  await releaseCurrentDevice();
  scheduleRender();
  renderShell();
}

function onDeviceDisconnected(reason) {
  connected = false;
  if (expandedId === registry.selectedId) expandedId = null;
  dismissUpdateNotice();
  pendingBank = null;
  livePosition.detach();
  liveVmapActive.detach();
  liveActiveBank.detach();
  bankFade.stop();
  toast("error", `Device disconnected (${reason}). Reconnect to resume.`);
  scheduleRender();
  renderShell();
}

/**
 * Let go of whatever device the editor is tracking, without touching the
 * chrome around it - onDisconnectClick() and the "connect to a different
 * device" path in onDeviceRowClick() want that part done differently.
 */
async function releaseCurrentDevice() {
  if (protocol) {
    // Stop the device streaming to a page that is no longer listening.
    await protocol.setLivePositionStreaming(false).catch(() => {});
  }

  connected = false;
  if (expandedId === registry.selectedId) expandedId = null;
  dismissUpdateNotice();
  pendingBank = null;
  livePosition.detach();
  liveVmapActive.detach();
  liveActiveBank.detach();
  bankFade.stop();

  await registry.disconnect();
  device = null;
  protocol = null;
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

// How long after a successful connect the update notice appears, if there is
// one to show - long enough that it reads as "by the way", not as something
// that ambushes a device the moment it comes online.
const UPDATE_NOTICE_DELAY_MS = 3000;
let updateNoticeTimer = null;
let updateNoticeEl = null;

/**
 * Announce an available update a few seconds after connecting, if
 * refreshPendingUpdate() found one.
 *
 * Not wired to the real flash flow yet - both buttons on the notice just
 * close it. That flow is its own piece of work; this is the UI that leads
 * into it.
 */
function scheduleUpdateNotice() {
  clearTimeout(updateNoticeTimer);
  updateNoticeTimer = null;
  if (!pendingUpdate?.available) return;

  updateNoticeTimer = setTimeout(() => {
    updateNoticeTimer = null;
    showUpdateNotice();
  }, UPDATE_NOTICE_DELAY_MS);
}

function showUpdateNotice() {
  if (updateNoticeEl || !pendingUpdate?.available) return;

  updateNoticeEl = buildFirmwareUpdateNotice({
    version: pendingUpdate.version,
    changelog: pendingUpdate.manifest?.changelog,
    onUpdate: dismissUpdateNotice,
    onDismiss: dismissUpdateNotice,
  });
  el.canvas.appendChild(updateNoticeEl);
}

function dismissUpdateNotice() {
  clearTimeout(updateNoticeTimer);
  updateNoticeTimer = null;
  updateNoticeEl?.remove();
  updateNoticeEl = null;
}

/*
	Offer the update whatever the device is running.

	For testing the flow without building a higher version every time. A URL
	flag rather than a constant, so it cannot be left switched on by accident
	in what gets published.
*/
function isForced() {
	return new URLSearchParams(location.search).has("forceUpdate");
}

const MOCK_ID_PREFIX = "mock-";

function mockDevicesEnabled() {
  return new URLSearchParams(location.search).has("mockDevices");
}

function mockDevices() {
  return [
    { id: `${MOCK_ID_PREFIX}detected`, name: "MOCK OFFLINE", state: "detected" },
    { id: `${MOCK_ID_PREFIX}busy`, name: "MOCK BUSY", state: "busy" },
    { id: `${MOCK_ID_PREFIX}failed`, name: "MOCK FAILED", state: "failed" },
    { id: `${MOCK_ID_PREFIX}identifying`, name: "MOCK CONNECTING", state: "identifying", progress: 0.4 },
    {
      id: `${MOCK_ID_PREFIX}connected`,
      name: "MOCK CONNECTED",
      state: "connected",
      firmwareVersion: "9.9.9",
      updateAvailable: true,
    },
  ];
}

/**
 * A bootloader turned up.
 *
 * Either an update is running - in which case the update itself is handling
 * it - or the device is sitting in DFU from an interrupted attempt or the
 * encoder gesture, and the user is offered a way to finish.
 */
async function onBootloaderPresent(dev) {
  bootloaderPresent = true;
  if (updateRunning) return;

  const manifest = await fetchManifest();
  if (!manifest?.version) return;

  pendingUpdate = { available: true, version: manifest.version, manifest };

  // A device in DFU has no MIDI interface to ask, so the only way to say what
  // is on it is to read the record the firmware leaves in flash.
  const info = dev ? await inspectBootloader(dev) : null;
  if (bootloaderPresent && !updateRunning) describeBootloader(info);
}

function describeBootloader(info) {
  bootloaderInfo = info
    ? `${info.id} ${info.dirty ? `${info.version}, modified build` : info.version}`
    : "not recognised";
  renderSidebar();
}

function onBootloaderGone() {
  bootloaderPresent = false;
  bootloaderInfo = null;
  if (updateRunning) return;
  renderSidebar();
}

/**
 * Work out whether the firmware this site carries is newer than the device's.
 *
 * The manifest is fetched from this origin rather than from GitHub: release
 * asset downloads carry no CORS headers, so a browser cannot read one however
 * the request is framed.
 */
async function refreshPendingUpdate(deviceVersion) {
  const manifest = await fetchManifest();
  const result = checkForUpdate(deviceVersion, manifest, { force: isForced() });
  pendingUpdate = result.available ? result : null;
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
  renderShell();
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
    renderShell();
  }
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
  if (!connected) {
    el.bankSelector.replaceChildren();
    return;
  }

  el.bankSelector.replaceChildren(
    buildBankSelector({
      count: NUM_BANKS,
      active: viewingBank,
      pending: pendingBank,
      onSelect: switchBank,
    })
  );
}

/**
 * The device is only drawn once there is one to draw.
 *
 * An unpowered chassis looks like a connected device with every light off,
 * which is a worse answer than saying there is nothing here.
 */
function renderDevicePresence() {
  el.deviceScaler.hidden = !connected;
  el.canvasEmpty.hidden = connected;

  // The bar holds the bank selector and nothing else, so with no device it
  // would be an empty bordered strip - which reads as a bug, not a blank.
  el.canvasBar.hidden = !connected;

  if (connected) fitDevice();
}

function setConnectFraction(fraction) {
  connectFraction = fraction;

  const id = registry.selectedId;
  const fill = id && el.deviceList.querySelector(`[data-progress="${CSS.escape(id)}"]`);
  if (!fill) {
    renderSidebar();
    return;
  }

  fill.style.width = fraction === null ? "0%" : `${Math.round(fraction * 100)}%`;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ The regions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * The devices to list.
 *
 * Everything detected appears, connected or not. A device in the bootloader
 * has no MIDI interface so it never shows up here - it is added separately,
 * because this is the tool that recovers it and it must not vanish from it.
 */
function currentDevices() {
  const rows = registry.list().map((d) => {
    const isSelected = d.id === registry.selectedId;

    // registry marks a device CONNECTED as soon as its MIDI port opens - long
    // before loadDevice() has actually read the configuration off it. Left
    // alone, the row would jump to "connected" for the whole of that real
    // work. The editor's own `connected` flag only goes true once loading has
    // actually finished, so it - not the registry's state - decides whether
    // the row still reads as busy.
    const stillLoading = isSelected && !connected && d.state === DeviceState.CONNECTED;
    const state = stillLoading ? DeviceState.IDENTIFYING : d.state;

    const isUpdateCandidate = isSelected && state === DeviceState.CONNECTED;

    return {
      id: d.id,
      name: d.name,
      state,
      firmwareVersion: state === DeviceState.CONNECTED ? model.deviceInfo?.fwVersion : undefined,
      updateAvailable: isUpdateCandidate && Boolean(pendingUpdate?.available),
      progress: isSelected && state === DeviceState.IDENTIFYING ? connectFraction : null,
    };
  });

  if (bootloaderPresent) {
    rows.push({
      id: "bootloader",
      name: "Unrecognised device",
      state: "bootloader",
      firmwareVersion: bootloaderInfo ?? undefined,
      updateAvailable: Boolean(pendingUpdate?.available),
    });
  }

  if (mockDevicesEnabled()) rows.push(...mockDevices());

  return rows;
}

function onDeviceRowExpand(id) {
  if (expandedId === id) return;
  expandedId = id;
  renderSidebar();
}

function renderSidebar() {
  el.deviceList.replaceChildren(
    buildDeviceList({
      units: currentDevices(),
      expandedId,
      onSelect: onDeviceRowClick,
      onExpand: onDeviceRowExpand,
      onUpdate: onUpdateClick,
      onRescan: () => registry.rescan(),
    })
  );
}

function renderInspector() {
  el.inspector.replaceChildren(
    buildInspector({
      tab: inspectorTab,
      onTab: (id) => {
        inspectorTab = id;
        renderInspector();
      },
      connected,
      deviceInfo: model.deviceInfo,
      bank: viewingBank,
      settings: { autoConnect: settings.get("autoConnect") },
      onSetting: onSettingChanged,
    })
  );
}

/**
 * Turning auto-connect on acts immediately, rather than waiting for the next
 * page load - otherwise the switch appears to do nothing.
 */
async function onSettingChanged(name, value) {
  settings.set(name, value);
  if (name === "autoConnect" && value) await autoConnect();
}

/**
 * Scale the device to the room the canvas has.
 *
 * It renders at a fixed 888px because that is what the geometry describes, and
 * with 600px of the viewport spent on the two side panels it will not fit on a
 * 1440px screen. Scaling here keeps every component ignorant of it. Never
 * scaled up past 1 - the artwork has no more detail to show.
 */
function fitDevice() {
  if (!chassis) return;

  const box = el.deviceViewport.getBoundingClientRect();
  const padding = 32;

  // The side switches sit at left/right -sideBtnW, so the device draws wider
  // than the chassis square it is measured by.
  const drawnWidth = chassis.chassisSize + 2 * GEOMETRY.sideBtnW;
  const drawnHeight = chassis.chassisSize;

  const scale = Math.max(
    0.35,
    Math.min(
      1,
      (box.width - padding) / drawnWidth,
      (box.height - padding) / drawnHeight
    )
  );

  // A transform does not affect layout, so the wrapper is given the drawn
  // size explicitly - otherwise the centring grid still reserves 888px.
  el.chassis.style.transformOrigin = "top left";

  // Scaling from the top-left would put the left switch at -sideBtnW * scale,
  // outside the wrapper and under its overflow clip, so it is shifted back in.
  el.chassis.style.transform =
    `translateX(${GEOMETRY.sideBtnW * scale}px) scale(${scale})`;
  el.deviceScaler.style.width = `${drawnWidth * scale}px`;
  el.deviceScaler.style.height = `${drawnHeight * scale}px`;
}

function renderShell() {
  renderDevicePresence();
  renderBankSelector();
  renderSidebar();
  renderInspector();
}

init();
