/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*
	A record of what this firmware is, left in flash where a host can find it.

	The device reports its version over sysex, but only while it is running.
	A device sitting in the bootloader has no MIDI interface at all, and the
	bootloader can do nothing but read and write memory - so the only way to
	answer "what is on this thing" is to read it out of the flash itself.

	Hashing the image and comparing it against a known release answers that
	too, but only for releases: it cannot name a development build, and it
	says nothing useful when it does not match. This can.

	It is found by searching the flash for FWINFO_ID. That string is both the
	marker and the identity - a fork would change it, and in doing so stop its
	firmware being mistaken for this one, which is the right outcome.

	Searching rather than reading a fixed address is deliberate. Pinning it low
	means a linker script, because the vector table and code already occupy
	that space; pinning it high means every write spans the whole 128 KB rather
	than the 19 KB an image actually uses. The host is reading flash anyway to
	check what is there, and searching those bytes costs nothing.
*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define FWINFO_ID							"NEON_SAMURAI"
#define FWINFO_ID_LEN					(16)
#define FWINFO_FORMAT_VERSION (1u)
#define FWINFO_COMMIT_LEN			(12u)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct fwinfo {
	u8 format;

	char id[FWINFO_ID_LEN];
	char commit[FWINFO_COMMIT_LEN];

	u8 version_major;
	u8 version_minor;
	u8 version_patch;
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

extern const struct fwinfo fw_info;
