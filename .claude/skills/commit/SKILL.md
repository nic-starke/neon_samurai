---
name: commit
description: Use whenever creating a git commit in this repository (neon_samurai), or drafting a commit message for the user to review. Encodes the conventional-commit style used in this project's history since mid-2024 — short type(scope) subject line, and for anything beyond a trivial change, a body that explains root cause, what was changed, and how it was verified.
---

# Commit style — neon_samurai

This repo's commit history has two eras. Older commits (pre-2024) use a
`[tag]` prefix style (`[feat]`, `[fix]`, `[build]`, `[doc]`, `[meta]`,
`[cleanup]`, `[prj]`) — **do not use this style**, it predates the current
convention and is kept only for history. Every commit since has used
Conventional Commits, and that's what to produce.

## Subject line

```
<type>(<scope>): <short summary, imperative mood, lowercase after colon>
```

- `type` — one of `feat`, `fix`, `refactor`, `docs`, `build`, `tools`,
  `test`, `chore`, `perf`. `feat`/`fix`/`refactor` dominate this history;
  don't invent new types casually.
- `scope` — optional but used often, and encouraged when the change is
  localized: the module or subsystem, e.g. `boot`, `build`, `console`,
  `led`, `leds`, `display`, `color`, `wiki`. Match an existing `src/<x>`
  directory name or doc area when one applies. Omit the scope only when the
  change is genuinely cross-cutting (e.g. `refactor: renaming of encoder
  structures`).
- Keep the subject on one line, no trailing period.
- No hard length rule has been enforced historically, but real examples
  stay well under 72 chars — aim for that.

## Body — when to include one

A bare subject line is fine for small, self-explanatory changes (see
`feat(led): enhance encoder display logic for detent mode and center
indicator` — no body). But for anything nontrivial — a bug fix, a hardware
issue, a behavioral change, anything where "why" isn't obvious from the
diff — write a body. This project's best commits (and the ones worth
imitating) read like a short incident report:

1. **What was wrong / the problem being solved.** Describe the symptom as
   observed, not just the code defect.
2. **Root cause**, if this is a fix — what investigation found, including
   any tools used (e.g. "Live tracing with a JTAGICE3", "-Wl,-Map output").
   Naming the actual mechanism (a stripped `.init3` section, an LTO
   interaction, a missing include) is far more useful to future readers
   than "fixed a bug".
3. **What changed**, often as a short bullet list when multiple files or
   mechanisms were touched. Bullets describing *why* each piece exists are
   more valuable than restating the diff.
4. **How it was verified**, especially for hardware-facing changes —
   real-hardware testing, specific commands run, round-trip tests. This
   project flashes physical devices; "verified end-to-end on real
   hardware" carries real weight here and should be stated plainly, not
   assumed.
5. **`Closes #N`** on its own line at the end when the commit resolves a
   tracked GitHub issue.

Wrap body text at roughly 72–80 columns (matches the repo's own 80-column
source convention).

## Reference example

```
fix(boot): implement bootloader entry mechanism

bootloader_check() was defined with .init3 section attribute in boot.h
but boot.c never included boot.h, so the attribute was silently dropped.
Combined with -ffunction-sections and --gc-sections, the function was
stripped entirely from the binary. This means flashing neon_samurai
firmware permanently removes the ability to re-enter DFU bootloader
mode, effectively soft-bricking the device.

Fixes:
- boot.c now includes hal/boot.h for proper attribute propagation
- New boot_init.c with top-level asm directive places a call to
  bootloader_check() in .init3, surviving LTO (which discards
  section attributes from C functions)
- boot.c and boot_init.c compile with -fno-lto to preserve .init3
- Linker -u flag keeps bootloader_check through gc-sections
- New 'bootloader' console command calls bootloader_start() to
  enter DFU mode on demand

The .init3 trampoline runs before main(), checks RST.STATUS for
watchdog reset flag + boot_key == 0x99C0FFEE, and jumps to the
factory DFU bootloader if both match.
```

Another good example for a smaller fix, showing the same "problem →
mechanism → fix" shape at shorter length:

```
fix(build): repair invalid $gcc problem matcher in build tasks

$gcc is not a built-in VSCode problem matcher and was failing to
resolve. Replace it with an inline GCC-diagnostic-format matcher, and
switch -fdiagnostics-color from always to auto so compiler output
isn't wrapped in ANSI escape codes that broke pattern matching when
piped into the task runner.
```

## Practical notes

- Never sign-off or add a co-authored item, the commit must just be a
  plainc commit message.
- Don't commit or push unless the user asked for it. If working on `main`,
  branch first rather than committing straight to it.
- Squash-merges from PRs in this history use `Squash merge <branch> into
  main` as the subject — that's a GitHub-generated convention for merge
  commits specifically, not something to imitate for regular commits.
