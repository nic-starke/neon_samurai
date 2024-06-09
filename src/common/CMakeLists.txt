# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Common Sources
# This cmakelists generates a library that a platform-specific application
# can link against/
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

add_library(common INTERFACE)

file(GLOB_RECURSE COMMON_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
target_sources(common INTERFACE ${COMMON_SOURCES})
target_include_directories(common INTERFACE ${CMAKE_SOURCE_DIR}/include)

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Configuration Files (cmake generated source/header)
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/include/sys/version.h.in"
  "${PROJECT_BINARY_DIR}/src/include/sys/version.h")
