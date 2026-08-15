// index.js - barrel re-export for the design-system component library.
// Callers (webui/js/twin.js, webui/js/ui.js) import from here rather than
// reaching into individual component files, so the internal module split
// can change without touching every call site. See ../README.md for the
// full component catalog.

export { elc, svgEl } from "./dom.js";
export { shift, dim, hsvHex } from "./color-utils.js";
export { buildChassis } from "./chassis.js";
export { buildEncoder } from "./encoder.js";
export { buildCapTopSvg } from "./cap.js";
export { buildLedRing } from "./led-ring.js";
export { buildRgbArc } from "./rgb-arc.js";
export { buildSideSwitch } from "./side-switch.js";
export { computeLitMask, DisplayMode as LedDisplayMode, ENC_MID, ENC_MAX } from "./led-mask.js";
