*** Settings ***
Documentation     Hardware-in-the-loop tests for the neon_samurai sysex
...               configuration protocol (src/midi/sysex.c). Requires a
...               real device connected and enumerated as a MIDI port -
...               these are not mocked, by design; see tests/robot/README.md.
...
...               Suite Setup reads the device's entire configuration and
...               Suite Teardown writes it back, so a run leaves the device as
...               it found it. Two tests here factory-reset the device, which
...               would otherwise destroy whatever the owner had configured.
...
...               Every test starts and ends by putting the bank/encoder/vmap
...               elements it writes to into a fixed baseline, so a value left
...               behind by an earlier test cannot stand in for one this test
...               was supposed to write.
Library           ../lib/NeonSamuraiLibrary.py
Library           Collections
Suite Setup       Begin Test Session
Suite Teardown    End Test Session
Test Setup        Set Elements Under Test To Baseline
Test Teardown     Set Elements Under Test To Baseline

*** Variables ***
${BANK}           0
${ENCODER}        0
${VMAP}           0

*** Keywords ***
Begin Test Session
    [Documentation]    Reads the device's whole configuration before anything
    ...    is written to it. Two tests in this suite factory-reset the device,
    ...    so without this a run would destroy whatever the owner had set up
    ...    and leave the suite's own scratch values behind.
    Connect To Device
    Back Up Device Config

End Test Session
    [Documentation]    Puts the backed-up configuration back and waits for the
    ...    device to flush it to EEPROM. Runs even when a test has failed,
    ...    which is exactly when it matters most.
    Restore Device Config
    Disconnect From Device

*** Test Cases ***
Device Info Reports Expected Hardware Capability
    [Documentation]    Confirms the device-info query (added alongside the
    ...    GET fix) returns real firmware version and hardware counts,
    ...    which the web GUI uses to self-configure instead of hardcoding.
    ${info}=    Get Device Info
    Should Be Equal As Integers    ${info}[num_encoders]    16
    Should Be Equal As Integers    ${info}[num_banks]    4
    Should Be Equal As Integers    ${info}[num_vmaps_per_encoder]    2
    Should Be Equal As Integers    ${info}[num_side_switches]    6

Detent Set And Get Round Trip
    [Documentation]    The original working param (predates this project's
    ...    fixes) - a baseline sanity check that basic GET/SET still works.
    Set Encoder Param    ENCODER_DETENT    ${BANK}    ${ENCODER}    1
    ${value}=    Get Encoder Param    ENCODER_DETENT    ${BANK}    ${ENCODER}
    Should Be Equal As Integers    ${value}[0]    1

    Set Encoder Param    ENCODER_DETENT    ${BANK}    ${ENCODER}    0
    ${value}=    Get Encoder Param    ENCODER_DETENT    ${BANK}    ${ENCODER}
    Should Be Equal As Integers    ${value}[0]    0

Vmap Range Set And Get Round Trip
    [Documentation]    Regression test for the fix that made GET actually
    ...    return values instead of just an ack code.
    Set Vmap Range    ${BANK}    ${ENCODER}    ${VMAP}    10    100
    ${lower}    ${upper}=    Get Vmap Range    ${BANK}    ${ENCODER}    ${VMAP}
    Should Be Equal As Integers    ${lower}    10
    Should Be Equal As Integers    ${upper}    100

Vmap Range Survives Device Reset
    [Documentation]    Regression test for the EEPROM persistence fix -
    ...    range/position were previously settable over sysex but silently
    ...    lost on every reboot. Uses MF_SYSEX_PARAM_SYSTEM_RESET (a plain
    ...    reboot, config untouched) rather than the factory reset this
    ...    suite otherwise uses for setup, since that would defeat the
    ...    point - persistence needs a value to survive a reboot, not be
    ...    wiped by one.
    ...
    ...    The 6s sleep before resetting is not padding - cfg_update()
    ...    (config.c) only autosaves live encoder state to EEPROM every
    ...    5s, so a SET immediately followed by a reset races that window
    ...    and reads back the pre-SET value after reboot, not because
    ...    persistence is broken but because the write was never flushed
    ...    to EEPROM in the first place. Confirmed directly: the first
    ...    version of this test without the sleep failed with exactly
    ...    that symptom (0 != 42).
    Set Vmap Range    ${BANK}    ${ENCODER}    ${VMAP}    42    123
    Sleep    6s    waiting for cfg_update()'s 5s autosave window to flush the SET to EEPROM
    Reset Device
    ${lower}    ${upper}=    Get Vmap Range    ${BANK}    ${ENCODER}    ${VMAP}
    Should Be Equal As Integers    ${lower}    42
    Should Be Equal As Integers    ${upper}    123

