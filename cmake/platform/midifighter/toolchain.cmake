# CMake Toolchain File for AVR XMEGA

# Define constants from the Meson cross-file
set(F_CPU "32000000")
set(F_USB "48000000")
set(MCU "atxmega128a4u")
set(MCU_ARCH "ARCH_XMEGA")
set(MCU_DEFINE "ATXMEGA128A4U")
set(USR_BOARD "USER_BOARD")
set(AVR_GCC_PATH "/usr/bin/")  # Update this with your actual AVR GCC toolchain path
set(AVR_LIBC_PATH "/usr/avr/include/")
set(LIB_XMEGA7_PATH "/usr/avr/lib/avrxmega7")

# Define compiler flags in a list variable
set(CMAKE_C_FLAGS_LIST
    "-I${AVR_LIBC_PATH}"
    "-mmcu=${MCU}"
    "-DARCH=${MCU_ARCH}"
    "-D${MCU_DEFINE}"
    "-DF_USB=${F_USB}"
    "-DF_CPU=${F_CPU}"
    "-DBOARD=${USR_BOARD}"
    "-fdata-sections"
    "-ffunction-sections"
    "-Wl,--gc-sections"
    # "-Wno-psabi"
    # "--specs=nosys.specs"
)

# Convert list to a single string with space-separated elements
string(REPLACE ";" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS_LIST}")

# Define other CMake variables as before
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions")
set(CMAKE_C_FLAGS_DEBUG "-Os -g3")
set(CMAKE_C_FLAGS_RELEASE "-Ofast -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

# Set compiler and linker paths
set(CMAKE_AR            "${AVR_GCC_PATH}avr-ar${CMAKE_EXECUTABLE_SUFFIX}")
set(CMAKE_ASM_COMPILER  "${AVR_GCC_PATH}avr-as${CMAKE_EXECUTABLE_SUFFIX}")
set(CMAKE_C_COMPILER    "${AVR_GCC_PATH}avr-gcc${CMAKE_EXECUTABLE_SUFFIX}")
set(CMAKE_CXX_COMPILER  "${AVR_GCC_PATH}avr-g++${CMAKE_EXECUTABLE_SUFFIX}")
set(CMAKE_LINKER        "${AVR_GCC_PATH}avr-ld${CMAKE_EXECUTABLE_SUFFIX}")
set(CMAKE_OBJCOPY       "${AVR_GCC_PATH}avr-objcopy${CMAKE_EXECUTABLE_SUFFIX}" CACHE INTERNAL "")
set(CMAKE_RANLIB        "${AVR_GCC_PATH}avr-ranlib${CMAKE_EXECUTABLE_SUFFIX}" CACHE INTERNAL "")
set(CMAKE_SIZE          "${AVR_GCC_PATH}avr-size${CMAKE_EXECUTABLE_SUFFIX}" CACHE INTERNAL "")
set(CMAKE_STRIP         "${AVR_GCC_PATH}avr-strip${CMAKE_EXECUTABLE_SUFFIX}" CACHE INTERNAL "")

# Set compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE INTERNAL "")

# Set find root path modes
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
