# File: meson.build ( 1st November 2021 )
# Project: Muffin
# Copyright 2021 - 2021 bxzn (mail@bxzn.one)
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
	version : '1',
	default_options: [
		'buildtype=release',
		'c_std=c99',
		'warning_level=1',
		'b_staticpic=false',
		'b_lto=true',
	],
	license : 'GNU v3',
)

assert(meson.is_cross_build(), 'Not a valid cross-compilation environment')

AVRDUDE_PATH        ='/usr/bin/'
PROGRAMMER          = 'avrispmkII'
PROGRAMMER_PORT     = 'usb'

conf_data = configuration_data()
conf_data.set('version', meson.project_version())
conf_data.set('EEKEY', 0x33)
configure_file(input: 'src/Utility/Config.h.in', output : 'Config.h', configuration: conf_data)

configuration_inc = include_directories('.')
# vcs_tag(input : 'src/Utility/Version.h.in', output : 'Version.h')

project_inc = []
project_src = []

subdir('src')
subdir('src/LUFA/')

muffin = executable(
	meson.project_name() + '.elf',
	[project_src],
	include_directories : [project_inc, configuration_inc],
    dependencies : [lufa],
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
	# command : [find_program('size'), '--target=' + meson.get_external_property('_MCU_'), hex.full_path()]
    command : [find_program('objdump'), '-Pmem-usage', muffin.full_path()]
)

run_target(
	'EepromSize',
	command : [find_program('size'), '-A', eep.full_path()]
)

if get_option('flash') == true
# AVRDUDE_COM_OPTS = -V -p meson.get_external_property('MCU')
# AVRDUDE_ISP_OPTS = -c $(PROGRAMMER) -P $(PROGRAMMER_PORT)
endif

summary({'bindir': get_option('bindir'),
				'libdir': get_option('libdir'),
				'datadir': get_option('datadir'),
				}, section: 'Directories')