Factory Reset Restores Defaults
    [Documentation]    Regression test for MF_SYSEX_PARAM_CONFIG_RESET
    ...    itself (also what Test Setup uses to establish known state for
    ...    every other test in this suite) - sets a non-default value,
    ...    factory-resets, and confirms it's back to the compiled-in
    ...    default (MIDI_CC_MIN/MIDI_CC_MAX, i.e. 0/127 - see
    ...    sw_encoder_init() in input_manager.c) rather than the value
    ...    that was just set.
    Set Vmap Range    ${BANK}    ${ENCODER}    ${VMAP}    88    99
    Factory Reset Device
    ${lower}    ${upper}=    Get Vmap Range    ${BANK}    ${ENCODER}    ${VMAP}
    Should Be Equal As Integers    ${lower}    0
    Should Be Equal As Integers    ${upper}    127

Vmap Hsv Set And Get Round Trip With High Byte Values
    [Documentation]    Regression test for the 7-bit MIDI packing fix -
    ...    saturation/value of 255 both have the high bit set, which
    ...    corrupted the sysex stream before packing was added. This is
    ...    the specific case that motivated that fix.
    Set Vmap Hsv    ${BANK}    ${ENCODER}    ${VMAP}    300    255    255
    ${hue}    ${sat}    ${val}=    Get Vmap Hsv    ${BANK}    ${ENCODER}    ${VMAP}
    Should Be Equal As Integers    ${hue}    300
    Should Be Equal As Integers    ${sat}    255
    Should Be Equal As Integers    ${val}    255

Vmap Hsv Set And Get Round Trip With Low Byte Values
    [Documentation]    Same as above but with values that fit in 7 bits,
    ...    as a control - both cases must work identically.
    Set Vmap Hsv    ${BANK}    ${ENCODER}    ${VMAP}    0    100    50
    ${hue}    ${sat}    ${val}=    Get Vmap Hsv    ${BANK}    ${ENCODER}    ${VMAP}
    Should Be Equal As Integers    ${hue}    0
    Should Be Equal As Integers    ${sat}    100
    Should Be Equal As Integers    ${val}    50

Out Of Range Bank Index Is Rejected Without Corrupting State
    [Documentation]    Regression test for the bounds-check additions -
    ...    an out-of-range bank/encoder/vmap index must be silently
    ...    rejected (no reply), and critically must not corrupt adjacent
    ...    encoder state. Sets a known-good value first, attempts an
    ...    invalid write, then re-reads the known-good value to confirm
    ...    it's untouched.
    Set Vmap Range    ${BANK}    ${ENCODER}    ${VMAP}    5    50
    ${bad_payload}=    Build Vmap Range Payload    99    0    0    10    100
    Expect No Response    SET    VMAP_RANGE    ${bad_payload}
    ${lower}    ${upper}=    Get Vmap Range    ${BANK}    ${ENCODER}    ${VMAP}
    Should Be Equal As Integers    ${lower}    5
    Should Be Equal As Integers    ${upper}    50

Out Of Range Vmap Index Is Rejected Without Corrupting State
    Set Vmap Range    ${BANK}    ${ENCODER}    ${VMAP}    7    77
    ${bad_payload}=    Build Vmap Range Payload    0    0    99    10    100
    Expect No Response    SET    VMAP_RANGE    ${bad_payload}
    ${lower}    ${upper}=    Get Vmap Range    ${BANK}    ${ENCODER}    ${VMAP}
    Should Be Equal As Integers    ${lower}    7
    Should Be Equal As Integers    ${upper}    77

Out Of Range Active Bank Is Rejected
    [Documentation]    An invalid active-bank index must be rejected
    ...    before being applied - it indexes gENCODERS[bank][...]
    ...    everywhere else in the firmware, so an invalid value here
    ...    would corrupt encoder lookups system-wide if it were ever
    ...    allowed through.
    ${bad_payload}=    Build Active Bank Payload    99
    Expect No Response    SET    ACTIVE_BANK    ${bad_payload}

