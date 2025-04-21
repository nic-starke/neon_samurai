/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2025) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#ifndef COMMON_CONSOLE_CONSOLE_H
#define COMMON_CONSOLE_CONSOLE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
 * @brief Prints the production signature row values to the console.
 *
 * This function reads each non-reserved field from the production signature row
 * and prints its name and value to the console.
 */
void console_print_signature_row(void);

#ifdef __cplusplus
}
#endif

#endif // COMMON_CONSOLE_CONSOLE_H
