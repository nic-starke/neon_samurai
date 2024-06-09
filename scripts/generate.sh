#!/usr/bin/env bash

meson setup build/midifighter --cross-file meson/crossfiles/avr/xmega/128a4u.cross -Dplatform=midifighter --wipe
meson setup build/linux -Dplatform=linux --wipe
