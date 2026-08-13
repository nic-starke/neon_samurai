---
name: module-architecture
description: Use when adding a new module, a new backend/driver implementation, or wiring code into the event system in this repository (neon_samurai) — e.g. "add a new module for X", "add support for a second USB backend", "hook this into events", "where should this file go". Encodes the module/include pairing, the interface-vs-backend pattern, the event pub/sub system, and where the project's own architecture docs have drifted from what actually exists.
---

# Module architecture — neon_samurai

## Directory pairing

Implementation and interface are split:

```
src/<module>/<file>.c          — implementation
src/include/<module>/<file>.h  — public interface for that file
```

`target_include_directories` in [CMakeLists.txt](CMakeLists.txt) only adds
`src/include/` (and `src/include/usb/`) to the include path — so all
cross-module `#include`s reference the `<module>/<file>.h` form (e.g.
`#include "event/event.h"`, `#include "hal/boot.h"`), never a relative
path into another module's `src/<module>/` directory. New source files
must register themselves explicitly in the `target_sources(neosam
PRIVATE ...)` list in CMakeLists.txt — there's no globbing.

Modules today: `animation`, `config`, `console`, `encoder`, `event`, `hal`,
`io`, `led`, `lfo`, `midi`, `system`, `usb`, plus root `main.c`. Match an
existing module directory when a change is a natural extension of it;
create a new `src/<name>/` + `src/include/<name>/` pair when it's a
genuinely new concern.

**Caveat — some headers exist with no implementation yet.**
[src/include/menu/menu.h](src/include/menu/menu.h),
[src/include/virtmap/virtmap.h](src/include/virtmap/virtmap.h), and
[src/include/protocol/protocol.h](src/include/protocol/protocol.h) are not
`#include`d by any `.c` file at present — they're planned/stubbed
interfaces, not wired-up modules. Don't assume every header under
`src/include/` has a live backing implementation; check for `#include`
references and a `CMakeLists.txt` entry before treating a module as active.

## Interface vs. backend pattern

Where a subsystem could plausibly have more than one implementation, the
codebase splits a generic interface header from a `_<backend>` suffixed
implementation:

- `src/include/usb/usb.h` (interface) → `src/usb/usb_lufa.c` +
  `src/include/usb/usb_lufa.h` (LUFA backend)
- `src/include/midi/midi.h` (interface) → `src/midi/midi_lufa.c` (LUFA
  backend)

LUFA is the only backend implemented for either today — per
[docs/Code Structure.md](docs/Code%20Structure.md), the point of the split
is future swappability (e.g. a hypothetical TinyUSB backend), not current
polymorphism. When adding a second backend, follow the same
`<module>_<backend>.c` naming and keep the interface header backend-agnostic
(no LUFA types/includes leaking into `usb.h`/`midi.h`).

## Event system (pub/sub)

Cross-module communication goes through the channel-based event system in
[src/include/event/event.h](src/include/event/event.h) /
`src/event/event.c`, rather than modules calling into each other directly.
Fixed channels (`enum event_ch`): `EVENT_CHANNEL_SYS`, `_IO`,
`_MIDI_IN`, `_MIDI_OUT`, `_ANIMATION`.

- Each channel has a statically allocated queue (no dynamic allocation —
  this is an 8-bit MCU with a few KB of RAM) and a linked list of
  handlers, registered with the `EVT_HANDLER(priority, name, handler_fn)`
  macro.
- Priority `0` = registration order; `1` = highest priority (called
  first); `255` = lowest (called last). Most handlers use `0`.
- Normal posts go through the queue and are drained later; anything that
  must be handled synchronously/immediately uses `event_post_rt()`, which
  blocks and calls handlers in turn right away — reach for this only when
  ordering/timing actually requires it (e.g. safety-relevant or
  time-critical paths), not as a default.
- Per-channel event payload types live in `event/<channel>.h` (e.g.
  `event/io.h`, `event/midi.h`, `event/sys.h`, `event/animation.h`) —
  add new event structs there, not in `event.h` itself, which only holds
  the channel/handler machinery.

When a new module needs to react to something happening elsewhere (an
encoder turn, a MIDI message, a bank change), wire it up as an event
handler on the relevant channel rather than adding a direct function-call
dependency between the two modules — that's the "minimal coupling"
principle [docs/Code Structure.md](docs/Code%20Structure.md) describes,
and it's actually followed in the current code (see how `animation.c`
reacts to bank-change events rather than being called directly by
whatever triggers a bank change).

## HAL layer

Hardware-touching code (`ADC`, `boot`, `DMA`, `GPIO`, `init`, `signature`,
`sys`, `timer`, `USART`) lives under `src/hal/` /
`src/include/hal/`, one file per peripheral, providing the boundary
between application code and AVR-specific registers/vendor headers.

**Known docs/reality drift:** [docs/Code Structure.md](docs/Code%20Structure.md)
describes the HAL as nested under a board/arch path like
`hal/avr/xmega/128a4u`, and mentions a top-level "board" directory concept
for future portability. Neither exists in the current tree — `src/hal/`
is flat, and there is no `board`/`boards` directory (a `[refactor] Removed
"platform" concept - now only midifighter is supported.` commit
deliberately went the other way, collapsing multi-platform support down to
midifighter-only). Treat that part of the doc as aspirational/historical,
not current structure — don't create the nested path it describes unless
the user explicitly wants to reintroduce multi-board support, and flag the
doc as stale if you're in the area making related changes.

## Boot / linker-section subtlety worth knowing about

`src/hal/boot.c` and `src/hal/boot_init.c` are deliberately excluded from
LTO (`-fno-lto` via `set_source_files_properties` in
[CMakeLists.txt](CMakeLists.txt)) because `bootloader_check()` relies on
an `.init3` section attribute that LTO silently discards, and a linker
`-u,bootloader_check` flag forces the linker to keep it through
`--gc-sections`. If you touch boot-related code, preserve this — don't
"clean up" the LTO exclusion or the linker flag without understanding why
they're there (see the `fix(boot): implement bootloader entry mechanism`
commit for the full story). This is the kind of change that can
soft-brick a physical device if gotten wrong; treat it with the same care
as [[build-flash-debug]]'s bootloader-flashing warning.
