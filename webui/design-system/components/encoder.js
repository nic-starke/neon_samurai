// encoder.js - one full "Encoder" assembly: plastic ring body, RGB
// indicator arc, indicator LED ring, and the knurled cap. This is the
// single component that owns all of an encoder's visual state, per the
// "encoder knob, encoder ring, indicator LEDs, and RGB LED should be a
// single Encoder component" design decision - see
// webui/design-system/README.md.
//
// Composes cap.js + led-ring.js + rgb-arc.js. Ported from
// Encoder.dc.html.

import { elc, svgEl } from "./dom.js";
import { buildCapTopSvg } from "./cap.js";
import { buildLedRing } from "./led-ring.js";
import { buildRgbArc } from "./rgb-arc.js";
import { hsvHex } from "./color-utils.js";

/**
 * @param {object} p See inline defaults below for the full prop list.
 *   Notable ones:
 *   - `rgbColor` / `rgbOff` - the RGB backlight; `rgbOff` suppresses glow entirely (see rgb-arc.js)
 *   - `litMask` - explicit per-LED indicator state (preferred for real device data); falls back to `value`/`max` fill if omitted
 *   - `capLightAngle` / `capLightOffset` - per-instance lighting angle so 16 identical caps under one shared panel light don't look like stickers (see twin.js's two-light derivation)
 *   - `selected` / `onSelect` - selection outline + click handler
 */
export function buildEncoder(p) {
	const bodySize = p.bodySize ?? 91;
	const center = bodySize / 2;
	const showLabel = p.showLabel ?? true;

	const outer = elc("div", {
		style: `cursor:pointer; display:flex; flex-direction:column; align-items:center; justify-content:center; gap:${showLabel ? 9 : 0}px; flex-shrink:0; padding:${showLabel ? 6 : 0}px;`,
		title: p.label ?? "",
		onClick: p.onSelect,
	});

	const bodyWrap = elc("div", {
		style: `position:relative; width:${bodySize}px; height:${bodySize}px; display:flex; align-items:center; justify-content:center;`,
	});
	outer.appendChild(bodyWrap);

	const selShadow = p.selected ? "0 0 0 3px var(--ds-accent)" : "0 0 0 0 transparent";
	const body = elc("div", {
		style:
			`position:relative; width:${bodySize}px; height:${bodySize}px; border-radius:50%; ` +
			"background:radial-gradient(circle at 50% 118%, #10201c 0%, #06090a 55%), linear-gradient(157deg, #16241f 0%, #0b1210 34%, #050706 68%, #0b1210 100%); " +
			`box-shadow:${selShadow}, inset 0 1.5px 1px rgba(255,255,255,0.08), inset 0 -2px 3px rgba(255,255,255,0.02), 0 4px 10px rgba(0,0,0,0.65); ` +
			"display:flex; align-items:center; justify-content:center; overflow:hidden;",
	});
	bodyWrap.appendChild(body);

	body.appendChild(
		elc("div", {
			style:
				"position:absolute; top:-12%; left:2%; width:74%; height:46%; border-radius:50%; background:linear-gradient(180deg, rgba(157,255,219,0.12) 0%, rgba(157,255,219,0.02) 55%, transparent 100%); filter:blur(3px); pointer-events:none;",
		}),
	);
	body.appendChild(
		elc("div", {
			style:
				"position:absolute; bottom:-6%; right:6%; width:52%; height:30%; border-radius:50%; background:linear-gradient(0deg, rgba(157,255,219,0.06) 0%, transparent 100%); filter:blur(4px); pointer-events:none;",
		}),
	);

	body.appendChild(
		buildRgbArc({
			bodySize,
			radius: p.arcRadius ?? 37.5,
			width: p.arcWidth ?? 9,
			length: p.arcLength ?? 25,
			color: p.rgbColor,
			off: p.rgbOff,
		}),
	);

	const ledFrag = buildLedRing({
		count: p.ledCount ?? 11,
		radius: p.ledRadius ?? 37,
		size: p.ledSize ?? 9,
		arcSpan: p.ledArcSpan ?? 270,
		value: p.value,
		max: p.max,
		litMask: p.litMask,
	});
	// buildLedRing returns a fragment of absolutely-positioned LEDs; they
	// need to be children of `body` (not bodyWrap) so their top:50%/left:50%
	// anchors resolve against the same box the arc/cap use.
	body.appendChild(ledFrag);

	// Cap (rubber knob + knurled top).
	const knobSize = p.knobSize ?? 53;
	const knob = elc("div", {
		style: `width:${knobSize}px; height:${knobSize}px; border-radius:50%; box-shadow:0 3px 7px rgba(0,0,0,0.65); position:relative; display:flex; align-items:center; justify-content:center;`,
	});
	body.appendChild(knob);
	knob.appendChild(
		buildCapTopSvg({
			size: knobSize,
			color: p.capColor ?? "#26282B",
			innerScallopDia: p.capInnerDia ?? 7.5,
			baseDia: p.capBaseDia ?? 18.5,
			gripDiaBottom: p.capGripDiaBottom ?? 15,
			gripDiaTop: p.capGripDiaTop ?? 13.5,
			ribCount: p.capRibCount ?? 19,
			lightAngle: p.capLightAngle ?? 265,
			lightOffset: p.capLightOffset ?? 180,
			topFaceHex: hsvHex(p.capTopHue ?? 220, p.capTopSat ?? 13, p.capTopVal ?? 17),
			innerFaceHex: hsvHex(p.capInnerHue ?? 219, p.capInnerSat ?? 9, p.capInnerVal ?? 18),
		}),
	);

	if (showLabel) {
		outer.appendChild(
			elc("span", {
				style: "font-family:var(--ds-font-mono); font-size:10.5px; color:var(--ds-text-dim);",
				text: p.label ?? "",
			}),
		);
	}

	return outer;
}
