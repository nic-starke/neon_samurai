# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Host unit tests for NEON_SAMURAI
# Copyright 2024 - Nicolaus Starke
# SPDX-License-Identifier: MIT
#
# https://github.com/nic-starke/neon_samurai
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
#
# This is a separate project root from the firmware build on purpose. The
# firmware cross-compiles for the AVR, and these tests compile the same
# sources for the host, so the two must not share a toolchain.
#
#   cmake -S tests/unit -B build/tests
#   cmake --build build/tests
#   ctest --test-dir build/tests --output-on-failure
#
# Only modules that are free of hardware register access can be listed here.
# Anything that touches a peripheral belongs in the hardware-in-the-loop
# suite under tests/robot instead.

cmake_minimum_required(VERSION 3.20)

project(neon_samurai_tests
  DESCRIPTION "Host unit tests for the NEON_SAMURAI firmware"
  LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

set(NS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../src")

# The firmware is built with these, and the layout of packed structs depends
# on them, so the tests must agree or they would be measuring a different
# program.
add_compile_options(
  -fpack-struct
  -fshort-enums
  -funsigned-char
  -funsigned-bitfields
  -Wall
  -Wextra
  -Wno-unused-parameter
  -Wundef
  -g
)

# Tests exist to catch defects, so let the sanitisers help where the host can.
option(NS_TESTS_SANITIZE "Build the unit tests with ASan and UBSan" ON)
if(NS_TESTS_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Test registration
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
#
# ns_add_test(<name> SOURCES <firmware sources under test...>)
function(ns_add_test name)
  cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})

  add_executable(${name} ${name}.c ${ARG_SOURCES})

  target_include_directories(${name} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/framework
    ${NS_SRC_DIR}/include
  )

  add_test(NAME ${name} COMMAND ${name})
  set_tests_properties(${name} PROPERTIES TIMEOUT 60)
endfunction()

ns_add_test(test_quadrature SOURCES ${NS_SRC_DIR}/encoder/quadrature.c)
ns_add_test(test_switch     SOURCES ${NS_SRC_DIR}/io/switch.c)
ns_add_test(test_utility)
ns_add_test(test_diag      SOURCES ${NS_SRC_DIR}/system/diag.c)
ns_add_test(test_hsv2rgb   SOURCES ${NS_SRC_DIR}/led/hsv2rgb.c)
ns_add_test(test_event      SOURCES ${NS_SRC_DIR}/event/event.c
                            ${CMAKE_CURRENT_SOURCE_DIR}/stubs/event_channels.c)
