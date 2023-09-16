# Technical

## Bootloader

A bootloader is an application that enables re-programming of the firmware on a device. The microcontroller within the MFT (an Atmel AVR XMEGA) is pre-programmed with a DFU (device firmware update) bootloader during production at the factory.

To download custom firmware to the MFT the bootloader application must be started - fortunately this can be done easily with the official MFT firmware, or even the desktop configuration tool MF Utility ([see below](#accessing-the-bootloader)).

Once in bootloader mode, the MFT will be reprogrammable with any firmware, the bootloader does not validate the new firmware image - it just writes data to the application section of the memory.
Programming the MCU in this way also ensures that the bootloader itself does not get overwritten, but it does mean that a small portion of memory must always be reserved for the bootloader application (the XMEGA chip has a dedicated memory section just for the bootloader).

If the user does not like the new firmware, they can simply program the MFT with a different firmware (such as the official firmware).

### Accessing the Bootloader

Press and hold in the four corner encoder switches, while holding them down plug in the USB cable. Wait for the MFT to boot - it should quickly reset into the bootloader mode. This is the same on the Muffin firmware.

> **The encoders will be lit up in a checkerboard pattern when the bootloader is active.**

Alternatively, run the MidiFighter Utility, on the top bar click on **Tools > MidiFighter > Enter Bootloader Mode**.

### Bootloader Memory Location

The bootloader application code resides in a specific memory location within the flash chip of the XMEGA. The AVR GCC IO.h header contains defines for the start and end memory locations for all sections, the boot section (BOOT_SECTION_START) is completely separate from the application section, therefore there is no chance of overwriting the boot section if the default application is within the memory limits (128k).
