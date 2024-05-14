# Project definition
project(
    'neosam', 'c',
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

add_global_arguments('-DQCBOR_DISABLE_FLOAT_HW_USE', language : 'c')
add_global_arguments('-DUSE_LUFA_CONFIG_HEADER', language : 'c')

# QCBOR serialisation
qcbor_options = cmake.subproject_options()
qcbor_subproject = cmake.subproject('qcbor', options: qcbor_options)
qcbor = qcbor_subproject.dependency('qcbor')

lufa_dep = dependency(
    'lufa',
    fallback : ['lufa', 'lufa_dep'],
    default_options: ['default_library=static'],
    required : true,
    native: false,
)

# Configuration Files
# conf_data = configuration_data()
# conf_data.set('version', meson.project_version())
# conf_data.set('EEKEY', 0x33)
# configure_file(input: 'src/Utility/Config.h.in', output : 'Config.h', configuration: conf_data)

# configuration_inc = include_directories('.')

# Sources and Includes
project_inc = []
project_src = []

subdir('src')
subdir('src/board/djtt/midifighter')

project_src += board_src


c_compiler_args = [
    '-std=gnu11',

    '-Ofast',
    '-DRELEASE',
    '-DNDEBUG',

    '-ffreestanding',
    '-flto=auto',
    '-fpack-struct',
    '-fshort-enums',
    '-mrelax',

    # '-fverbose-asm',

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

# Targets
neosam = executable(
    meson.project_name() + '.elf',
    [project_src],
    c_args : c_compiler_args,
    link_args: c_link_args,
    # include_directories : [project_inc, configuration_inc],
    include_directories : [project_inc],
    dependencies : [qcbor, lufa_dep],
)

hex = custom_target(
    meson.project_name() + '.hex',
    input : neosam,
    output : meson.project_name() + '.hex',
    command : [find_program('objcopy'), meson.get_external_property('objcopy_hex_args')]
)

bin = custom_target(
    meson.project_name() + '.bin',
    input : neosam,
    output : meson.project_name() + '.bin',
    command : [find_program('objcopy'), meson.get_external_property('objcopy_bin_args')]
)

eep = custom_target(
    meson.project_name() + '.eep',
    input : neosam,
    output : meson.project_name() + '.eep',
    command : [find_program('objcopy'), meson.get_external_property('objcopy_eep_args')]
)

run_target(
    'BinSize',
    command : [find_program('objdump'), '-Pmem-usage', neosam.full_path()]
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
