# CMake Toolchain File for AVR XMEGA (Nix-compatible)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)

# Set find root path modes
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# AVR-GCC Toolchain - using environment variables for Nix compatibility
# Falls back to system paths if environment variables are not set
if(DEFINED ENV{AVR_GCC_PATH})
    set(AVR_GCC_PATH $ENV{AVR_GCC_PATH})
else()
    set(AVR_GCC_PATH /usr/bin/)
endif()

if(DEFINED ENV{AVR_LIBC_PATH})
    set(AVR_LIBC_PATH $ENV{AVR_LIBC_PATH})
else()
    set(AVR_LIBC_PATH /usr/lib/gcc/avr/14.1.0/include/)
endif()

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

# Note: MCU definitions have been moved to the root CMakeLists.txt
# to ensure they're available when setting compiler flags

# Custom command function
function(add_avr_post_build_commands target)
    # Use CMAKE_BINARY_DIR directly as it's more reliable
    set(OUTPUT_DIR ${CMAKE_BINARY_DIR})

    # Generate the EEPROM file
    add_custom_command(
        TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -j .eeprom --no-change-warnings --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0 -O ihex $<TARGET_FILE:${target}> ${OUTPUT_DIR}/${target}.eep
        COMMENT " =========== Generating EEPROM file  =========== "
    )

    # Generate the HEX file
    add_custom_command(
        TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} --strip-debug -O ihex -R .eeprom $<TARGET_FILE:${target}> ${OUTPUT_DIR}/${target}.hex
        COMMENT " =========== Generating HEX file  ==========="
    )

    # Generate the BIN file
    add_custom_command(
        TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} --strip-debug -O binary -R .eeprom $<TARGET_FILE:${target}> ${OUTPUT_DIR}/${target}.bin
        COMMENT " =========== Generating BIN file  =========== "
    )

    # Optional: Print file sizes
    if(CMAKE_SIZE)
        add_custom_command(
            TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_SIZE} -C --mcu=${MCU} $<TARGET_FILE:${target}>
            COMMENT " =========== ELF file size =========== "
        )
        add_custom_command(
            TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_SIZE} ${OUTPUT_DIR}/${target}.eep
            COMMENT " =========== EEPROM file size  =========== "
        )
    endif()
endfunction()
