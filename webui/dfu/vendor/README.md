# Flash tool on Web for AVR
You can program AVR in web browser at: https://tmk.github.io/AVRFlashOnWeb/

Use Chrome or Edge. WebUSB is not supported by Safari and Firefox unfortunately.
 
On Linux you need to install [udev rules](https://github.com/tmk/tmk_keyboard/wiki/FAQ-Build#linux-udev-rules) file to get permission.

On Windows you need to install WinUSB driver refering to [this instruction](https://github.com/tmk/tmk_keyboard/wiki/WinUSB-Driver), or you can just use [Zadig](https://zadig.akeo.ie/) instead.

Check this also.
https://github.com/tmk/tmk_keyboard/wiki#flash-on-web


## Atmel USB DFU protocol
https://ww1.microchip.com/downloads/en/DeviceDoc/doc7618.pdf

## WebUSB
https://developer.mozilla.org/en-US/docs/Web/API/WebUSB_API

## Supported controllers
This supports AVR microcontrollers with Atmel USB DFU bootloader.
- at90usb128x
- at90usb64x
- at90usb162
- at90usb82
- atmega32u6
- atmega32u4
- atmega32u2
- atmega16u4
- atmega16u2
- atmega8u2
