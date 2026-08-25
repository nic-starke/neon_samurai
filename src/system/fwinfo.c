/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/pgmspace.h>

#include "system/types.h"
#include "system/fwinfo.h"
#include "system/project.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Supplied by the build system. A build from a tarball, or without git, has no
// commit to record.
#ifndef GIT_COMMIT
#define GIT_COMMIT ""
#endif

_Static_assert(sizeof(FWINFO_ID) <= FWINFO_ID_LEN,
							 "the identifier does not fit its field");

_Static_assert(sizeof(GIT_COMMIT) <= FWINFO_COMMIT_LEN,
							 "the commit string does not fit its field");

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
	Nothing in the firmware reads this - it exists to be read out of flash by
	something else - so both the compiler and the linker would happily discard
	it. `used` stops the compiler, and CMakeLists passes -u,fw_info to stop
	--gc-sections. That is the same arrangement bootloader_check needs, for the
	same reason.

	PROGMEM, so it costs flash rather than SRAM - the scarce one here.
*/
const struct fwinfo fw_info __attribute__((used)) PROGMEM = {
		.id						 = FWINFO_ID,
		.format				 = FWINFO_FORMAT_VERSION,
		.version_major = VERSION_MAJOR,
		.version_minor = VERSION_MINOR,
		.version_patch = VERSION_PATCH,
		.commit				 = GIT_COMMIT,
};
