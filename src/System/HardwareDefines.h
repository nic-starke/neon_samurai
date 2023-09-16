/*
 * File: HardwareDefines.h ( 6th November 2021 )
 * Project: Muffin
 * Copyright 2021 - 2021 Nic Starke (mail@bxzn.one)
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

#define NUM_ENCODERS		 (16)
#define NUM_ENCODER_SWITCHES (NUM_ENCODERS)
#define NUM_SIDE_SWITCHES	 (6)

/* per encoder */
#define NUM_IND_LEDS (11)
#define NUM_RGB_LEDS (3)
#define NUM_DET_LEDS (2)

/* total */
#define NUM_LED_SHIFTREG   (2 * NUM_ENCODERS)
#define NUM_INPUT_SHIFTREG (6) /* 4 for rotary channels A/B, and 2 for the encoder switches */