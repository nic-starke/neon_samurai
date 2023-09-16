/*
 * File: Settings.h ( 22nd November 2021 )
 * Project: Muffin
 * Copyright 2021 bxzn (mail@bxzn.one)
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

/* Faster updates can reduce flickering (some people may be sensitive)
 * But this will slow down input processing */

#define DISPLAY_REFRESH_TEST   (800)
#define DISPLAY_REFRESH_SLOW   (80)
#define DISPLAY_REFRESH_AVG    (40)
#define DISPLAY_REFRESH_FAST   (10)
#define DISPLAY_REFRESH_INSANE (5)

// The display refresh rate directly sets the timer interrupt rate for the display DMA transfer
// Reducing the interval will cause main loop starvation.
#define DISPLAY_REFRESH_RATE (DISPLAY_REFRESH_AVG) 

#define INPUT_SCAN_SLOW   (40)
#define INPUT_SCAN_AVG    (10)
#define INPUT_SCAN_FAST   (5)
#define INPUT_SCAN_INSANE (1)

// The input can rate directly sets the timer interrupt rate for input scanning
// Reducing the interval will cause main loop starvation.
#define INPUT_SCAN_RATE (INPUT_SCAN_AVG)