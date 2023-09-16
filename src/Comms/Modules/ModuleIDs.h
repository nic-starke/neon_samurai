/*
 * File: ModuleIDs.h ( 26th March 2022 )
 * Project: Muffin
 * Copyright 2022 bxzn (mail@bxzn.one)
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

// To be used for special messages that do not originate from comms-enabled modules.
// FIXME - this could just another enum that after the NUM_MODULE_IDs
#define INVALID_MODULE_ID (0xFF)

typedef enum
{
    MODULE_NETWORK,

    NUM_MODULE_IDS,
} eModuleID;

/**
 * @brief Checks if a module ID is valid.
 */
static inline bool IsValidModuleID(u8 ModuleID)
{
    return ((ModuleID >= 0) && ModuleID < NUM_MODULE_IDS);
}