# Default C compiler flags and arguments
include(${CMAKE_CURRENT_LIST_DIR}/utils/compiler/CheckAndApplyFlags.cmake)

set(default_cc_flags
# Diagonistics
	-ffunction-sections               # Place each function in its own section (ELF Only)
	-fdata-sections                   # Place each data in its own section (ELF Only)
	-fdiagnostics-show-option         # Show the corresponding warning option for each diagnostic
	-fcolor-diagnostics               # Use colors in diagnostics

# Warnings
	-Wall                             # Enable all common compiler warnings
	-Wpedantic                        # Warn about violations of strict ISO C and ISO C++
	-Wextra                           # Enable extra compiler warnings

# Errors
	# -Werror # All warnings are treated as errors
	-Wcast-align                      # Warn about casts that increase alignment requirements
	-Wcast-align=strict               # Warn about casts that increase alignment requirements (strict mode)
	-Wcast-qual                       # Warn about casts that remove a type qualifier from the type
	-Wconversion                      # Warn for implicit conversions that may alter a value
	-Wdisabled-optimization           # Warn about optimizations that are disabled
	-Wdouble-promotion                # Warn about implicit conversions from 'float' to 'double'
	-Wduplicated-cond                 # Warn about duplicated conditions in an if-else-if chain
	-Werror-implicit-function-declaration # Treat implicit function declarations as errors
	-Wfloat-equal                     # Warn if floating-point values are used in equality operations
	-Wformat-truncation               # Warn about truncated output strings
	-Wformat=2                        # Warn about non-literal format strings
	-Wincompatible-pointer-types      # Warn about incompatible pointer types
	-Winline                          # Warn about functions that cannot be inlined
	-Winvalid-pch                     # Warn about invalid precompiled header files
	-Wlogical-op                      # Warn about suspicious uses of logical operators
	-Wmissing-declarations            # Warn about global functions that are not declared
	-Wmissing-include-dirs            # Warn if a user-supplied include directory does not exist
	-Wnull-dereference                # Warn about dereferencing a null pointer
	-Wpointer-arith                   # Warn about pointer arithmetic that is not portable
	-Wredundant-decls                 # Warn about redundant declarations
	-Wreturn-type                     # Warn about inconsistent return type
	-Wshadow                          # Warn when a local variable shadows another local variable
	-Wshift-overflow=2                # Warn about shift operations that may cause overflow
	-Wsign-conversion                 # Warn about implicit conversions that may change the sign of an integer value
	-Wstrict-overflow                 # Warn about strict violations of the C99 Strict Aliasing Rules
	-Wswitch-enum                     # Warn about switch statements that do not handle all enum values
	-Wtrampolines                     # Warn about trampolines generated for nested functions
	-Wundef                           # Warn if an undefined identifier is evaluated in '#if'
	-Wunused                          # Warn about unused variables, functions, labels, etc.
	-Wunused-function                 # Warn about unused functions
	-Wvector-operation-performance    # Warn about suboptimal vector operation performance
	-Wwrite-strings                   # Warn about string literals being written to

	# Disabled warnings
	# -Wno-padded                       # Disable warnings about padded structs
	# -Wno-lto-type-mismatch # Disable warnings about type mismatches in Link Time Optimization (LTO)
)

set(default_link_flags
 	-flto
 	-fuse-ld=mold
 	-Wl,-Map=output.map               # Generate a load map file
	-Wl,--gc-sections                 # Enable link-time garbage collection
	-Wl,--build-id                    # Generate build ID
	-Wl,-z,defs                       # Disallow undefined symbols
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  apply_supported_compiler_flags_globally(C default_cc_flags)
  apply_supported_compiler_flags_globally(CXX default_cc_flags)
	apply_supported_linker_flags_globally_using_compiler(C default_link_flags)
endif()
