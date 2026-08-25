*** Settings ***
Documentation     Hardware-in-the-loop test for the firmware update path.
...
...               Separate from sysex_protocol.robot because it is destructive
...               in a way the others are not: it erases and rewrites the
...               application section. It is also slow, and it leaves the
...               device unreachable over MIDI for part of the run.
...
...               What this covers is the device-side contract - that the
...               guarded sysex command really reaches the bootloader, that
...               the bootloader accepts an image and validates it, and that
...               the device comes back running what was written. The wire
...               format of the browser port is not tested here; that is
...               tools/dfu/xmega-dfu.test.js, which can assert on individual
...               bytes in a way a hardware test cannot.
...
...               dfu-programmer is used as the reference flasher. Requires
...               scripts/99-neon-samurai.rules installed, or it cannot reach
...               the device even though lsusb shows it.
Library           ../lib/NeonSamuraiLibrary.py
Library           ../lib/DfuLibrary.py
Library           OperatingSystem
Suite Setup       Start From A Running Device
Suite Teardown    Make Sure The Device Is Running

*** Variables ***
${FIRMWARE}       ${CURDIR}/../../../build/Release/neosam.hex

*** Keywords ***
Start From A Running Device
    [Documentation]    Check the image is there, and get the device out of the
    ...    bootloader if something left it there.
    ...
    ...    Worth doing rather than assuming: a previous run that failed part
    ...    way, or any manual poking with the browser flasher, leaves the
    ...    device in DFU with no MIDI interface at all - and every test here
    ...    then fails with "no rawmidi device", which says nothing about what
    ...    is actually wrong.
    File Should Exist    ${FIRMWARE}
    ...    msg=Build the firmware first - this suite flashes build/Release/neosam.hex

    ${in_dfu}=    Dfu Device Is Present
    IF    ${in_dfu}
        Log    Device was in DFU at suite start - recovering it    level=WARN
        Erase Device Flash
        Flash Firmware    ${FIRMWARE}
        Launch Application
        Wait Until Keyword Succeeds    15s    0.3s    Connect To Device
        Disconnect From Device
    END

Make Sure The Device Is Running
    [Documentation]    However the run ended, the device should not be left
    ...    sitting in the bootloader. Flashing again is safe: the bootloader is
    ...    never written, so a repeat is always recoverable.
    ${in_dfu}=    Dfu Device Is Present
    IF    ${in_dfu}
        Log    Device left in DFU - flashing it back    level=WARN
        Erase Device Flash
        Flash Firmware    ${FIRMWARE}
        Launch Application
        Wait Until Keyword Succeeds    15s    0.3s    Connect To Device
    END
    Run Keyword And Ignore Error    Disconnect From Device

Reconnect After Update
    [Documentation]    The device re-enumerates after leaving the bootloader,
    ...    and the ALSA card index can move, so discovery is redone rather than
    ...    the old path being reused.
    Wait Until Keyword Succeeds    15s    0.3s    Connect To Device

*** Test Cases ***
The Device Can Be Updated And Comes Back Running
    [Documentation]    The whole cycle, in the order a real update happens.
    ...
    ...    The firmware written is the one already on the device, so the run
    ...    ends where it started. What is being proved is that each step hands
    ...    over to the next - not that the version changed, which would need a
    ...    second image built solely for the test.
    Connect To Device
    ${before}=    Get Device Info
    Log    Before update: ${before}[fw_version]

    Enter Bootloader
    Wait For Dfu Device

    Erase Device Flash
    Flash Firmware    ${FIRMWARE}
    Launch Application

    Reconnect After Update
    ${after}=    Get Device Info

    Should Be Equal    ${after}[fw_version]    ${before}[fw_version]
    Should Be Equal As Integers    ${after}[num_encoders]    16
    Should Be Equal As Integers    ${after}[num_banks]    4
    Disconnect From Device

The Device Is Configurable After An Update
    [Documentation]    Coming back on the bus is not the same as working. A
    ...    device that enumerates but has lost its settings, or cannot accept
    ...    them, has still had a failed update from the user's point of view.
    Connect To Device
    Set Vmap Range    0    0    0    11    99
    ${lower}    ${upper}=    Get Vmap Range    0    0    0
    Should Be Equal As Integers    ${lower}    11
    Should Be Equal As Integers    ${upper}    99
    Disconnect From Device

The Bootloader Is Reachable Twice In A Row
    [Documentation]    A device that can only be updated once is a device that
    ...    cannot be updated. The boot key and the reset cause both have to be
    ...    left in a state that allows the next entry.
    Connect To Device
    Enter Bootloader
    Wait For Dfu Device
    Launch Application

    Reconnect After Update
    ${info}=    Get Device Info
    Should Be Equal As Integers    ${info}[num_encoders]    16
    Disconnect From Device
