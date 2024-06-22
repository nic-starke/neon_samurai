# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# CMake configuration file for NEON_SAMURAI
# Copyright 2024 - Nicolaus Starke
# SPDX-License-Identifier: MIT
#
# https://github.com/nic-starke/neon_samurai
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

cmake_minimum_required(VERSION 3.20)
set(CMAKE_GENERATOR "Ninja")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Project Definition / Settings
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
project(NEON_SAMURAI
  VERSION 1.1.0
  DESCRIPTION "An alternative firmware for the DJ Tech Tools Midifighter Twister"
  HOMEPAGE_URL "https://github.com/nic-starke/neon_samurai"
  LANGUAGES C CXX ASM
)

set_property(GLOBAL PROPERTY C_STANDARD 11)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# CMake includes for helper functions/utilities
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
include(${CMAKE_SOURCE_DIR}/cmake/utils/linker/AddExecutableWithLinkerScriptDep.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/DefaultCCompilerSettings.cmake)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Common Source Code and Libraries
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
add_subdirectory(${CMAKE_SOURCE_DIR}/src/common)
add_subdirectory(${CMAKE_SOURCE_DIR}/src/hal/avr/xmega/128a4u)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Build Types
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
set(default_build_type "RelWithDebInfo")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)

  # Set the possible values of build type for cmake-gui/ccmake
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo"
)
endif()

# Set compiler flags based on the build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
    set(CMAKE_C_FLAGS_RELEASE "-O3")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g3")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g3")
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
    set(CMAKE_C_FLAGS_MINSIZEREL "-Os")
else()
    message(WARNING "Unknown build type: ${CMAKE_BUILD_TYPE}")
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Other
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdiagnostics-color=always")

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Platform & Build Directory
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
if(NOT DEFINED PLATFORM)
    message(FATAL_ERROR "PLATFORM variable is not defined.")
endif()

string(TOLOWER "${PLATFORM}" PLATFORM_NAME)

# Check if the platform directories exist before adding them
if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/platform/${PLATFORM_NAME}")
    add_subdirectory(${CMAKE_SOURCE_DIR}/cmake/platform/${PLATFORM_NAME})
else()
    message(FATAL_ERROR "The directory ${CMAKE_SOURCE_DIR}/cmake/platform/${PLATFORM_NAME} does not exist.")
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Output Message
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
message("---------------------------------------")
message("Build type:       \t ${CMAKE_BUILD_TYPE}")
message("Build directory:  \t ${CMAKE_BINARY_DIR}")
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
message("${PROJECT_NAME} Version:          \t ${PROJECT_VERSION}")
message("---------------------------------------")
