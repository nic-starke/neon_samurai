# CMake Toolchain File for AVR XMEGA
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)

# Set find root path modes
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# AVR-GCC Toolchain
set(AVR_GCC_PATH        /usr/bin/)
set(AVR_LIBC_PATH       /usr/lib/avr/include/)
set(LIB_XMEGA7_PATH     /usr/lib/avr/lib/avrxmega7)

set(CMAKE_AR            avr-ar)
set(CMAKE_ADDR2LINE     avr-addr2line)
set(CMAKE_ASM_COMPILER  avr-as)
set(CMAKE_C_COMPILER    avr-gcc)
set(CMAKE_CXX_COMPILER  avr-g++)
set(CMAKE_LINKER        avr-ld)
set(CMAKE_OBJCOPY       avr-objcopy CACHE INTERNAL "")
set(CMAKE_OBJDUMP       avr-objdump CACHE INTERNAL "")
set(CMAKE_RANLIB        avr-ranlib CACHE INTERNAL "")
set(CMAKE_SIZE          avr-size CACHE INTERNAL "")
set(CMAKE_STRIP         avr-strip CACHE INTERNAL "")

# XMega128a4u Chip
set(F_CPU               32000000)
set(F_USB               48000000)
set(MCU                 atxmega128a4u)
set(MCU_ARCH            ARCH_XMEGA)
set(MCU_DEFINE          ATXMEGA128A4U)
set(USR_BOARD           USER_BOARD)

# Compiler defines
add_definitions(
    -D${MCU_DEFINE}
    -DF_USB=${F_USB}
    -DF_CPU=${F_CPU}
    -DARCH=${MCU_ARCH}
    -DBOARD=${USR_BOARD}
)

# Define compiler flags in a list variable
# Using "set" instead of add_compile_options as the latter seems to
# break compilation of the cmake test program
set(CMAKE_C_FLAGS_LIST
    "-I${AVR_LIBC_PATH}"
    "-mmcu=${MCU}"
    "-fdata-sections"
    "-ffunction-sections"
    "-Wl,--gc-sections"
    # "-Wl,--print-gc-sections"
    # "-Wl,--print-memory-usage"
    "-Wl,--relax"

)

# Convert list to a single string with space-separated elements
string(REPLACE ";" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS_LIST}")

# C Flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" CACHE INTERNAL "")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG} -O0 -g3 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE} -Ofast -DNDEBUG")

# C++ Flags
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

# Custom command function
function(add_avr_post_build_commands target)
    set(VER "_${PROJECT_VERSION}")
    # Print the ELF memory usage
    # add_custom_command(
    #     TARGET ${target} POST_BUILD
    #     COMMAND ${CMAKE_SIZE} -C $<TARGET_FILE:${target}>
    #     COMMENT " =========== Memory Usage =========== "
    # )

    # Generate the EEPROM file
    add_custom_command(
        TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -j .eeprom --no-change-warnings --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0 -O ihex $<TARGET_FILE:${target}> ${CMAKE_CURRENT_BINARY_DIR}/${target}${VER}.eep
        COMMENT " =========== Generating EEPROM file  =========== "
    )

    # Print the EEPROM file size
    # add_custom_command(
    #     TARGET ${target} POST_BUILD
    #     COMMAND ${CMAKE_SIZE} ${CMAKE_BINARY_DIR}/${target}${VER}.eep
    #     COMMENT " =========== EEPROM file size  =========== "
    # )

    # Generate the HEX file
    add_custom_command(
        TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} --strip-debug -O ihex -R .eeprom $<TARGET_FILE:${target}> ${CMAKE_CURRENT_BINARY_DIR}/${target}${VER}.hex
        COMMENT " =========== Generating HEX file  ==========="
    )

    # Print the HEX file size
    # add_custom_command(
    #     TARGET ${target} POST_BUILD
    #     COMMAND ${CMAKE_SIZE} -C -x --common ${CMAKE_BINARY_DIR}/${target}${VER}.hex
    #     COMMENT " =========== HEX file size  =========== "
    # )

    # Generate the BIN file
    add_custom_command(
        TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} --strip-debug -O binary -R .eeprom $<TARGET_FILE:${target}> ${CMAKE_CURRENT_BINARY_DIR}/${target}${VER}.bin
        COMMENT " =========== Generating BIN file  =========== "
    )

    # Print the BIN file size
    # add_custom_command(
    #     TARGET ${target} POST_BUILD
    #     COMMAND ${CMAKE_SIZE} -A --target=bin ${CMAKE_BINARY_DIR}/${target}${VER}.bin
    #     COMMENT " =========== BIN file size  =========== "
    # )


endfunction()
