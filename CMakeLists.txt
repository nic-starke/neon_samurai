# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# CMake configuration file for NEON_SAMURAI
# Copyright 2024 - Nicolaus Starke
# SPDX-License-Identifier: MIT
#
# https://github.com/nic-starke/neon_samurai
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

cmake_minimum_required(VERSION 3.20)
set(CMAKE_GENERATOR "Ninja")

include(FetchContent)

# Share the FetchContent source checkout (e.g. LUFA) across all build-type
# directories (build/Debug, build/Release, ...) instead of re-cloning it once
# per build type. Each build type still compiles its own object files from
# this shared source.
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/build/_deps" CACHE PATH "" FORCE)

# LTO breaks reliable single-stepping/breakpoints in Debug builds, so only
# enable it for optimized build types.
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG FALSE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# clangd only looks for compile_commands.json at a fixed conventional
# location (workspace root, or "build/"), and has no notion of our separate
# build/<Type> directories. Re-point a top-level symlink at whichever build
# type was configured most recently, so clangd always sees current flags
# regardless of which build type you're actively working with.
file(CREATE_LINK
    "${CMAKE_BINARY_DIR}/compile_commands.json"
    "${CMAKE_SOURCE_DIR}/compile_commands.json"
    SYMBOLIC
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Project Definition / Settings
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Default to RelWithDebInfo when no build type is given. Every compiler flag
# below lives in a per-build-type variable, so an empty CMAKE_BUILD_TYPE would
# otherwise produce a build with no -mmcu, no optimisation and no warnings.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo)
endif()

# C only. There are no C++ or standalone assembly sources - enabling those
# languages just forces an avr-g++ probe on every configure. The one piece of
# assembly in the project is a top-level asm() block inside src/hal/boot_init.c,
# which the C compiler handles.
project(NEON_SAMURAI
  VERSION 1.1.0
  DESCRIPTION "An alternative firmware for the DJ Tech Tools Midifighter Twister"
  HOMEPAGE_URL "https://github.com/nic-starke/neon_samurai"
  LANGUAGES C
)

set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON) # -std=gnu11; avr-libc headers rely on GNU extensions

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# AVR MCU Settings
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# XMega128a4u Chip - define MCU settings here since they're used in compiler flags
set(MCU                 atxmega128a4u)
set(F_CPU               32000000)
set(F_USB               48000000)
set(MCU_ARCH            ARCH_XMEGA)
set(MCU_DEFINE          ATXMEGA128A4U)
set(USR_BOARD           USER_BOARD)

