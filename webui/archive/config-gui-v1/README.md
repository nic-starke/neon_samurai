# config-gui-v1 (archived)

Frozen snapshot of the full sysex config-editing GUI - bank tabs, the
per-encoder detail panel (display mode, detent, switch mode, per-vmap
HSV/range/position/MIDI proto config), side-switch mode dropdowns, local
preset save/load, and factory reset - taken when `webui/index.html` was
stripped down to a connect-and-view-only digital twin (no editing UI).

Self-contained - this directory has its own copies of every module it
needs (`js/`, `design-system/`, `js/vendor/`) rather than importing from
the live `webui/` tree, matching `../twin-v1/`'s precedent, so it keeps
working even as the live app's modules change out from under it. Still
runnable as-is (`python3 -m http.server` from `webui/`, then open
`archive/config-gui-v1/index.html`, connect a real device) if you want to
compare against the current view or need the editing functionality back,
but it is not maintained - fixes and new features go into the live
version only.

## Why this was archived

The live app's focus narrowed to being a passive digital twin - connect,
watch the device's real state render live, nothing else. The config-
editing functionality this archive preserves (writing new HSV/range/
mode/proto values back to the device, presets, factory reset) may come
back as a separate tool later, but isn't part of the twin's job.
