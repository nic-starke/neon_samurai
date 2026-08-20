// The full assembled device: faceplate/bevel, 4x4 encoder grid, 6 side
// switches, and the two-light knurl shading. Shared by twin.js and
// live-twin.js so the grid and lighting geometry live in one place.
//
// Each encoder sits in its own cell wrapper, returned as `encoderCells`, so a
// caller can replace one encoder without rebuilding the other fifteen.
// `encoderProps(i, knurlLight)` is called per index 0-15 in reading order and
// returns a buildEncoder() prop object; `sideSwitchProps(side, i)` is
// optional.

import { elc } from "./dom.js";
import { buildChassis } from "./chassis.js";
import { buildEncoder } from "./encoder.js";
import { buildSideSwitch } from "./side-switch.js";

const NUM_ENCODERS = 16;
const NUM_SIDE_SWITCHES_PER_SIDE = 3;

export function buildDeviceChassis(g, encoderProps, sideSwitchProps) {
	const gridGap = g.pitch - g.bodySize;
	const chassisPad = g.edgeFirst - g.bodySize / 2;
	const chassisSize = 4 * g.bodySize + 3 * gridGap + 2 * chassisPad;

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

	const encoderCells = [];
	const knurlLights = [];

	for (let i = 0; i < NUM_ENCODERS; i++) {
		const cx = chassisPad + g.bodySize / 2 + (i % 4) * g.pitch;
		const cy = chassisPad + g.bodySize / 2 + Math.floor(i / 4) * g.pitch;
		const a1 = deg(Math.atan2(cy - ly1, cx - lx1));
		const a2 = deg(Math.atan2(cy - ly2, cx - lx2));
		const knurlLight = {
			angle: Math.round(a1 + 90),
			offset: Math.round((((a2 - a1) % 360) + 360) % 360),
		};
		knurlLights.push(knurlLight);

		const cell = elc("div", {
			style: "display:flex; align-items:center; justify-content:center;",
		});
		cell.appendChild(
			buildEncoder({
				bodySize: g.bodySize,
				capLightAngle: knurlLight.angle,
				capLightOffset: knurlLight.offset,
				...encoderProps(i, knurlLight),
			}),
		);
		grid.appendChild(cell);
		encoderCells.push(cell);
	}

	return { el: outer, chassisSize, encoderCells, knurlLights, bodySize: g.bodySize };
}

export { NUM_ENCODERS, NUM_SIDE_SWITCHES_PER_SIDE };
