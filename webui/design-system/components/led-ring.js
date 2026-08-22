// `count` LEDs at fixed angular positions spread evenly across `arcSpan`
// degrees, centred on top - not a continuous arc.
//
// For a firmware-accurate lit pattern use led-mask.js's computeLitMask()
// and pass it as `litMask`; for DIS_MODE_MULTI_PWM's leading-LED fade use
// computeLedBrightness() as `brightness`, which takes priority over
// `litMask` when both are given.
//
// `colorOverride` recolors one LED. The detent red/blue LEDs share the
// centre indicator's physical slot rather than being a separate pair - see
// encoder_led_s in src/led/led.c, where both drive the same bit of the
// 16-bit frame word.

import { elc } from "./dom.js";

const MAX_BRIGHTNESS = 255; // matches led-mask.js

export function buildLedRing(p) {
  const count = p.count ?? 11;
  const radius = p.radius ?? 37;
  const size = p.size ?? 9;
  const span = p.arcSpan ?? 270;
  const value = p.value ?? MAX_BRIGHTNESS;
  const max = p.max ?? 127;
  const powered = p.powered ?? true;
  const lit = Math.round(count * ((value || 0) / (max || 1)));
  const litMask = p.litMask ?? Array.from({ length: count }, (_, i) => i < lit);
  const colorOverride = powered ? p.colorOverride : null;

  const frag = document.createDocumentFragment();
  for (let s = 0; s < count; s++) {
    const angle = -(span / 2) + (span / (count - 1)) * s;
    const offColor = powered
      ? "var(--ds-led-off)"
      : "var(--ds-led-powered-off)";

    let background;
    let boxShadow;
    if (colorOverride && colorOverride.index === s) {
      background = colorOverride.color;
      boxShadow = `0 0 6px ${colorOverride.color}, inset 0 0 2px rgba(0,0,0,0.25)`;
    } else if (powered && p.brightness) {
      const frac = Math.max(0, Math.min(1, p.brightness[s] / MAX_BRIGHTNESS));
      background = `color-mix(in srgb, var(--ds-led-on) ${(frac * 100).toFixed(
        0
      )}%, ${offColor})`;
      boxShadow =
        frac > 0.02
          ? `0 0 ${(frac * 7).toFixed(1)}px rgba(216,255,240,${(
              frac * 0.85
            ).toFixed(2)}), inset 0 0 2px rgba(0,0,0,0.25)`
          : "inset 0 1px 1.5px rgba(0,0,0,0.4)";
    } else {
      const litSeg = powered && Boolean(litMask[s]);
      background = litSeg ? "var(--ds-led-on)" : offColor;
      boxShadow = litSeg
        ? "var(--ds-glow), inset 0 0 2px rgba(0,0,0,0.25)"
        : "inset 0 1px 1.5px rgba(0,0,0,0.4)";
    }

    frag.appendChild(
      elc("div", {
        style:
          `position:absolute; top:50%; left:50%; width:${size}px; height:${size}px; border-radius:50%; ` +
          `background:${background}; ` +
          `box-shadow:${boxShadow}; ` +
          `transform:translate(-50%,-50%) rotate(${angle}deg) translateY(-${radius}px);`,
      })
    );
  }
  return frag;
}
