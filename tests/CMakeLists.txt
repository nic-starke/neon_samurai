# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Host-side unit tests for NEON_SAMURAI
# Copyright 2026 - Nicolaus Starke
# SPDX-License-Identifier: MIT
#
# These build for the HOST, not for AVR, so they get a debugger, sanitizers and
# sub-second feedback. Configure separately from the firmware build:
#
#   cmake -S tests -B build/tests && cmake --build build/tests
#   ctest --test-dir build/tests --output-on-failure
#
# The firmware build does not reference this directory, so an AVR-only checkout
# is unaffected.
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

cmake_minimum_required(VERSION 3.20)
project(neon_samurai_tests LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)

set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/../src)

# -fpack-struct and -fshort-enums are ABI-affecting and the firmware is built
# with both. The sysex wire format in particular derives its layout from
# sizeof() of enum members, so a host build without these flags would silently
# be testing a different message layout than the device parses.
set(ABI_FLAGS -fpack-struct -fshort-enums -funsigned-char -funsigned-bitfields)

# The sanitizers do real work here: several sysex regression tests rely on ASan
# to catch an out-of-bounds access rather than on an assertion.
set(SAN_FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer -g)

add_library(test_support STATIC support/stubs.c)
target_include_directories(test_support PUBLIC
    ${SRC_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}       # for "support/..."
    ${CMAKE_CURRENT_LIST_DIR}/support # shim for <avr/io.h>
)
target_compile_options(test_support PUBLIC ${ABI_FLAGS} ${SAN_FLAGS} -Wall -Wextra)
target_link_options(test_support PUBLIC ${SAN_FLAGS})

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Test executables
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

set(ALL_TESTS "")

function(add_unit_test name)
  add_executable(${name} ${name}.c)
  target_link_libraries(${name} PRIVATE test_support)
  target_include_directories(${name} PRIVATE ${SRC_DIR})
  add_test(NAME ${name} COMMAND ${name})
  set(ALL_TESTS ${ALL_TESTS} ${name} PARENT_SCOPE)
endfunction()

add_unit_test(test_sysex)
add_unit_test(test_quadrature)

# Build everything, then run it. `cmake --build . --target test_all` is a single
# command that fails the build on a failing test, which is what both the VS Code
# task and CI want. CMake's built-in `test` target only runs ctest and will
# happily test stale binaries.
add_custom_target(test_all
  COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
  DEPENDS ${ALL_TESTS}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Running host unit tests"
  USES_TERMINAL
)

message(STATUS "Host unit tests configured (ASan + UBSan enabled)")
