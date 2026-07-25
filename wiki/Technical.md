# Technical

## Bootloader

A bootloader is an application that enables re-programming of the firmware on a device. The microcontroller within the MFT (an Atmel AVR XMEGA) is pre-programmed with a DFU (device firmware update) bootloader during production at the factory.

To download custom firmware to the MFT the bootloader application must be started - fortunately this can be done easily with the official MFT firmware, or even the desktop configuration tool MF Utility ([see below](#accessing-the-bootloader)).

Once in bootloader mode, the MFT will be reprogrammable with any firmware, the bootloader does not validate the new firmware image - it just writes data to the application section of the memory.
Programming the MCU in this way also ensures that the bootloader itself does not get overwritten, but it does mean that a small portion of memory must always be reserved for the bootloader application (the XMEGA chip has a dedicated memory section just for the bootloader).

If the user does not like the new firmware, they can simply program the MFT with a different firmware (such as the official firmware).

### Accessing the Bootloader

> **Warning:** Do not flash NEON_SAMURAI firmware unless you have a PDI programmer/debugger (e.g. a JTAGICE3, or similar) and are willing to open the device and reflash it directly, or have verified one of the software recovery paths below works on your unit first. If neither software path works, the only way back into the bootloader is hardware reprogramming via the PDI header inside the device.

On the official MFT firmware, holding the four corner encoder switches while plugging in the USB cable resets the device into bootloader mode. NEON_SAMURAI implements the same gesture, but the mapping from physical corner to encoder index has not yet been verified against real hardware - see [issue #29](https://github.com/nic-starke/neon_samurai/issues/29). If it does not work on your unit, use the console command below instead.

NEON_SAMURAI also provides a `bootloader` console command that calls into the bootloader on demand - this does not depend on the corner gesture and is the more reliable option if your console connection is working.

> **The encoders will be lit up in a checkerboard pattern when the bootloader is active.**

Alternatively, on the official MFT firmware, run the MidiFighter Utility, on the top bar click on **Tools > MidiFighter > Enter Bootloader Mode**. This tool is not compatible with NEON_SAMURAI.

### Bootloader Memory Location

The bootloader application code resides in a specific memory location within the flash chip of the XMEGA. The AVR GCC IO.h header contains defines for the start and end memory locations for all sections, the boot section (BOOT_SECTION_START) is completely separate from the application section, therefore there is no chance of overwriting the boot section if the default application is within the memory limits (128k).
