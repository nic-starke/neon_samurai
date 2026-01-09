/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2025) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifndef COMMON_CONSOLE_CONSOLE_H
#define COMMON_CONSOLE_CONSOLE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>				// required for  snprintf_P
#include <avr/pgmspace.h> // for PSTR and PROGMEN macros

#ifdef __cplusplus
extern "C" {
#endif

#define CONSOLE_LINE_BUFFER_SIZE                                               \
	128 // Define the size of the console line buffer

/**
 * @brief Initializes the console module.
 */
void console_init(void);

/**
 * @brief Updates the console module state.
 *
 * This function should be called periodically in the main loop.
 * It checks for incoming serial data, processes commands, and handles output.
 */
void console_update(void);

/**
 * @brief Sends a single character to the console.
 *
 * @param c The character to send.
 */
void console_putc(char c);

/**
 * @brief Sends a null-terminated string to the console.
 *
 * @param str The string to send.
 */
void console_puts(const char* str);

/**
 * @brief Sends a null-terminated string from program memory to the console.
 *
 * @param str_p Pointer to the string in program memory.
 */
void console_puts_p(const char* str_p);

/**
 * @brief Prints an integer value to the console.
 *
 * @param value The integer value to print.
 */
#define console_put_int(value)	 console_put_value(PSTR("%d\r\n"), value)

/**
 * @brief Prints an unsigned integer value to the console.
 *
 * @param value The unsigned integer value to print.
 */
#define console_put_uint(value)	 console_put_value(PSTR("%u\r\n"), value)

/**
 * @brief Prints a floating point value to the console.
 *
 * @param value The floating point value to print.
 */
#define console_put_float(value) console_put_value(PSTR("%.2f\r\n"), value)

/**
 * @brief Prints a floating point value without newline to the console.
 *
 * @param value The floating point value to print.
 */
void console_put_float_val(float value);

/**
 * @brief Prints a hexadecimal value to the console.
 *
 * @param value The hexadecimal value to print.
 */
#define console_put_hex(value)	 console_put_value(PSTR("0x%08X\r\n"), value)

/**
 * @brief Prints the production signature row values to the console.
 *
 * This function reads each non-reserved field from the production signature row
 * and prints its name and value to the console.
 */
void console_print_signature_row(void);

#define console_put_value(format_pstr, value)                                  \
	do {                                                                         \
		char buffer[CONSOLE_LINE_BUFFER_SIZE];                                     \
		snprintf_P(buffer, sizeof(buffer), format_pstr, value);                    \
		console_puts(buffer);                                                      \
	} while (0)

#ifdef __cplusplus
}
#endif

#endif // COMMON_CONSOLE_CONSOLE_H