# Compiler defines for the MCU
add_definitions(
    -D${MCU_DEFINE}
    -DF_USB=${F_USB}
    -DF_CPU=${F_CPU}
    -DARCH=${MCU_ARCH}
    -DBOARD=${USR_BOARD}
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Compiler Flags - Common to all build types
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Compiler and warning flags are set explicitly here rather than being probed
# with check_c_compiler_flag(). The probe links a test binary without -mmcu, so
# it fails for this target and the flag is silently dropped - but only on some
# machines, which previously meant two developers building identical source got
# different warning sets AND different language modes (-std=c11 vs -std=gnu11).
# Everything the build uses is listed below, unconditionally, so the result is
# identical everywhere.
#
# -fpack-struct     : AVR8 has no alignment requirements; keeps EEPROM and USB
#                     descriptor layouts byte-exact. ABI-affecting - do not
#                     remove without re-checking struct sizes in config.c.
# -fshort-enums     : load-bearing. src/midi/sysex.c derives wire-format sizes
#                     from sizeof() of enum members; without this the sysex
#                     protocol silently changes layout.
set(AVR_COMMON_FLAGS "-mmcu=${MCU} -fpack-struct -fshort-enums -ffunction-sections -fdata-sections -funsigned-char -funsigned-bitfields")

# Warning set. Kept deliberately achievable: every flag here is clean or nearly
# clean on the current tree, so new warnings mean new problems.
# Aspirational additions, each of which currently produces a large number of
# findings and should be enabled one at a time as the code is cleaned up:
#   -Wconversion -Wsign-conversion -Wshadow -Wcast-align=strict -Wpedantic
#   -Wredundant-decls -Wmissing-declarations -Wdouble-promotion
set(AVR_WARNING_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wformat=2 -Wformat-truncation -Wundef -Wno-attributes -Wnull-dereference -Wlogical-op -Wduplicated-cond -Wshift-overflow=2 -Wpointer-arith -Wwrite-strings -Werror-implicit-function-declaration")

# Common linker flags (same for all build types).
# Deliberately NOT used here, and why:
#   -fuse-ld=mold  mold has no AVR backend; passed the compiler-only probe and
#                  was a silent no-op that would hard-fail on a stricter driver.
#   -Wl,--build-id emitted a 36-byte ALLOC note into the program region that
#                  avr-readelf reports as malformed. Pure flash cost.
#   -Wl,-z,defs    meaningless for a statically linked bare-metal image.
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections -Wl,-Map=${CMAKE_BINARY_DIR}/output.map -mmcu=${MCU} -Wl,--relax")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Compiler Flags - Debug build type
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
set(CMAKE_C_FLAGS_DEBUG "${AVR_COMMON_FLAGS} ${AVR_WARNING_FLAGS} -Og -g3 -ggdb -gdwarf-4")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_ASM_FLAGS_DEBUG "-mmcu=${MCU} -g")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Compiler Flags - Release build type
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
set(CMAKE_C_FLAGS_RELEASE "${AVR_COMMON_FLAGS} ${AVR_WARNING_FLAGS} -Os -flto -fuse-linker-plugin -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_ASM_FLAGS_RELEASE "-mmcu=${MCU}")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Compiler Flags - RelWithDebInfo build type
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${AVR_COMMON_FLAGS} ${AVR_WARNING_FLAGS} -Os -g -ggdb -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
set(CMAKE_ASM_FLAGS_RELWITHDEBINFO "-mmcu=${MCU} -g")

# Debug message to show which compiler flags are being used
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Using Debug compiler flags: ${CMAKE_C_FLAGS_DEBUG}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "Using Release compiler flags: ${CMAKE_C_FLAGS_RELEASE}")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  message(STATUS "Using RelWithDebInfo compiler flags: ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
else()
  message(WARNING "Unknown build type: ${CMAKE_BUILD_TYPE}")
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# CMake includes for helper functions/utilities
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Note - two includes were removed here:
#
#   cmake/utils/linker/AddExecutableWithLinkerScriptDep.cmake
#     Globally overrode the builtin add_executable() to attach LINK_DEPENDS from
#     a LINKER_SCRIPT_DEPENDENCIES variable that this project never sets, so it
#     shadowed a core command to do nothing.
#
#   cmake/DefaultCCompilerSettings.cmake
#     Applied ~46 flags through check_c_compiler_flag(). The probe links without
#     -mmcu and so fails for this target, making the effective flag set depend on
#     the host toolchain rather than on this file. Flags are now explicit above.
#
# Neither is used, so cmake/utils is no longer required to configure. The
# submodule is still checked in - it carries clang-tidy/cppcheck/unity modules
# that are worth adopting later.

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Dependencies
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# LUFA USB Library
# Pinned to an explicit commit. A branch name here made the firmware depend on
# whatever that branch happened to point at when you configured, which is not a
# reproducible build. Bump this SHA deliberately.
# GIT_SHALLOW is incompatible with fetching an arbitrary SHA, so it is omitted.
FetchContent_Declare(
	LUFA
	GIT_REPOSITORY https://github.com/nic-starke/lufa.git
	GIT_TAG 0200c233f30774935822ddb2b963a7ae0675edd9
)

# MakeAvailable already populates; the FetchContent_Populate() block that used
# to follow could never execute and the single-argument form is deprecated as
# of CMake 3.30.
FetchContent_MakeAvailable(LUFA)

# message(STATUS "LUFA source directory: ${lufa_SOURCE_DIR}")

# Add LUFA sources to a variable
set(LUFA_SOURCES
	${lufa_SOURCE_DIR}/LUFA/Platform/UC3/InterruptManagement.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/Template/Template_Endpoint_RW.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/Template/Template_Endpoint_Control_W.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/Template/Template_Endpoint_Control_R.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/USBInterrupt_XMEGA.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/USBController_XMEGA.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/PipeStream_XMEGA.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/Pipe_XMEGA.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/Host_XMEGA.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/EndpointStream_XMEGA.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/Endpoint_XMEGA.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/XMEGA/Device_XMEGA.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/Template/Template_Pipe_RW.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/Template/Template_Endpoint_RW.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/Template/Template_Endpoint_Control_W.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/Template/Template_Endpoint_Control_R.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/USBInterrupt_UC3.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/USBController_UC3.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/PipeStream_UC3.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/Pipe_UC3.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/Host_UC3.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/EndpointStream_UC3.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/Endpoint_UC3.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/UC3/Device_UC3.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/Template/Template_Pipe_RW.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/Template/Template_Endpoint_RW.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/Template/Template_Endpoint_Control_W.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/Template/Template_Endpoint_Control_R.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/USBInterrupt_AVR8.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/USBController_AVR8.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/PipeStream_AVR8.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/Pipe_AVR8.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/Host_AVR8.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/EndpointStream_AVR8.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/Endpoint_AVR8.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/AVR8/Device_AVR8.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/USBTask.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/HostStandardReq.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/Events.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/DeviceStandardReq.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Core/ConfigDescriptors.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Host/StillImageClassHost.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Host/RNDISClassHost.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Host/PrinterClassHost.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Host/MIDIClassHost.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Host/MassStorageClassHost.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Host/HIDClassHost.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Host/CDCClassHost.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Host/AudioClassHost.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Host/AndroidAccessoryClassHost.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Device/RNDISClassDevice.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Device/PrinterClassDevice.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Device/MIDIClassDevice.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Device/MassStorageClassDevice.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Device/HIDClassDevice.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Device/CDCClassDevice.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Device/CCIDClassDevice.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Device/AudioClassDevice.c
	${lufa_SOURCE_DIR}/LUFA/Drivers/USB/Class/Common/HIDParser.c
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Executable
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
add_executable(neosam)

set_target_properties(neosam PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

target_sources(neosam PRIVATE
    ${CMAKE_SOURCE_DIR}/src/main.c
    ${CMAKE_SOURCE_DIR}/src/animation/animation.c
    ${CMAKE_SOURCE_DIR}/src/config/config.c
    ${CMAKE_SOURCE_DIR}/src/console/console.c
    ${CMAKE_SOURCE_DIR}/src/encoder/encoder.c
    ${CMAKE_SOURCE_DIR}/src/encoder/hardware.c
    ${CMAKE_SOURCE_DIR}/src/encoder/quadrature.c
    ${CMAKE_SOURCE_DIR}/src/event/event.c
    ${CMAKE_SOURCE_DIR}/src/io/hw_switch.c
    ${CMAKE_SOURCE_DIR}/src/io/input_manager.c
    ${CMAKE_SOURCE_DIR}/src/io/io.c
    ${CMAKE_SOURCE_DIR}/src/io/switch.c
    ${CMAKE_SOURCE_DIR}/src/led/color.c
    ${CMAKE_SOURCE_DIR}/src/led/hw_led.c
    ${CMAKE_SOURCE_DIR}/src/led/hsv2rgb.c
    ${CMAKE_SOURCE_DIR}/src/led/led.c
    ${CMAKE_SOURCE_DIR}/src/lfo/lfo.c
    ${CMAKE_SOURCE_DIR}/src/midi/midi_lufa.c
    ${CMAKE_SOURCE_DIR}/src/midi/sysex.c
    ${CMAKE_SOURCE_DIR}/src/system/rng.c
    ${CMAKE_SOURCE_DIR}/src/system/sys.c
    ${CMAKE_SOURCE_DIR}/src/system/systime.c
    ${CMAKE_SOURCE_DIR}/src/usb/usb_lufa.c
    ${CMAKE_SOURCE_DIR}/src/hal/adc.c
    ${CMAKE_SOURCE_DIR}/src/hal/boot.c
    ${CMAKE_SOURCE_DIR}/src/hal/boot_init.c
    ${CMAKE_SOURCE_DIR}/src/hal/dma.c
    ${CMAKE_SOURCE_DIR}/src/hal/gpio.c
    ${CMAKE_SOURCE_DIR}/src/hal/init.c
    ${CMAKE_SOURCE_DIR}/src/hal/signature.c
    ${CMAKE_SOURCE_DIR}/src/hal/sys.c
    ${CMAKE_SOURCE_DIR}/src/hal/timer.c
    ${CMAKE_SOURCE_DIR}/src/hal/usart.c
)

# boot.c and boot_init.c must NOT use LTO - the .init3 section attribute on
# bootloader_check() is discarded by LTO, preventing it from running before
# main(). The boot_init.c file contains a top-level asm directive that places
# a call to bootloader_check() in .init3.
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/hal/boot.c
    ${CMAKE_SOURCE_DIR}/src/hal/boot_init.c
    PROPERTIES COMPILE_OPTIONS "-fno-lto"
)

target_include_directories(neosam PRIVATE
	${CMAKE_SOURCE_DIR}/src/include/
	${CMAKE_SOURCE_DIR}/src/include/usb/
)

set_target_properties(neosam PROPERTIES
    OUTPUT_NAME "neosam"
    SUFFIX ".elf"
)

# Force linker to keep bootloader_check (in .init3) - LTO would otherwise discard it
target_link_options(neosam PRIVATE
    "LINKER:-u,bootloader_check"
)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# LUFA
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
target_compile_definitions(neosam PRIVATE USE_LUFA_CONFIG_HEADER=1)

# Note - the compiler suppresses warnings for SYSTEM includes, which
# is useful due to the large number of warnings generated by the LUFA headers.
target_include_directories(neosam SYSTEM PRIVATE ${lufa_SOURCE_DIR})

# Note - to suppress warnings in LUFA sources just apply a compile option
set_property(SOURCE ${LUFA_SOURCES} PROPERTY COMPILE_OPTIONS "-w")

target_sources(neosam PRIVATE ${LUFA_SOURCES})

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# NEON SAMURAI Source Files
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# Add ENABLE_CONSOLE to the defines.
# NOTE: this was previously add_compile_definitions(neosam PRIVATE ENABLE_CONSOLE).
# That command is directory-scoped and takes no target or scope argument, so
# CMake treated "neosam" and "PRIVATE" as macro names and the build defined
# -Dneosam -DPRIVATE for every translation unit, LUFA included.
target_compile_definitions(neosam PRIVATE ENABLE_CONSOLE)

# Add post build commands for AVR-based platforms
# See toolchain.cmake
add_avr_post_build_commands(neosam)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Output Message
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
message("---------------------------------------")
message("Build type:       \t ${CMAKE_BUILD_TYPE}")
message("Build directory:  \t ${CMAKE_BINARY_DIR}")
message("Binary directory: \t ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
# message("Install prefix:   \t ${CMAKE_INSTALL_PREFIX}")
# message("Testing enabled:  \t ${ENABLE_TESTING}")
# message("Clang-tidy:       \t ${ENABLE_CLANG_TIDY}")
# message("Cppcheck:         \t ${ENABLE_CPPCHECK}")
# message("Compiler:         \t ${CMAKE_CXX_COMPILER_ID}")
# message("Sanizizers:       \t ${ENABLE_ASAN}")
# message("Shared libs:      \t ${BUILD_SHARED_LIBS}")
# message("Build libcpp:     \t ${LIBCPP}")
# message("CCache executable:\t ${CCACHE}")
# message("Building doc:     \t ${BUILD_DOC}")
message("${PROJECT_NAME} Version:  \t ${PROJECT_VERSION}")
message("---------------------------------------")
