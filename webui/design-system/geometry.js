// Physical geometry of the rendered chassis, shared by the live view
// (js/editor.js) and the tuning tool (js/twin.js). Previously duplicated
// in both as GEOMETRY/SPEC and kept in step by a comment.
//
// All size fields - not counts, angles, or percentages - are 1.5x the
// original design; see git history for the pre-scale values. The tuning
// tool copies these into mutable signals and adds its own cap-colour
// fields on top; the live view uses them as-is.

export const GEOMETRY = Object.freeze({
	pitch: 204,
	edgeFirst: 138,
	cornerRadius: 54,
	bevelWidth: 21,
	bodySize: 168,
	knobSize: 106.5,
	capBaseDia: 27.75,
	capGripDiaBottom: 22.5,
	capGripDiaTop: 20.25,
	capRibCount: 19,
	capInnerDia: 16.95,
	lightX1: -35,
	lightY1: -45,
	lightX2: 130,
	lightY2: 145,
	ledCount: 11,
	ledRadius: 69,
	ledSize: 15,
	ledArcSpan: 270,
	arcRadius: 69,
	arcWidth: 15,
	arcLength: 48,
	sideBtnW: 9,
	sideBtnH: 58.5,
	sideBtnSpacing: 114,
	sideBtnOffsetY: 0,
});
