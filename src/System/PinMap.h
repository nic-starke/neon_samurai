/*
 * File: PinMap.h ( 14th November 2021 )
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

// No guard

// #define SR_ENABLE				IOPORT_CREATE_PIN(PORTD, 0)
// #define SR_CLK					IOPORT_CREATE_PIN(PORTD, 1)
// #define SR_DATA					IOPORT_CREATE_PIN(PORTD, 3)
// #define SR_LATCH				IOPORT_CREATE_PIN(PORTD, 4)
// #define SR_RESET				IOPORT_CREATE_PIN(PORTD, 5)

// clang-format off

#ifndef PINMAP
#define PINMAP(module, desc, port, pin, dir, mode, active, init)
#endif
//              module          desc       port        pin     direction      mode                    active       init
#define PINMAP( DISPLAY_SR,     ENABLE,    PORTD,      0,      OUTPUT_PORT,   PORT_MODE_PULL_DOWN,    false,       LOW)
#define PINMAP( DISPLAY_SR,     CLOCK,     PORTD,      1,      OUTPUT_PORT,   PORT_MODE_PULL_DOWN,    false,       LOW)
#define PINMAP( DISPLAY_SR,     DATA,      PORTD,      3,      OUTPUT_PORT,   PORT_MODE_PULL_DOWN,    false,       LOW)
#define PINMAP( DISPLAY_SR,     LATCH,     PORTD,      4,      OUTPUT_PORT,   PORT_MODE_PULL_DOWN,    HIGH,        LOW)
#define PINMAP( DISPLAY_SR,     RESET,     PORTD,      5,      OUTPUT_PORT,   PORT_MODE_PULL_DOWN,    LOW,         LOW)

#undef PINMAP

// clang-format on