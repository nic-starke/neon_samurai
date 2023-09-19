# File: meson.build (1st November 2021)
# Project: Muffin
# Copyright 2021 - 2021 Nicolaus Starke
# -----
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see http://www.gnu.org/licenses/.
#

# Project definition
project(
    'muffin', 'c',
    version: '1',
    default_options: [
        'buildtype=release',
        'c_std=c99',
        'warning_level=1',
        'b_staticpic=false',
        'b_lto=true',
    ],
    license: 'GNU v3',
)

# Cross Compilation
assert(meson.is_cross_build(), 'Not a valid cross-compilation environment')

# Dependencies
cmake = import('cmake')
qcbor_options = cmake.subproject_options()
qcbor_subproject = cmake.subproject('QCBOR', options: qcbor_options)
qcbor = qcbor_subproject.dependency('qcbor')

atomthreads_proj = subproject('atomthreads')
atomthreads = atomthreads_proj.dependency('atomthreads_dep')

# Configuration Files
conf_data = configuration_data()
conf_data.set('version', meson.project_version())
conf_data.set('EEKEY', 0x33)
configure_file(input: 'src/Utility/Config.h.in', output : 'Config.h', configuration: conf_data)

configuration_inc = include_directories('.')

# Sources and Includes
project_inc = []
project_src = []

subdir('src')
subdir('src/LUFA/')

extra_args = [
    '-DMIDI_ENABLE',
    # '-DHID_ENABLE',
    '-DVSER_ENABLE',
]

# Targets
muffin = executable(
    meson.project_name() + '.elf',
    [project_src],
    c_args : extra_args,
    include_directories : [project_inc, configuration_inc],
    dependencies : [lufa, qcbor, atomthreads],
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