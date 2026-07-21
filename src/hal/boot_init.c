/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * .init3 trampoline for bootloader_check().
 *
 * A top-level asm directive places a `call` in the .init3 section so that
 * bootloader_check() runs before main(). This file MUST be compiled with
 * -fno-lto; the LTO linker plugin discards section attributes from C
 * functions, which would silently remove the .init3 placement.
 */

extern void bootloader_check(void);

__asm__(".section .init3,\"ax\",@progbits\n\t"
        "call bootloader_check\n\t"
        ".section .text\n");
