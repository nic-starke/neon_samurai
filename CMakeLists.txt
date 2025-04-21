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
include(ExternalProject)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Project Definition / Settings
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
project(NEON_SAMURAI
  VERSION 1.1.0
  DESCRIPTION "An alternative firmware for the DJ Tech Tools Midifighter Twister"
  HOMEPAGE_URL "https://github.com/nic-starke/neon_samurai"
  LANGUAGES C CXX ASM
)

set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

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

# Common compiler flags
set(AVR_COMMON_FLAGS "-mmcu=${MCU} -fpack-struct -fshort-enums -ffunction-sections -fdata-sections -funsigned-char -funsigned-bitfields")

# Common warning flags
set(AVR_WARNING_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wformat=2 -Wformat-truncation -Wundef -Wno-attributes")

# Common linker flags (same for all build types)
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections -Wl,-Map=${CMAKE_BINARY_DIR}/output.map -mmcu=${MCU}")

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
include(${CMAKE_SOURCE_DIR}/cmake/utils/linker/AddExecutableWithLinkerScriptDep.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/DefaultCCompilerSettings.cmake)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Dependencies
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

# LUFA USB Library
FetchContent_Declare(
	LUFA
	GIT_REPOSITORY https://github.com/nic-starke/lufa.git
	GIT_TAG master
	GIT_SHALLOW true
)

FetchContent_MakeAvailable(LUFA)
FetchContent_GetProperties(LUFA)
if(NOT lufa_POPULATED)
	FetchContent_Populate(LUFA)
endif()

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
    ${CMAKE_SOURCE_DIR}/src/led/hw_led.c
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
    ${CMAKE_SOURCE_DIR}/src/hal/dma.c
    ${CMAKE_SOURCE_DIR}/src/hal/gpio.c
    ${CMAKE_SOURCE_DIR}/src/hal/init.c
    ${CMAKE_SOURCE_DIR}/src/hal/signature.c
    ${CMAKE_SOURCE_DIR}/src/hal/sys.c
    ${CMAKE_SOURCE_DIR}/src/hal/timer.c
    ${CMAKE_SOURCE_DIR}/src/hal/usart.c
)

target_include_directories(neosam PRIVATE
	${CMAKE_SOURCE_DIR}/src/include/
	${CMAKE_SOURCE_DIR}/src/include/usb/
)

set_target_properties(neosam PROPERTIES
    OUTPUT_NAME "neosam"
    SUFFIX ".elf"
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

# Add VSER_ENABLE to the defines
add_compile_definitions(neosam PRIVATE VSER_ENABLE)

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
