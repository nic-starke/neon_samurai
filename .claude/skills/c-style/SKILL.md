---
name: c-style
description: Use whenever writing or editing C source (.c) or header (.h) files in this repository (neon_samurai) — creating a new module file, adding functions, or touching existing embedded/AVR firmware code. Encodes the file banner and section-marker template, formatting rules (tabs, 80 columns), naming conventions, and the project's fixed-width type aliases, so generated code is indistinguishable from the surrounding codebase.
---

# C style — neon_samurai

This is AVR/xmega embedded firmware (C11, avr-gcc, freestanding). Code must
match the existing formatting exactly — clang-format runs in pre-commit and
will rewrite anything that doesn't conform, so get it right the first time
rather than relying on reformatting.

## File template

Every `.c`/`.h` file starts with the same banner and section markers. Use
the VSCode snippets at [.vscode/snippets.code-snippets](.vscode/snippets.code-snippets)
(`ctemplate` / `htemplate`) as the source of truth; reproduced here:

**Header (`.h`):**
```c
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - <year>) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
```

**Source (`.c`):** same banner, plus `Global Variables` / `Local Variables`
/ `Global Functions` / `Local Functions` markers after `Prototypes`:
```c
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
```

Rules for using these markers:
- Keep every marker even if its section is empty — grep any existing file
  (e.g. [src/hal/boot.h](src/hal/boot.h)) to see this: empty sections are
  left with nothing but the marker line, not deleted.
- Header (`.h`) files declare in `Prototypes`; source (`.c`) files
  implement in `Global Functions` (exported, declared in the matching
  header) or `Local Functions` (`static`, file-scope only).
- `#pragma once` is the near-universal include guard in this codebase (41
  of ~45 headers) — use it, not `#ifndef`/`#define` guards.
- Bump the copyright year range's end to the current year when creating a
  new file; don't touch it on unrelated edits to existing files.

## Formatting (enforced by [.clang-format](.clang-format) / [.editorconfig](.editorconfig))

- **Tabs**, not spaces, width 2 (`UseTab: Always`, `TabWidth: 2`,
  `IndentWidth: 2`).
- **80-column** limit (`ColumnLimit: 80`).
- Braces attached (`BreakBeforeBraces: Attach`): `if (x) {` not `if (x)\n{`.
- Pointer alignment **left**: `char* p`, not `char *p`.
- Consecutive assignments/declarations/bit-fields/macros are
  vertically aligned — clang-format handles this automatically; write
  naturally and let it align, don't hand-align.
- Short `if`/function bodies are never collapsed onto one line
  (`AllowShortIfStatementsOnASingleLine: Never`,
  `AllowShortFunctionsOnASingleLine: None`) — always use braces and a
  newline body, even for a single statement.
- Run `pre-commit run clang-format --files <file>` (or just let
  `pre-commit` run on commit) rather than trusting hand-formatting.

## Types

Use the project's fixed-width aliases from
[src/include/system/types.h](src/include/system/types.h) instead of raw
`stdint.h` names or bare `int`/`unsigned`:

```c
u8  u16  u32      // uint8_t / uint16_t / uint32_t
i8  i16  i32      // int8_t  / int16_t  / int32_t
vu8 vu16 vu32     // volatile uint*_t — use for MMIO / ISR-shared state
vi8 vi16 vi32     // volatile int*_t
uint  vuint       // unsigned int / volatile unsigned int
f32 f64           // float / double
uptr              // uintptr_t
```

Reach for the `v`-prefixed variants for anything touched from an ISR or
memory-mapped I/O register — that's their whole purpose on this MCU.

## Naming and other conventions

- Files/functions/variables: `snake_case`. Types: `snake_case` with `_t`
  suffix for typedef'd structs/enums (`console_command_t`,
  `command_handler_t`). Macros/constants: `SCREAMING_SNAKE_CASE`.
- Module pairing: implementation in `src/<module>/<file>.c`, its public
  interface in `src/include/<module>/<file>.h` (see [[module-architecture]]
  for the full module-boundary conventions).
- `static` for every file-local function and variable — nothing is
  implicitly exported. Look at [src/console/console.c](src/console/console.c)
  for a representative file: static handler table, static line buffer,
  forward-declared static handlers in `Prototypes`.
- String literals used only for display (help text, command names, log
  messages) go in `PROGMEM` (`static const char foo[] PROGMEM = "...";`) —
  this is an AVR with very little RAM; anything that doesn't need to live
  in RAM shouldn't.
- `assert()` at the top of functions that take pointer arguments is
  idiomatic here (see `encoder_movement_init`/`_update` in
  [src/encoder/encoder.c](src/encoder/encoder.c)) — keep doing it for new
  functions with pointer/handle parameters.
- Warnings are treated seriously: the build uses `-Wall -Wextra
  -Wformat=2 -Wformat-truncation -Wundef` (see [CMakeLists.txt](CMakeLists.txt)).
  Write code that's clean under those, not code that relies on
  `-Wno-unused-parameter`/`-Wno-attributes` (the two blanket suppressions
  already granted).
- Comments are sparse and purposeful — short trailing `// why`-style notes
  on nonobvious lines (see the `#include` block in console.c: `// Add ADC
  header for temperature reading`) rather than block comments restating
  what the code does. Match that density; don't over-comment.

## Comments

- Do not add comments regarding "changed" implementations, for example
  if a factor changed in an algorithm, or the size of an array, or constant
  which then impacts other parts of the code - these comments do not
  provide anything meaningful. Do not add them.
- In general, do not add comments, only add comments to explain complex
  systems, designs, or modules.