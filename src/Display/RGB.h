/*
 * File: RGB.h ( 28th November 2021 )
 * Project: Muffin
 * Copyright 2021 Nic Starke (mail@bxzn.one)
 * -----
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#pragma once

#include "Types.h"

#define RGB_MAX_VAL (UINT8_MAX)
#define RGB_MIN_VAL (0)

typedef struct
{
	u8 Red;
	u8 Green;
	u8 Blue;
} sRGB;

#define RGB_RED                                                                                                                            \
	{                                                                                                                                      \
		255, 0, 0                                                                                                                          \
	}
#define RGB_YELLOW                                                                                                                         \
	{                                                                                                                                      \
		255, 255, 0                                                                                                                        \
	}
#define RGB_GREEN                                                                                                                          \
	{                                                                                                                                      \
		0, 255, 0                                                                                                                          \
	}
#define RGB_CYAN                                                                                                                           \
	{                                                                                                                                      \
		0, 255, 255                                                                                                                        \
	}
#define RGB_BLUE                                                                                                                           \
	{                                                                                                                                      \
		0, 0, 255                                                                                                                          \
	}
#define RGB_FUSCHIA                                                                                                                        \
	{                                                                                                                                      \
		255, 0, 255                                                                                                                        \
	}
#define RGB_WHITE                                                                                                                          \
	{                                                                                                                                      \
		255, 255, 255                                                                                                                      \
	}
