# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #
# Executable
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

add_executable(${PLATFORM_NAME}_executable)

# Define platform-specific sources
file(GLOB_RECURSE PLATFORM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

# Add platform-specific sources to the executable target
target_sources(${PLATFORM_NAME}_executable PRIVATE ${PLATFORM_SOURCES})

# Link against common settings library
target_link_libraries(${PLATFORM_NAME}_executable common)
