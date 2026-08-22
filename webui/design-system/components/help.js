// Contextual help, sourced from the manual.
//
// The text shown here is generated from docs/manual/ by
// tools/docs/build_manual.py, so there is no second copy of it to drift out
// of date. A control asks for help by topic id - the same id as the heading
// anchor on the manual page - and gets that section's first paragraph, with
// a link through to the rest.
//
//   import { helpIcon, attachHelp, loadManual } from "./help.js";
//
//   await loadManual();                 // once, at start-up
//   parent.appendChild(helpIcon("detent"));
//   attachHelp(someLabelElement, "detent");
//
// If the manual has not been built, every call degrades to doing nothing
// rather than throwing - the editor is still usable without its help text.

const MANUAL_URL = "manual.json";
const SITE_BASE = "../site";

let manual = null;
let popover = null;
let activeAnchor = null;

/** Load the generated manual index. Safe to call more than once. */
export async function loadManual(url = MANUAL_URL) {
	if (manual) return manual;

	try {
		const response = await fetch(url);
		if (!response.ok) throw new Error(`${response.status}`);
		manual = await response.json();
	} catch {
		// Missing manual is not an error worth breaking the page over.
		manual = { topics: {}, pages: [], version: null };
	}

	return manual;
}

/** The text for one topic, or null if it is not in the manual. */
export function topic(id) {
	return (manual && manual.topics[id]) || null;
}

function ensurePopover() {
	if (popover) return popover;

	popover = document.createElement("div");
	popover.className = "help-popover";
	popover.hidden = true;
	popover.setAttribute("role", "dialog");
	document.body.appendChild(popover);

	// Any click outside, or Escape, dismisses it.
	document.addEventListener("click", (event) => {
		if (!popover.hidden && !popover.contains(event.target) && event.target !== activeAnchor) {
			hide();
		}
	});

	document.addEventListener("keydown", (event) => {
		if (event.key === "Escape") hide();
	});

	return popover;
}

function hide() {
	if (!popover) return;
	popover.hidden = true;
	if (activeAnchor) activeAnchor.setAttribute("aria-expanded", "false");
	activeAnchor = null;
}

function show(anchor, id) {
	const entry = topic(id);
	if (!entry) return;

	const box = ensurePopover();

	box.innerHTML = "";

	const heading = document.createElement("h3");
	heading.className = "help-popover__title";
	heading.textContent = entry.title;

	const body = document.createElement("p");
	body.className = "help-popover__body";
	body.textContent = entry.summary;

	const link = document.createElement("a");
	link.className = "help-popover__link";
	link.href = `${SITE_BASE}/${entry.page}.html#${entry.anchor}`;
	link.target = "_blank";
	link.rel = "noopener";
	link.textContent = `Read more in ${entry.pageTitle}`;

	box.append(heading, body, link);

	// Position under the anchor, nudged back inside the viewport if it would
	// otherwise run off the right-hand edge.
	const rect = anchor.getBoundingClientRect();
	box.hidden = false;

	const width = box.offsetWidth;
	const left = Math.min(
		Math.max(8, rect.left + window.scrollX),
		window.scrollX + document.documentElement.clientWidth - width - 8,
	);

	box.style.left = `${left}px`;
	box.style.top = `${rect.bottom + window.scrollY + 8}px`;

	activeAnchor = anchor;
	anchor.setAttribute("aria-expanded", "true");
}

/** A small "?" button that opens the popover for a topic. */
export function helpIcon(id, label) {
	const button = document.createElement("button");
	button.type = "button";
	button.className = "help-icon";
	button.textContent = "?";
	button.setAttribute("aria-expanded", "false");
	button.setAttribute(
		"aria-label",
		label || `What is ${topic(id)?.title || id}?`,
	);

	button.addEventListener("click", (event) => {
		event.stopPropagation();
		if (activeAnchor === button) {
			hide();
		} else {
			show(button, id);
		}
	});

	return button;
}

/**
 * Make an existing element explain itself: a native tooltip on hover, and
 * the full popover on click. Used for labels that have no room for an icon.
 */
export function attachHelp(element, id) {
	const entry = topic(id);
	if (!entry) return element;

	element.classList.add("has-help");
	element.title = entry.summary;
	element.setAttribute("aria-expanded", "false");

	element.addEventListener("click", (event) => {
		event.stopPropagation();
		if (activeAnchor === element) {
			hide();
		} else {
			show(element, id);
		}
	});

	return element;
}

/** Every page of the manual, for building an index. */
export function pages() {
	return (manual && manual.pages) || [];
}
