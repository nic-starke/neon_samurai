#
# File: meson.build ( 1st November 2021 )
# Project: Muffin
# Copyright 2021 - 2021 Nic Starke (mail@bxzn.one)
# -----
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program. If not, see http://www.gnu.org/licenses/.
#

project(
  'muffin', 'c',
  version : '0.1',
  default_options: [
    'buildtype=release',
    'c_std=c99',
    'warning_level=3',
    'b_staticpic=false',
    'b_lto=true',
  ],
  license : 'GNU v3',
)

assert(meson.is_cross_build(), 'Not a valid cross-compilation environment')

AVRDUDE_PATH			= '/usr/bin/'
PROGRAMMER        = 'avrispmkII'
PROGRAMMER_PORT   = 'usb'

conf_data = configuration_data()
conf_data.set('version', meson.project_version())
configure_file(input: 'src/Config.h.in', output : 'Config.h', configuration: conf_data)
configuration_inc = include_directories('.')
# vcs_tag(input : 'src/Version.h.in', output : 'Version.h')

project_src = []
project_inc = []

folders = [
  'src',
  'src/Display',
  'src/System',
]

foreach f : folders
subdir(f)
project_inc += include_directories(f)
endforeach

muffin = executable(
  meson.project_name() + '.elf',
  [project_src],
  include_directories : [project_inc, configuration_inc]
)

hex = custom_target(
  meson.project_name() + '.hex',
  input : muffin,
  output : meson.project_name() + '.hex',
  command : [find_program('objcopy'), meson.get_external_property('objcopy_hex_args')]
)

bin = custom_target(
  meson.project_name() + '.bin',
  input : muffin,
  output : meson.project_name() + '.bin',
  command : [find_program('objcopy'), meson.get_external_property('objcopy_bin_args')]
)

eep = custom_target(
  meson.project_name() + '.eep',
  input : muffin,
  output : meson.project_name() + '.eep',
  command : [find_program('objcopy'), meson.get_external_property('objcopy_eep_args')]
)

run_target(
  'BinSize',
  command : [find_program('size'), hex.full_path()]
)

run_target(
  'EepromSize',
  command : [find_program('size'), eep.full_path()]
)

if get_option('flash') == true
# AVRDUDE_COM_OPTS = -V -p meson.get_external_property('MCU')
# AVRDUDE_ISP_OPTS = -c $(PROGRAMMER) -P $(PROGRAMMER_PORT)

# upload:    $(TARGET_HEX)
# 		$(AVRDUDE) $(AVRDUDE_COM_OPTS) $(AVRDUDE_ISP_OPTS) \
# 			-U flash:w:$(TARGET_HEX):i

# upload_boot:    $(TARGET_BIN)
# 		$(AVRDUDE) $(AVRDUDE_COM_OPTS) $(AVRDUDE_ISP_OPTS) \
# 			-U boot:w:$(TARGET_BIN)
  # command : [avrdude,
  # '-C/usr/share/arduino/hardware/tools/avrdude.conf',
  # '-v', '-v', '-v', '-v',
  # '-patmega328p', '-carduino', '-P/dev/ttyACM0', '-b115200',
  # '-D', '-Uflash:w:@0@:i'.format(hex.full_path())],
  # depends : hex,
endif

summary({'bindir': get_option('bindir'),
        'libdir': get_option('libdir'),
        'datadir': get_option('datadir'),
        }, section: 'Directories')