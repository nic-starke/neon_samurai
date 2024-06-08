# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Meson build project file for NEON_SAMURAI
# Copyright 2024 - Nicolaus Starke
# SPDX-License-Identifier: MIT
#
# https://github.com/nic-starke/neon_samurai
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

project(
    'neosam', 'c',
    version: '1.1',
    default_options: [
        'buildtype=release',
        'c_std=c11',
        'warning_level=1',
        #'b_staticpic=false',
        # 'b_lto=true',
    ],
    license: 'MIT',
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Configuration Files
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Configuration Files
# conf_data = configuration_data()
# conf_data.set('version', meson.project_version())
# conf_data.set('EEKEY', 0x33)
# configure_file(input: 'src/Utility/Config.h.in', output : 'Config.h', configuration: conf_data)

# configuration_inc = include_directories('.')

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Dependencies
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

project_dep = []
platform_dep = []

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Source Files
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

platform_inc = []
platform_src = []
project_inc = []
project_src = []

subdir('src')

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Compiler and Linker Flags/Arguments
# These are in addition to those defined in applicable cross file.
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

c_compiler_args = [
    '-DVSER_ENABLE',

    '-Wall',
    '-Wextra',
    '-Wpedantic',
    # '-Werror',
    '-Wshadow',
    '-Wincompatible-pointer-types',
    '-Wstrict-overflow',
    '-Werror-implicit-function-declaration',
    '-Wnull-dereference',
    '-Wunused-function',
    '-Wunused',
    '-Wreturn-type',
    '-Wredundant-decls',
    '-Wno-lto-type-mismatch',
]

c_link_args = [
  '-flto',
  '-fuse-ld=mold',
]

if get_option('buildtype') == 'release'
    c_compiler_args += ['-DRELEASE', '-DNDEBUG']
endif

if get_option('buildtype') == 'debug'
    c_compiler_args += ['-DDEBUG']
endif


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Platform
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

platform = get_option('platform')

subdir(join_paths('src/platform/', platform))
project_src += platform_src
project_inc += include_directories(join_paths('src/include/platform/', platform))

subdir(join_paths('meson/platform/', platform))
