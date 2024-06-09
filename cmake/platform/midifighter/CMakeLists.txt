# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Executable
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

add_executable(${PLATFORM_NAME}_executable)

# Define platform-specific sources
file(GLOB_RECURSE PLATFORM_SOURCES "${CMAKE_SOURCE_DIR}/src/platform/${PLATFORM_NAME}/*.c")

# Add platform-specific sources to the executable target
target_sources(${PLATFORM_NAME}_executable PRIVATE ${PLATFORM_SOURCES})
target_include_directories(${PLATFORM_NAME}_executable PRIVATE ${CMAKE_SOURCE_DIR}/src/include/platform/${PLATFORM_NAME})

i need to add lufa as a dependency
maybe use cmake_fetch and then create a library from it?
need a nice way to handle the fetch (making sure that we can track a specified commit hash)
and then... includes?

# Link against common settings library
target_link_libraries(${PLATFORM_NAME}_executable common)
