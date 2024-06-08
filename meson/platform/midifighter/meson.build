# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Platform file for Midifighter Twister (AVR XMega128a4u)
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Environment validation
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

assert(meson.is_cross_build(), 'Not a valid cross-compilation environment')

if get_option('target') != 'midifighter'
    error('This file should only be included when building for midifighter.')
endif

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Meson imports
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

cmake = import('cmake')

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Dependencies
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

qcbor_options = cmake.subproject_options()
qcbor_subproject = cmake.subproject('qcbor', options: qcbor_options)
qcbor = qcbor_subproject.dependency('qcbor')

lufa_dep = dependency(
    'lufa',
    fallback: ['lufa', 'lufa_dep'],
    default_options: ['default_library=static'],
    required: true,
    native: false,
)

add_global_arguments('-DQCBOR_DISABLE_FLOAT_HW_USE', language: 'c')
add_global_arguments('-DUSE_LUFA_CONFIG_HEADER', language: 'c')

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Source Files
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

platform_inc = include_directories('src/include/platform/midifighter')

platform_src = files(

    'hal/avr/xmega/128a4u/boot.c',
    'hal/avr/xmega/128a4u/dma.c',
    'hal/avr/xmega/128a4u/gpio.c',
    'hal/avr/xmega/128a4u/init.c',
    'hal/avr/xmega/128a4u/timer.c',
    'hal/avr/xmega/128a4u/usart.c',

    'midi/backend/midi_lufa.c',
    'usb/lufa/usb_lufa.c',
    'display.c',
    'hw_encoder.c',
    'hw_led.c',
    'hw_switch.c',
    'input_manager.c',

    'main.c',
    'menu.c',
    'systime.c',
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Compiler and Linker Flags/Arguments
# These are in addition to those defined in the cross file.
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

c_compiler_args += [
    '-std=gnu11',

    '-g3', # Generate debug symbols, can be stripped later

    '-ffreestanding',
    '-fwhole-program',
    '-fpack-struct',
    '-fshort-enums',
    '-mrelax',

    # '-fverbose-asm',
]

if get_option('buildtype') == 'release'
    c_compiler_args += ['-Ofast', '-DRELEASE', '-DNDEBUG']
endif

if get_option('buildtype') == 'debug'
    c_compiler_args += ['-O0', '-g3', '-DDEBUG']
endif

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Build targets
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

neosam = executable(
    meson.project_name() + '.elf',
    [project_src],
    c_args: c_compiler_args,
    link_args: c_link_args,
    include_directories: [project_inc],
    dependencies: [qcbor, lufa_dep],
)

hex = custom_target(
    meson.project_name() + '.hex',
    input: neosam,
    output: meson.project_name() + '.hex',
    command: [
        find_program('objcopy'),
        meson.get_external_property('objcopy_hex_args'),
    ],
)

bin = custom_target(
    meson.project_name() + '.bin',
    input: neosam,
    output: meson.project_name() + '.bin',
    command: [
        find_program('objcopy'),
        meson.get_external_property('objcopy_bin_args'),
    ],
)

eep = custom_target(
    meson.project_name() + '.eep',
    input: neosam,
    output: meson.project_name() + '.eep',
    command: [
        find_program('objcopy'),
        meson.get_external_property('objcopy_eep_args'),
    ],
)

run_target(
    'BinSize',
    command: [find_program('objdump'), '-Pmem-usage', neosam.full_path()],
)

run_target('EepromSize', command: [find_program('size'), '-A', eep.full_path()])
