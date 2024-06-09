# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Common Sources
# This cmakelists generates a library that a platform-specific application
# can link against.
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

set(LIB_NAME common)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/include/${LIB_NAME})

add_library(${LIB_NAME} INTERFACE)

file(GLOB_RECURSE ${LIB_NAME}_sources "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
target_sources(${LIB_NAME} INTERFACE ${${LIB_NAME}_sources})
target_include_directories(${LIB_NAME} INTERFACE ${INCLUDE_DIR})

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Configuration Files (cmake generated source/header)
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
configure_file(
  "${INCLUDE_DIR}/sys/project.h.in"
  "${PROJECT_BINARY_DIR}/include/${LIB_NAME}/sys/project.h")
