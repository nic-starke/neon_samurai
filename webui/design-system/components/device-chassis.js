// Composes chassis.js + encoder.js + side-switch.js into the full
// assembled device: faceplate/bevel, 4x4 grid, 6 side switches, and the
// two-light knurl-shading system. Shared by twin.js (demo) and
// live-twin.js (real device) so the grid/lighting geometry lives in one
// place instead of being duplicated per caller.

import { elc } from "./dom.js";
import { buildChassis } from "./chassis.js";
import { buildEncoder } from "./encoder.js";
import { buildSideSwitch } from "./side-switch.js";

const NUM_ENCODERS = 16;
const NUM_SIDE_SWITCHES_PER_SIDE = 3;

/**
 * `encoderProps(i, knurlLight)` is called once per encoder index 0-15
 * (reading order - see live-twin.js's visualPositionToFirmwareIndex()
 * if the caller needs firmware index order) and must return the full
 * buildEncoder() prop object. `sideSwitchProps(side, i)` is optional;
 * omit to render no side switches.
 */
export function buildDeviceChassis(g, encoderProps, sideSwitchProps) {
	const gridGap = g.pitch - g.bodySize;
	const chassisPad = g.edgeFirst - g.bodySize / 2;
	const chassisSize = 4 * g.bodySize + 3 * gridGap + 2 * chassisPad;

	// Two lights fixed to the panel, not to each cap: every encoder's
	// highlight angle is derived from its own position relative to the
	// two light points, so the knurl shading reads as one lit surface
	// rather than 16 identical stickers.
	const lx1 = (g.lightX1 / 100) * chassisSize;
	const ly1 = (g.lightY1 / 100) * chassisSize;
	const lx2 = (g.lightX2 / 100) * chassisSize;
	const ly2 = (g.lightY2 / 100) * chassisSize;
	const deg = (rad) => (rad * 180) / Math.PI;

	const { outer, face } = buildChassis({
		size: chassisSize,
		cornerRadius: g.cornerRadius,
		bevelWidth: g.bevelWidth,
	});
	face.style.padding = `${Math.max(0, chassisPad - g.bevelWidth)}px`;
	face.style.boxSizing = "border-box";

	if (sideSwitchProps) {
		const mid = chassisSize / 2 + g.sideBtnOffsetY;
		const centres = [mid - g.sideBtnSpacing, mid, mid + g.sideBtnSpacing];
		for (const side of ["L", "R"]) {
			centres.forEach((c, i) => {
				outer.appendChild(
					buildSideSwitch({
						side,
						index: i,
						width: g.sideBtnW,
						height: g.sideBtnH,
						top: c - g.sideBtnH / 2,
						...sideSwitchProps(side, i),
					}),
				);
			});
		}
	}

	const grid = elc("div", {
		style: `display:grid; grid-template-columns:repeat(4, ${g.bodySize}px); grid-template-rows:repeat(4, ${g.bodySize}px); gap:${gridGap}px;`,
	});
	face.appendChild(grid);

	for (let i = 0; i < NUM_ENCODERS; i++) {
		const cx = chassisPad + g.bodySize / 2 + (i % 4) * g.pitch;
		const cy = chassisPad + g.bodySize / 2 + Math.floor(i / 4) * g.pitch;
		const a1 = deg(Math.atan2(cy - ly1, cx - lx1));
		const a2 = deg(Math.atan2(cy - ly2, cx - lx2));
		const knurlLight = {
			angle: Math.round(a1 + 90),
			offset: Math.round((((a2 - a1) % 360) + 360) % 360),
		};

		grid.appendChild(
			buildEncoder({
				bodySize: g.bodySize,
				capLightAngle: knurlLight.angle,
				capLightOffset: knurlLight.offset,
				...encoderProps(i, knurlLight),
			}),
		);
	}

	return { el: outer, chassisSize };
}

export { NUM_ENCODERS, NUM_SIDE_SWITCHES_PER_SIDE };
