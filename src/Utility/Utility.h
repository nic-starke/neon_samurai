/*
 * File: Utility.h ( 16th November 2021 )
 * Project: Muffin
 * Copyright 2021 Nicolaus Starke  
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

#include "DataTypes.h"

#define SET_REG(reg, val) ((reg) |= (val))
#define CLR_REG(reg, val) ((reg) &= ~(val))

// Check if a value is within a range (inclusive)
#define IN_RANGE(x, min, max) (((x) >= min) && ((x) <= max))