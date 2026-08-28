// The "a new firmware version exists" announcement - not the multi-step
// flash dialog (update-dialog.js), which is a different moment: this one
// only tells the user an update exists and lets them start it or wave it
// away, appearing unprompted a few seconds after connecting.
//
// Deliberately not wired to the real update flow yet - see js/editor.js.
// Choosing UPDATE closes this the same way DISMISS does, until the rest of
// that flow is built.

const el = (tag, props = {}, children = []) => {
	const node = document.createElement(tag);
	Object.assign(node, props);
	for (const c of children) node.append(c);
	return node;
};

// The manifest carries no changelog field yet - releasing that is its own
// piece of work. Placeholder text says so plainly rather than inventing
// release notes or leaving the list conspicuously empty.
const PLACEHOLDER_CHANGELOG = [
	"Changelog summary goes here.",
	"and here.",
	"and a bit more here.",
];

/**
 * @param p.version    the available version, e.g. "0.2.0"
 * @param p.changelog  optional list of bullet strings
 * @param p.onUpdate   called when the user chooses to update
 * @param p.onDismiss  called when the user waves it away
 * @returns the backdrop element - absolutely positioned, so append it into a
 *          `position: relative` container (the canvas) rather than <body>.
 */
export function buildFirmwareUpdateNotice(p) {
	const changelog = p.changelog?.length ? p.changelog : PLACEHOLDER_CHANGELOG;

	const list = el(
		"ul",
		{},
		changelog.map((line) => el("li", { textContent: line })),
	);

	const body = el("div", { className: "fw-dialog__body" }, [
		el("p", { textContent: `A new firmware update is available -> v${p.version}` }),
		list,
	]);

	const dismiss = el("button", { type: "button", className: "fw-btn", textContent: "Dismiss" });
	const update = el("button", {
		type: "button",
		className: "fw-btn fw-btn--primary",
		textContent: "Update",
	});

	dismiss.addEventListener("click", () => p.onDismiss?.());
	update.addEventListener("click", () => p.onUpdate?.());

	const actions = el("div", { className: "fw-dialog__actions" }, [dismiss, update]);

	const panel = el(
		"div",
		{ className: "fw-dialog", role: "alertdialog" },
		[el("h2", { className: "fw-dialog__title", textContent: "Firmware update" }), body, actions],
	);
	panel.setAttribute("aria-modal", "true");
	panel.setAttribute("aria-label", "Firmware update available");

	return el("div", { className: "fw-update-notice-backdrop" }, [panel]);
}
