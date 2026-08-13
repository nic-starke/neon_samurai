# twin-v1 (archived)

Frozen snapshot of the first digital-twin import (commit `7e5ffba`,
"feat: import initial digital twin"), taken before the geometry-tuning
prototype was split into `webui/design-system/` components and wired into
the live config GUI.

Self-contained - `twin.html`/`twin.css`/`js/twin.js`/`js/twin-render.js`
only reference each other, not anything in `webui/js/` or
`webui/design-system/`. Still runnable as-is (`python3 -m http.server` from
`webui/`, then open `archive/twin-v1/twin.html`) if you want to compare
against the current view, but it is not maintained - fixes and new
features go into the live version only.