Every Bank Is Independently Addressable
    [Documentation]    The fourth bank was added after this suite was first
    ...    written, and a bank that is counted by the device-info reply but
    ...    not actually reachable would look identical from the outside. Writes
    ...    a distinct value to the same encoder in every bank, then reads them
    ...    all back - which fails both if a bank is unreachable and if two
    ...    banks alias onto the same storage.
    ${info}=    Get Device Info
    FOR    ${bank}    IN RANGE    ${info}[num_banks]
        ${lower}=    Evaluate    10 + ${bank}
        ${upper}=    Evaluate    100 + ${bank}
        Set Vmap Range    ${bank}    ${ENCODER}    ${VMAP}    ${lower}    ${upper}
    END

    FOR    ${bank}    IN RANGE    ${info}[num_banks]
        ${lower}    ${upper}=    Get Vmap Range    ${bank}    ${ENCODER}    ${VMAP}
        ${want_lower}=    Evaluate    10 + ${bank}
        ${want_upper}=    Evaluate    100 + ${bank}
        Should Be Equal As Integers    ${lower}    ${want_lower}
        Should Be Equal As Integers    ${upper}    ${want_upper}
    END

Active Bank Accepts Every Bank The Device Reports
    [Documentation]    Pairs with "Out Of Range Active Bank Is Rejected" -
    ...    that one proves too high a value is refused, this one proves the
    ...    boundary is in the right place and the last valid bank is not
    ...    refused along with it.
    ${info}=    Get Device Info
    FOR    ${bank}    IN RANGE    ${info}[num_banks]
        ${status}=    Set Active Bank    ${bank}
        Should Be Equal As Integers    ${status}    0
    END
    Set Active Bank    0

Fourteen Bit Range Survives The Wire
    [Documentation]    High-resolution CC ranges do not fit in the 7 bits a
    ...    sysex data byte carries, so range values are split across two bytes
    ...    and reassembled. A value above 127 is the case that catches a
    ...    regression in that packing - it round-trips as its low byte alone
    ...    if the high byte is dropped anywhere along the path.
    Set Vmap Range    ${BANK}    ${ENCODER}    ${VMAP}    1000    16383
    ${lower}    ${upper}=    Get Vmap Range    ${BANK}    ${ENCODER}    ${VMAP}
    Should Be Equal As Integers    ${lower}    1000
    Should Be Equal As Integers    ${upper}    16383

Descending Range Survives The Wire
    [Documentation]    A range given high-to-low is how an inverted control is
    ...    stored, so it must come back the same way round rather than being
    ...    silently sorted.
    Set Vmap Range    ${BANK}    ${ENCODER}    ${VMAP}    120    20
    ${lower}    ${upper}=    Get Vmap Range    ${BANK}    ${ENCODER}    ${VMAP}
    Should Be Equal As Integers    ${lower}    120
    Should Be Equal As Integers    ${upper}    20

Zero Width Range Leaves The Device Responsive
    [Documentation]    A range whose ends are equal reaches a division by the
    ...    span when the encoder is turned. That is guarded in
    ...    convert_range_i16() (system/utility.h), but the guard is only
    ...    meaningful if the value can actually be stored and the device keeps
    ...    running afterwards - an unguarded divide would not return an error,
    ...    it would take the firmware out. The GET after it is the real
    ...    assertion: a device that still answers has not fallen over.
    Set Vmap Range    ${BANK}    ${ENCODER}    ${VMAP}    64    64
    ${lower}    ${upper}=    Get Vmap Range    ${BANK}    ${ENCODER}    ${VMAP}
    Should Be Equal As Integers    ${lower}    64
    Should Be Equal As Integers    ${upper}    64

    ${info}=    Get Device Info
    Should Be Equal As Integers    ${info}[num_encoders]    16

Settings In Every Bank Survive A Reset
    [Documentation]    Extends the single-bank persistence test across all
    ...    banks. Storage for the banks is one contiguous EEPROM structure, so
    ...    an off-by-one in the layout shows up as the last bank failing to
    ...    persist while the earlier ones look fine.
    ${info}=    Get Device Info
    FOR    ${bank}    IN RANGE    ${info}[num_banks]
        ${upper}=    Evaluate    50 + ${bank}
        Set Vmap Range    ${bank}    ${ENCODER}    ${VMAP}    5    ${upper}
    END

    Sleep    6s    waiting for cfg_update()'s 5s autosave window to flush the SETs to EEPROM
    Reset Device

    FOR    ${bank}    IN RANGE    ${info}[num_banks]
        ${lower}    ${upper}=    Get Vmap Range    ${bank}    ${ENCODER}    ${VMAP}
        ${want_upper}=    Evaluate    50 + ${bank}
        Should Be Equal As Integers    ${lower}    5
        Should Be Equal As Integers    ${upper}    ${want_upper}
    END
