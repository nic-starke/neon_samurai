# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Dependencies
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

include(FetchContent)
include(ExternalProject)

# LUFA USB Library

# https://inhzus.io/posts/2023-12-01-cmake-external-project/

ExternalProject_Add(
  LUFA
  PREFIX external
  GIT_REPOSITORY https://github.com/nic-starke/lufa.git
  GIT_TAG master
  GIT_SHALLOW true
  BUILD_ALWAYS true
  LOG_DOWNLOAD true
  LOG_BUILD true
  CONFIGURE_COMMAND ""
  INSTALL_COMMAND ""
  BUILD_COMMAND make all -C <SOURCE_DIR>/LUFA VERBOSE=1
  BUILD_IN_SOURCE TRUE
  COMMAND_EXPAND_LISTS
  # INSTALL_COMMAND make DESTDIR=<INSTALL_DIR> install
)


# FetchContent_Declare(
  #   liblufa
  #   GIT_REPOSITORY https://github.com/nic-starke/lufa.git
#   GIT_TAG master
# )

# FetchContent_MakeAvailable(liblufa)

# add_library(LUFA INTERFACE)

# target_include_directories(LUFA INTERFACE 
#   ${liblufa_SOURCE_DIR}/
#   ${CMAKE_SOURCE_DIR}/src/include/platform/${PLATFORM_NAME}
# )

# target_include_directories(LUFA INTERFACE ${PROJECT_BINARY_DIR}/include)
# target_link_libraries(LUFA INTERFACE LUFA)

# target_compile_definitions(LUFA INTERFACE USE_LUFA_CONFIG_HEADER=1)
# target_compile_options(LUFA INTERFACE -w)

# AVR XMega128a4u HAL

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Executable
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
add_executable(${PLATFORM_NAME}_executable)

# Define platform-specific sources
file(GLOB_RECURSE PLATFORM_SOURCES "${CMAKE_SOURCE_DIR}/src/platform/${PLATFORM_NAME}/*.c")

# Add platform-specific sources to the executable target
target_sources(${PLATFORM_NAME}_executable PRIVATE ${PLATFORM_SOURCES})

target_include_directories(${PLATFORM_NAME}_executable PRIVATE
  ${CMAKE_SOURCE_DIR}/src/include/platform/${PLATFORM_NAME}/
)

target_link_libraries(${PLATFORM_NAME}_executable PRIVATE common hal_xmega128a4u LUFA)
