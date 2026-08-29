import { hsvToCss } from "./color.js";

export const WAVE_STEPS = 64;
export const STEP_MS = 40;

const LEVEL_FLOOR = 40;
const LEVEL_PEAK = 255;
const HUE_BASE = 560;
const HUE_STEP = 11;
const SATURATION = 200;
const DELAY_STEPS = 6;

const ENVELOPE = Array.from({ length: WAVE_STEPS }, (_, i) =>
  Math.round(((1 - Math.cos((i / WAVE_STEPS) * Math.PI * 2)) / 2) * 255)
);

export function waveLevel(distanceFromLead, step) {
  const offset = distanceFromLead * DELAY_STEPS;
  const idx = (((step - offset) % WAVE_STEPS) + WAVE_STEPS) % WAVE_STEPS;
  return LEVEL_FLOOR + Math.round((ENVELOPE[idx] / 255) * (LEVEL_PEAK - LEVEL_FLOOR));
}

export function waveHue(distanceFromLead) {
  return HUE_BASE + distanceFromLead * HUE_STEP;
}

export function waveColor(distanceFromLead, step) {
  return hsvToCss(waveHue(distanceFromLead), SATURATION, waveLevel(distanceFromLead, step));
}

export function startIdleWave(tick) {
  const reduced =
    typeof matchMedia === "function" && matchMedia("(prefers-reduced-motion: reduce)").matches;

  tick(0);
  if (reduced) return () => {};

  let step = 0;
  const timer = setInterval(() => {
    step = (step + 1) % WAVE_STEPS;
    tick(step);
  }, STEP_MS);
  return () => clearInterval(timer);
}
