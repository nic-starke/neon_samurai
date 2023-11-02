#!/bin/bash

# If the neosam firmware has been built with serial then run this command to listen to the virtual serial.
# May need sudo...
minicom -D /dev/ttyACM0