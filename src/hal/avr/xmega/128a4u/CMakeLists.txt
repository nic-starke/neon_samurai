# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Common Sources
# This cmakelists generates a library that a platform-specific application
# can link against.
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

set(LIB_NAME hal_xmega128a4u)
# set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/include/hal/avr/xmega/128a4u)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/include/)

add_library(${LIB_NAME} INTERFACE)

file(GLOB_RECURSE ${LIB_NAME}_sources "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
target_sources(${LIB_NAME} INTERFACE ${${LIB_NAME}_sources})
target_include_directories(${LIB_NAME} INTERFACE ${INCLUDE_DIR})