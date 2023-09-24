avrdude -v -u -c jtag3pdi -p x128a4u -P usb -U flash:w:"build/muffin.hex":a

# avrdude -v -u -c jtag3pdi -p x128a4u -P usb -U eeprom:w:"build/muffin.eep"

# if using dfu-programmer
#sudo dfu-programmer atxmega128a4u:1,8 flash ./builddir/muffin.hex
#where :1,8 is the usb BUS and DEVICE numbers from lsusb
# but make sure to put the mft into bootloader mode and then exec lsusb, then dfu-programmer