*** Settings ***
Documentation     Hardware-in-the-loop tests for the neon_samurai sysex
...               configuration protocol (src/midi/sysex.c). Requires a
...               real device connected and enumerated as a MIDI port -
...               these are not mocked, by design; see tests/robot/README.md.
Library           ../lib/NeonSamuraiLibrary.py
Library           Collections
Suite Setup       Connect To Device
Suite Teardown    Disconnect From Device

*** Variables ***
${BANK}           0
${ENCODER}        0
${VMAP}           0

*** Test Cases ***
Device Info Reports Expected Hardware Capability
    [Documentation]    Confirms the device-info query (added alongside the
    ...    GET fix) returns real firmware version and hardware counts,
    ...    which the web GUI uses to self-configure instead of hardcoding.
    ${info}=    Get Device Info
    Should Be Equal As Integers    ${info}[num_encoders]    16
    Should Be Equal As Integers    ${info}[num_banks]    3
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
    ...    lost on every reboot. Requires a manual reset between the SET
    ...    and the GET; see tests/robot/README.md for how this suite runs
    ...    it (console command, not available over this MIDI-only library).
    [Tags]    manual-reset
    Set Vmap Range    ${BANK}    ${ENCODER}    ${VMAP}    42    123
    Log    Trigger a device reset now (console 'reset' command or power cycle), then run 'Vmap Range Persisted After Reset' separately.    level=WARN

Vmap Range Persisted After Reset
    [Documentation]    Companion to the test above - run this AFTER
    ...    manually resetting the device, to confirm the value set there
    ...    survived. Kept as a separate test case (not chained
    ...    automatically) since triggering the reset needs a human or a
    ...    separate serial keyword this MIDI-only suite doesn't have.
    [Tags]    manual-reset
    ${lower}    ${upper}=    Get Vmap Range    ${BANK}    ${ENCODER}    ${VMAP}
    Should Be Equal As Integers    ${lower}    42
    Should Be Equal As Integers    ${upper}    123

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
