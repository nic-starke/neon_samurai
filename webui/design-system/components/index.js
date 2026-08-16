// Barrel re-export - see ../README.md for the component catalog.

export { elc, svgEl } from "./dom.js";
export { shift, dim, hsvHex } from "./color-utils.js";
export { buildChassis } from "./chassis.js";
export { buildDeviceChassis, NUM_ENCODERS, NUM_SIDE_SWITCHES_PER_SIDE } from "./device-chassis.js";
export { buildEncoder } from "./encoder.js";
export { buildCapTopSvg } from "./cap.js";
export { buildLedRing } from "./led-ring.js";
export { buildRgbArc } from "./rgb-arc.js";
export { buildVmapPill } from "./vmap-pill.js";
export { buildSideSwitch } from "./side-switch.js";
export {
	computeLitMask,
	computeLedBrightness,
	computeDetentColorOverride,
	DisplayMode as LedDisplayMode,
	ENC_MID,
	ENC_MAX,
	NUM_PWM_FRAMES,
} from "./led-mask.js";
