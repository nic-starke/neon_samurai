#!/bin/bash
avrdude -u -c avrisp2 -p x128a4u -P usb -U flash:w:"$BUILD_DIR/muffin.hex":a