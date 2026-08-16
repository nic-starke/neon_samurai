// Knob + ring + indicator LEDs + RGB LED are one component (not four),
// per the design decision in webui/design-system/README.md - they all
// update together from the same device state. Ported from Encoder.dc.html.

import { elc, svgEl } from "./dom.js";
import { buildCapTopSvg } from "./cap.js";
import { buildLedRing } from "./led-ring.js";
import { buildRgbArc } from "./rgb-arc.js";
import { buildVmapPill } from "./vmap-pill.js";
import { hsvHex } from "./color-utils.js";

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

	// Neutral black/grey - real hardware plastic, not UI chrome.
	const selShadow = p.selected ? "0 0 0 3px var(--ds-accent)" : "0 0 0 0 transparent";
	const body = elc("div", {
		style:
			`position:relative; width:${bodySize}px; height:${bodySize}px; border-radius:50%; ` +
			"background:radial-gradient(circle at 50% 118%, #1e2026 0%, #0a0b0d 55%), linear-gradient(157deg, #2f333c 0%, #14161a 34%, #08090b 68%, #101216 100%); " +
			`box-shadow:${selShadow}, inset 0 1.5px 1px rgba(255,255,255,0.11), inset 0 -2px 3px rgba(255,255,255,0.025), 0 4px 10px rgba(0,0,0,0.65); ` +
			"display:flex; align-items:center; justify-content:center; overflow:hidden;",
	});
	bodyWrap.appendChild(body);

	body.appendChild(
		elc("div", {
			style:
				"position:absolute; top:-12%; left:2%; width:74%; height:46%; border-radius:50%; background:linear-gradient(180deg, rgba(255,255,255,0.15) 0%, rgba(255,255,255,0.03) 55%, transparent 100%); filter:blur(3px); pointer-events:none;",
		}),
	);
	body.appendChild(
		elc("div", {
			style:
				"position:absolute; bottom:-6%; right:6%; width:52%; height:30%; border-radius:50%; background:linear-gradient(0deg, rgba(255,255,255,0.07) 0%, transparent 100%); filter:blur(4px); pointer-events:none;",
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
			powered: p.powered,
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
		brightness: p.ledBrightness,
		colorOverride: p.ledColorOverride,
		powered: p.powered,
	});
	// Children of `body`, not `bodyWrap` - their top:50%/left:50% anchors
	// need the same box the arc/cap use.
	body.appendChild(ledFrag);

	const knobSize = p.knobSize ?? 53;
	// Rotates the whole cap (including its SVG's rib geometry) to reflect
	// the knob's live position. capLightAngle/capLightOffset and the RGB
	// reflection are panel-fixed ambient lighting, not part of the physical
	// cap - buildCapTopSvg() counter-rotates those internally using this
	// same knobRotation so they don't spin along with the ribs.
	const knobRotation = p.knobRotation ?? 0;
	const knob = elc("div", {
		style:
			`width:${knobSize}px; height:${knobSize}px; border-radius:50%; box-shadow:0 3px 7px rgba(0,0,0,0.65); position:relative; display:flex; align-items:center; justify-content:center; ` +
			`transform:rotate(${knobRotation}deg);`,
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
			knobRotation,
			topFaceHex: hsvHex(p.capTopHue ?? 220, p.capTopSat ?? 13, p.capTopVal ?? 17),
			innerFaceHex: hsvHex(p.capInnerHue ?? 219, p.capInnerSat ?? 9, p.capInnerVal ?? 18),
			// Only reflect a real, actually-lit colour - same "off means off,
			// not dim" rule rgb-arc.js follows.
			reflectionColor: p.rgbOff || p.powered === false || !p.rgbColor ? undefined : p.rgbColor,
		}),
	);

	if (p.vmapCount > 1) {
		// Centred directly on the cap's inner disc, not the ring - small
		// enough to fit inside p.capInnerDia without covering the whole
		// knob. Appended to `knob`, not `body`, so it rotates along with
		// the cap (matching how the letters sit fixed to the physical
		// button face on real hardware, not the panel).
		knob.appendChild(
			elc("div", {
				style: `position:absolute; top:50%; left:50%; transform:translate(-50%,-50%) rotate(${-knobRotation}deg);`,
				children: [buildVmapPill({ count: p.vmapCount, active: p.vmapActive, powered: p.powered, compact: true })],
			}),
		);
	}

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
