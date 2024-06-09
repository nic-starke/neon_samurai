# Toolchain file for compiling with GCC on Linux

# Set the system name
set(CMAKE_SYSTEM_NAME Linux)

# Set compiler and linker commands
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_ASM_COMPILER gcc)
set(CMAKE_LINKER ld)

# # Set compiler and linker flags
# set(CMAKE_C_FLAGS "-Wall -Wextra -O2")
# set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=c++11")
# set(CMAKE_ASM_FLAGS "")
# set(CMAKE_EXE_LINKER_FLAGS "")

# # Set debug and release flags
# set(CMAKE_C_FLAGS_DEBUG "-g")
# set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

# Set find root path mode
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
