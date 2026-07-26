/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2026) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*
	Host-build shim for <avr/io.h>.

	Only src/include/hal/sys.h includes <avr/io.h>, and only for register
	definitions that the pure-logic modules under test never touch. This shim
	exists so those modules can be compiled for the host without dragging in
	avr-libc.

	If a module under test starts genuinely needing a register definition, that
	is a signal the module has a hardware dependency that should be injected
	rather than a signal to grow this file.
*/
