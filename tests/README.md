# Host unit tests

```sh
scripts/test.sh          # configure, build, run
```

or, from VS Code, the **Test** task.

Manually:

```sh
cmake -S tests -B build/tests -G Ninja
cmake --build build/tests --target test_all
```

`test_all` builds every suite and then runs them, failing the build if any test
fails. (CMake's built-in `test` target only runs `ctest`, and will happily test
stale binaries.)

## Why these are separate from the firmware build

The firmware is configured with `cmake/toolchain.cmake`, which cross-compiles
for `atxmega128a4u`. A single CMake configure cannot produce both cross and host
targets, so `add_subdirectory(tests)` from the top-level `CMakeLists.txt` would
try to build these with `avr-gcc` and run the results on the wrong machine.

Keeping them standalone means the firmware build is unaffected — an AVR-only
checkout never configures this directory — at the cost of a second configure
step, which `scripts/test.sh` and the VS Code task hide.

## What runs

| Suite | Covers |
| --- | --- |
| `test_sysex` | The sysex message parser: framing, manufacturer ID, command and parameter validation, frame length, index bounds, GET/SET semantics, buffer overflow, recovery after a rejected frame. |
| `test_quadrature` | The quadrature decoder: half-step resolution, direction polarity, bounce rejection, and an exhaustive equivalence check against the decoder it replaced. |

## How it is put together

**Sanitizers are always on.** Several tests assert nothing themselves and rely
on ASan or UBSan to fail the run — `test_set_rejects_out_of_range_bank` is a
memory-safety regression test, and the useful signal is "no out-of-bounds write
occurred", which an assertion cannot check as directly as ASan can.

**ABI flags match the firmware.** `-fpack-struct` and `-fshort-enums` are set
here because the firmware uses them and they are ABI-affecting. The sysex wire
format in particular derives its layout from `sizeof()` of enum members, so a
host build without these would silently be testing a different message layout
than the device parses.

**Modules are reached by `#include`-ing the `.c` file.** Both suites under test
expose their logic through a single static function. Including the translation
unit is the conventional embedded approach and avoids weakening the module's
interface purely to make it testable.

**Hardware is faked, not stubbed out.** `support/stubs.c` provides the three
symbols the sysex parser needs — `gENCODERS`, `event_post()` and
`event_channel_subscribe()` — and records what was posted so tests can assert on
the replies the parser generates. `support/avr/` holds shims for `<avr/io.h>`
and `<avr/pgmspace.h>`; the latter is what lets a module keep its real `PROGMEM`
annotations, and so stay correct on the device, while still being testable
natively.

## Adding a suite

1. Write `tests/test_<thing>.c`, including `support/test.h` and the module's
   `.c` file.
2. Add `add_unit_test(test_<thing>)` to `tests/CMakeLists.txt`.
3. If the module needs a firmware symbol that does not exist on the host, add a
   fake to `support/stubs.c` rather than an `#ifdef` in the firmware.

`support/test.h` provides `CHECK`, `CHECK_EQ_INT`, `RUN_TEST` and
`TEST_SUMMARY`. It is deliberately dependency-free — no network fetch at
configure time and nothing vendored. If the suite outgrows it, Unity is the
natural next step and `cmake/utils` already carries a CMake module for it.

## A note on what tests are for here

Both suites were written alongside a fix and verified to **fail against the code
they were fixing** — that is the bar for a regression test in this directory.

The quadrature suite is a worked example of why. The first replacement decoder
passed every behavioural test and was still wrong: the exhaustive comparison
against the previous implementation found 15,474 divergences on reversal
patterns. Without that test the difference would not have surfaced until someone
noticed encoders feeling subtly wrong on real hardware.
