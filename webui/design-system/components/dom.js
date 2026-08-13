// dom.js - tiny DOM/SVG construction helpers shared by every component in
// this design system. Deliberately not a framework - see
// webui/design-system/README.md for why (no build step, no runtime other
// than the vendored @preact/signals-core for reactive state).

const SVG_NS = "http://www.w3.org/2000/svg";

/** Create an HTML element. `opts.style` is a CSS text string (matching
 * this codebase's existing inline-style convention in ui.js). */
export function elc(tag, opts = {}) {
	const e = document.createElement(tag);
	if (opts.style) e.style.cssText = opts.style;
	if (opts.class) e.className = opts.class;
	if (opts.text !== undefined) e.textContent = opts.text;
	if (opts.title !== undefined) e.title = opts.title;
	if (opts.onClick) e.addEventListener("click", opts.onClick);
	if (opts.attrs) {
		for (const [k, v] of Object.entries(opts.attrs)) e.setAttribute(k, v);
	}
	for (const child of opts.children || []) e.appendChild(child);
	return e;
}

/** Create an SVG element. */
export function svgEl(tag, attrs = {}) {
	const e = document.createElementNS(SVG_NS, tag);
	for (const [k, v] of Object.entries(attrs)) e.setAttribute(k, String(v));
	return e;
}
