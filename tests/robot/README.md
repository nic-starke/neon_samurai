# Hardware-in-the-loop tests (Robot Framework)

Tests the neon_samurai firmware's sysex configuration protocol
(`src/midi/sysex.c`) against a **real, connected device** over MIDI. There
is no mock/simulated transport by design - a bug in the actual USB-MIDI
path, the firmware's packet framing, or hardware itself is exactly what
these tests exist to catch, and a mock would hide all of that.

## Setup

```sh
python3 -m venv tests/robot/.venv
tests/robot/.venv/bin/pip install -r tests/robot/requirements.txt
```

## Running

Connect a neon_samurai device (flashed with firmware built from this repo,
or newer - `RelWithDebInfo`/`Release`, not `Debug`, see the note in
[build-flash-debug](../../.claude/skills/build-flash-debug/SKILL.md) about
a Debug-build-specific LED init issue unrelated to these tests but worth
knowing about if you see odd behaviour). Confirm it enumerates:

```sh
amidi -l
# Dir Device    Name
# IO  hw:4,0,0  SAMURAI MIDI 1
```

Then run the suite:

```sh
tests/robot/.venv/bin/python3 -m robot --outputdir tests/robot/results \
    tests/robot/suites/sysex_protocol.robot
```

This is fully automated, including device resets - no manual intervention
needed. Suite Setup factory-resets the device (`Factory Reset Device`, over
sysex - see below) before the first test runs, so every run starts from the
same known state regardless of what a previous run or manual poking left
behind. Individual tests that need a reboot mid-test (persistence, factory
reset itself) trigger and reconnect automatically too. Expect the full run
to take a few minutes - each factory reset alone can take up to ~30s (see
`init_eeprom()`'s byte-by-byte EEPROM erase in `config.c`), and this suite
triggers three resets total (one at Suite Setup, one each in `Vmap Range
Survives Device Reset` and `Factory Reset Restores Defaults`).

Results (`log.html`/`report.html`) land in `tests/robot/results/` (gitignored).

### Resetting the device from a test

`Reset Device` (soft reboot, config untouched) and `Factory Reset Device`
(wipes EEPROM back to defaults, then reboots) are sysex-triggered keywords
in `NeonSamuraiLibrary.py`, backed by `MF_SYSEX_PARAM_SYSTEM_RESET` and
`MF_SYSEX_PARAM_CONFIG_RESET` in the firmware. Both send their SET ack
*before* the device actually reboots (see the design note in `sysex.c` -
the ack has to go out through `event_post_rt()`, synchronously, before the
reboot happens, or it would never be transmitted at all), then the device
drops off the bus and re-enumerates - the library reconnects by re-running
device discovery afterward rather than assuming the same
`/dev/snd/midiCxDy` path is still valid, since the ALSA card index has been
observed to shift across a reset when other USB MIDI devices are present.

These were added specifically so a test suite (or the web GUI, e.g. a
"factory reset" button) doesn't need a second transport - the CDC serial
console has `reset`/`reset_cfg` commands that do the same thing, but that's
a separate port from the sysex/MIDI interface these tests otherwise only
ever touch.

## Why direct ALSA rawmidi, not python-rtmidi/mido

`tests/robot/lib/rawmidi.py` talks to `/dev/snd/midiCxDy` directly rather
than through `python-rtmidi` (which `mido` also wraps). This was a
deliberate choice after hitting a real, reproducible bug: `rtmidi`'s Linux
backend goes through the ALSA **sequencer** (`snd-seq`), which silently
dropped sysex messages as short as 13 bytes total when talking to this
firmware - confirmed by sending byte-for-byte identical messages via
`amidi` (which uses ALSA **rawmidi**, no sequencer involved) and getting a
reply every time. `rawmidi.py` reimplements just enough of what `amidi`
does - open the raw device node, write bytes, read the reply - to avoid
the sequencer path entirely. `amidi -l` is still used for device discovery
(mapping a device name to its `/dev/snd/midiCxDy` path), so `alsa-utils`
remains a dependency, same as it already was for the project's own
`scripts/flash.sh` workflow.

## Structure

```text
tests/robot/
  requirements.txt     - pinned Python deps (robotframework only)
  lib/
    sysex.py            - wire-protocol encode/decode, independent
                           reimplementation of src/midi/sysex.c's format
                           (a second implementation to cross-check the
                           firmware against, not a binding to its C code)
    rawmidi.py           - ALSA rawmidi transport (see above)
    NeonSamuraiLibrary.py - Robot Framework keyword library wrapping both
  suites/
    sysex_protocol.robot - the actual test cases
```

## Adding a new test

Most new sysex-protocol tests can be written entirely in
`sysex_protocol.robot` using the existing keywords (`Get Vmap Range`, `Set
Vmap Hsv`, `Expect No Response`, ...). If a new sysex param is added to the
firmware, mirror it in three places, in this order:

1. `sysex.py`'s `Param` enum (must match `enum mf_sysex_param` in
   `src/include/midi/sysex.h` exactly - same order, same values)
2. A `*_payload()` builder function in `sysex.py`, matching the new
   param's wire struct in `sysex.h`
3. A `Get X` / `Set X` keyword pair in `NeonSamuraiLibrary.py`, following
   the existing ones' shape
