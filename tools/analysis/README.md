# Static analysis

`run-analyzer.sh` runs the clang static analyzer over the firmware sources.
CI runs the same script, so a clean run locally means a clean run there.

```sh
tools/analysis/run-analyzer.sh          # analyse every source file
tools/analysis/run-analyzer.sh src/led  # or just a subtree
```

The firmware is built with avr-gcc; clang is used here only as an analyser and
never produces an artefact.

## Why the shim headers exist

clang's AVR target rejects the numeric register clobbers (`"30"`, `"31"`) that
avr-libc's `string.h` and `avr/pgmspace.h` use in their inline assembly, so the
analyser cannot get as far as reading the project's own code. `shim/` holds
minimal stand-ins for exactly those two headers: plain prototypes, and program
space reads written as ordinary dereferences.

That last part is the point rather than a workaround. Modelling
`pgm_read_byte()` as a dereference is what lets the analyser follow a value out
of a lookup table and reason about what the code does with it. It also means
the shims describe the *interface* only - they are never compiled into
firmware, and the real headers are the ones the device runs.

Keep them minimal. If the analyser starts reporting an undeclared function from
one of these two headers, add the prototype; do not add behaviour.
