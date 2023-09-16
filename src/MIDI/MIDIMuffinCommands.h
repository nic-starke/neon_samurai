/*
 * File: MIDIMuffinCommands.h ( 17th March 2022 )
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

/*  
    The following commands are unique to the muffin firmware, and must be sent over sysex.
    A message byte sequence is as follows:

    SysEx Header             (6 bytes)
    0xF0,                   - start of sysex
    0x00, 0x48, 0x01,       - manufacturer id
    0x00, 0x01              - product id

    $CMD        -   Muffin command number   (1 byte)
    [n]         -   Muffin command data     (n bytes)
    F7          -   SysEx End               (1 byte)

    A response is sent for every received command, the response types will be VALID or INVALID
    For commands that require data (such as SET_USB_VID_PID), if the number of bytes does not match inSize
    as defined below, then an INVALID_CMD response is sent.
    If the data is not received within a timeout period, a timeout is sent, and the message parser
    is then reset - new messages can then be parsed.
*/

// clang-format off

#ifndef MUFFIN_MIDICMD_DEF

#endif

MUFFIN_MIDICMD_DEF(INVALID_CMD,              "Received command was invalid",  0x00,  0, 0)
MUFFIN_MIDICMD_DEF(VALID_CMD,                "Received command was valid",    0x01,  0, 0)
MUFFIN_MIDICMD_DEF(TIMEOUT,                  "Timeout waiting for data",      0x02,  0, 0)
MUFFIN_MIDICMD_DEF(RESET,                    "Reset",                         0x03,  0, 0)
MUFFIN_MIDICMD_DEF(MED_CONNECT,              "Muffin Editor - Connect",       0x0C,  0, 0)
MUFFIN_MIDICMD_DEF(MED_DISCONNECT,           "Muffin Editor - Disconnect",    0x0D,  0, 0)
MUFFIN_MIDICMD_DEF(SET_MIDI_PRODUCT_STR,     "Set MIDI product string",       0x0E, 30, 0)
MUFFIN_MIDICMD_DEF(SET_USB_VID_PID,          "Set USB VID/PID",               0x0F,  8, 0)

#undef MUFFIN_MIDICMD_DEF