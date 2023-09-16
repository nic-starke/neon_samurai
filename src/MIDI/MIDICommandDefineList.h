/*
 * File: MIDICommandDefineList.h ( 17th March 2022 )
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

// clang-format off
#ifndef MIDI_CMD_DEF
#define MIDI_CMD_DEF(type, str, val, dSize)
#endif

MIDI_CMD_DEF(NOTE_OFF,          "Note Off",         0x80,   2)
MIDI_CMD_DEF(NOTE_ON,           "Note On",          0x90,   2)
MIDI_CMD_DEF(AFTERTOUCH,        "Aftertouch",       0xa0,   2)
MIDI_CMD_DEF(CC,                "Control Change",   0xb0,   2)
MIDI_CMD_DEF(PC,                "Program Change",   0xc0,   1)
MIDI_CMD_DEF(CHANNEL_PRESSURE,  "Channel Pressure", 0xd0,   1)
MIDI_CMD_DEF(PITCH_BEND,        "Pitch Bend",       0xe0,   2)
MIDI_CMD_DEF(SYSEX_START,       "SysEx Start",      0xf0,   0)
MIDI_CMD_DEF(MTC_QUARTER,       "MTC Quarter Frame", 0xf1,  2)
MIDI_CMD_DEF(SONG_POS,          "Song Position",    0xf2,   2)
MIDI_CMD_DEF(SONG_SELECT,       "Song Select",      0xf3,   2)
MIDI_CMD_DEF(TUNE_REQUEST,      "Tune Request",     0xf6,   0)
MIDI_CMD_DEF(SYSEX_END,         "SysEx End",        0xf7,   0)
MIDI_CMD_DEF(CLOCK,             "Clock",            0xf8,   0)
MIDI_CMD_DEF(START,             "Start",            0xfa,   0)
MIDI_CMD_DEF(CONTINUE,          "Continue",         0xfb,   0)
MIDI_CMD_DEF(STOP,              "Stop",             0xfc,   0)
MIDI_CMD_DEF(SENSING,           "Active Sensing",   0xfe,   0)
MIDI_CMD_DEF(RESET,             "Reset",            0xff,   0)

#undef MIDI_CMD_DEF

#ifndef MIDI_SYSEX_NON_REALTIME_DEF
#define MIDI_SYSEX_NON_REALTIME_DEF(type, str, subid)
#endif

MIDI_SYSEX_NON_REALTIME_DEF(SAMPLE_DUMP_HDR,    "Sample Dump Header",       0x01)
MIDI_SYSEX_NON_REALTIME_DEF(SAMPLE_DUMP_PACKET, "Sample Dump Data Packet",  0x02)
MIDI_SYSEX_NON_REALTIME_DEF(SAMPLE_DUMP_REQ,    "Sample Dump Request",      0x03)
MIDI_SYSEX_NON_REALTIME_DEF(MTC_CUEING,         "MTC Cueing",               0x04)
MIDI_SYSEX_NON_REALTIME_DEF(SAMPLE_DUMPEXT,     "Sample Dump Extensions",   0x05)
MIDI_SYSEX_NON_REALTIME_DEF(GENERAL_SYS,        "General System Information", 0x06)
MIDI_SYSEX_NON_REALTIME_DEF(FILE_DUMP,          "File Dump",                0x07)
MIDI_SYSEX_NON_REALTIME_DEF(TUNING_STANDARD,    "Tuning Standard",          0x08)
MIDI_SYSEX_NON_REALTIME_DEF(GM,                 "General MIDI System",      0x09)
MIDI_SYSEX_NON_REALTIME_DEF(DLS,                "Downloadable Sounds System", 0x0A)
MIDI_SYSEX_NON_REALTIME_DEF(EOF,                "End of File",              0x7B)
MIDI_SYSEX_NON_REALTIME_DEF(WAIT,               "Wait",                     0x7C)
MIDI_SYSEX_NON_REALTIME_DEF(CANCEL,             "Cancel",                   0x7D)
MIDI_SYSEX_NON_REALTIME_DEF(NAK,                "NAK",                      0x7E)
MIDI_SYSEX_NON_REALTIME_DEF(ACK,                "ACK",                      0x7F)

#undef MIDI_SYSEX_NON_REALTIME_DEF

// MIDI_SYSEX_NON_REALTIME_DEF(MTC_SPECIAL,            "MTC Cueing", 0x04, "Special", 0x00)
// MIDI_SYSEX_NON_REALTIME_DEF(MTC_PUNCH_IN_POINTS,    "MTC Cueing", 0x04, "Punch In Points", 0x01)
// MIDI_SYSEX_NON_REALTIME_DEF(MTC_PUNCH_OUT_POINTS,   "MTC Cueing", 0x04, "Punch Out Points", 0x02)
// MIDI_SYSEX_NON_REALTIME_DEF(MTC_PUNCH_IN_DELETE,    "MTC Cueing", 0x04, "Delete Punch In Point", 0x03)
// MIDI_SYSEX_NON_REALTIME_DEF(MTC_PUNCH_OUT_DELETE,   "MTC Cueing", 0x04, "Delete Punch Out Point", 0x04)
// MIDI_SYSEX_NON_REALTIME_DEF(MTC_EVENT_START_POINT,  "MTC Cueing", 0x04, "Event Start Point", 0x05)
// MIDI_SYSEX_NON_REALTIME_DEF(MTC_EVENT STOP_POINT,   "MTC Cueing", 0x04, "Event Stop Point", 0x06)
// MIDI_SYSEX_NON_REALTIME_DEF(MTC_EVENT_START_ADDT,   "MTC Cueing", 0x04, "Event Start Points with additional info", 0x07)
// MIDI_SYSEX_NON_REALTIME_DEF(MTC_EVENT_STOP_ADDT,    "MTC Cueing", 0x04, "Event Stop Points with additional info", 0x08)
// MIDI_SYSEX_NON_REALTIME_DEF(MTC_EVENT_START_DELETE, "MTC Cueing", 0x04, "Delete Event Start Point", 0x09)
// MIDI_SYSEX_NON_REALTIME_DEF(MTC_EVENT_STOP_DELETE,  "MTC Cueing", 0x04, "Delete Event Stop Point", 0x0A)
// MIDI_SYSEX_NON_REALTIME_DEF(MTC_CUE_POINTS,         "MTC Cueing", 0x04, "Cue Points", 0x0B)
// MIDI_SYSEX_NON_REALTIME_DEF(MTC_CUE_POINTS_ADDT,    "MTC Cueing", 0x04, "Cue Points with additional info", 0x0C)
// MIDI_SYSEX_NON_REALTIME_DEF(MTC_CUE_POINT_DELETE,   "MTC Cueing", 0x04, "Delete Cue Point", 0x0D)
// MIDI_SYSEX_NON_REALTIME_DEF(MTC_EVENT_NAME_ADDT,    "MTC Cueing", 0x04, "Event name with additional info", 0x0E)

// MIDI_SYSEX_NON_REALTIME_DEF(SAMPLE_DUMPEXT_LOOP_POINT_TX,       "Sample Dump Extensions", 0x05, "Loop Point Transmission", 0x01)
// MIDI_SYSEX_NON_REALTIME_DEF(SAMPLE_DUMPEXT_LOOP_POINT_REQ,      "Sample Dump Extensions", 0x05, "Loop Point Request", 0x02)
// MIDI_SYSEX_NON_REALTIME_DEF(SAMPLE_DUMPEXT_NAME_TX,             "Sample Dump Extensions", 0x05, "Sample Name Transmission", 0x03)
// MIDI_SYSEX_NON_REALTIME_DEF(SAMPLE_DUMPEXT_NAME_REQ,            "Sample Dump Extensions", 0x05, "Sample Name Request", 0x04)
// MIDI_SYSEX_NON_REALTIME_DEF(SAMPLE_DUMPEXT_HEADER,              "Sample Dump Extensions", 0x05, "Extended Dump Header", 0x05)
// MIDI_SYSEX_NON_REALTIME_DEF(SAMPLE_DUMPEXT_LOOP_POINT_TX_EXT,   "Sample Dump Extensions", 0x05, "Extended Loop Point Transmission", 0x06)
// MIDI_SYSEX_NON_REALTIME_DEF(SAMPLE_DUMPEXT_LOOP_POINT_REQ_EXT,  "Sample Dump Extensions", 0x05, "Extended Loop Point Request", 0x07)

#ifndef GENERAL_SYS_DEF
#define GENERAL_SYS_DEF(type, str, subid)
#endif

GENERAL_SYS_DEF(ID_REQ, "Device Identity Request",  0x01)
GENERAL_SYS_DEF(ID_REP, "Device Identity Reply",    0x02)

#undef GENERAL_SYS_DEF

// MIDI_SYSEX_NON_REALTIME_DEF(FILE_DUMP_HDR, "File Dump", 0x07, "Header", 0x01)
// MIDI_SYSEX_NON_REALTIME_DEF(FILE_DUMP_PKT, "File Dump", 0x07, "Data Packet", 0x02)
// MIDI_SYSEX_NON_REALTIME_DEF(FILE_DUMP_REQ, "File Dump", 0x07, "Request", 0x03)

// MIDI_SYSEX_NON_REALTIME_DEF(MIDI_TUNE_DUMP_REQ, "MIDI Tuning Standard", 0x08, "Bulk Tuning Dump Request", 0x01)
// MIDI_SYSEX_NON_REALTIME_DEF(MIDI_TUNE_DUMP_REP, "MIDI Tuning Standard", 0x08, "Bulk Tuning Dump Reply", 0x02)

// MIDI_SYSEX_NON_REALTIME_DEF(MIDI_GM, "General MIDI (GM) System", 0x09, "Enable" 0x01)
// MIDI_SYSEX_NON_REALTIME_DEF(MIDI_GM, "General MIDI (GM) System", 0x09, "Disable" 0x02)

// MIDI_SYSEX_NON_REALTIME_DEF(MIDI_DLS, "Down-Loadable Sounds (DLS) System", 0x0A, "Enable", 0x01)
// MIDI_SYSEX_NON_REALTIME_DEF(MIDI_DLS, "Down-Loadable Sounds (DLS) System", 0x0A, "Disable", 0x02)

#ifndef MIDI_CC_DEF
#define MIDI_CC_DEF(type, str, val)
#endif

MIDI_CC_DEF(MSB_BANK, "Bank selection", 0x00)
MIDI_CC_DEF(MSB_MODWHEEL, "Modulation", 0x01)
MIDI_CC_DEF(MSB_BREATH, "Breath", 0x02)
MIDI_CC_DEF(MSB_FOOT, "Foot", 0x04)
MIDI_CC_DEF(MSB_PORTAMENTO_TIME, "Portamento time", 0x05)
MIDI_CC_DEF(MSB_DATA_ENTRY, "Data entry", 0x06)
MIDI_CC_DEF(MSB_MAIN_VOLUME, "Main volume", 0x07)
MIDI_CC_DEF(MSB_BALANCE, "Balance", 0x08)
MIDI_CC_DEF(MSB_PAN, "Panpot", 0x0a)
MIDI_CC_DEF(MSB_EXPRESSION, "Expression", 0x0b)
MIDI_CC_DEF(MSB_EFFECT1, "Effect1", 0x0c)
MIDI_CC_DEF(MSB_EFFECT2, "Effect2", 0x0d)
MIDI_CC_DEF(MSB_GENERAL_PURPOSE1, "General purpose 1", 0x10)
MIDI_CC_DEF(MSB_GENERAL_PURPOSE2, "General purpose 2", 0x11)
MIDI_CC_DEF(MSB_GENERAL_PURPOSE3, "General purpose 3", 0x12)
MIDI_CC_DEF(MSB_GENERAL_PURPOSE4, "General purpose 4", 0x13)
MIDI_CC_DEF(LSB_BANK, "Bank selection", 0x20)
MIDI_CC_DEF(LSB_MODWHEEL, "Modulation", 0x21)
MIDI_CC_DEF(LSB_BREATH, "Breath", 0x22)
MIDI_CC_DEF(LSB_FOOT, "Foot", 0x24)
MIDI_CC_DEF(LSB_PORTAMENTO_TIME, "Portamento time", 0x25)
MIDI_CC_DEF(LSB_DATA_ENTRY, "Data entry", 0x26)
MIDI_CC_DEF(LSB_MAIN_VOLUME, "Main volume", 0x27)
MIDI_CC_DEF(LSB_BALANCE, "Balance", 0x28)
MIDI_CC_DEF(LSB_PAN, "Panpot", 0x2a)
MIDI_CC_DEF(LSB_EXPRESSION, "Expression", 0x2b)
MIDI_CC_DEF(LSB_EFFECT1, "Effect1", 0x2c)
MIDI_CC_DEF(LSB_EFFECT2, "Effect2", 0x2d)
MIDI_CC_DEF(LSB_GENERAL_PURPOSE1, "General purpose 1", 0x30)
MIDI_CC_DEF(LSB_GENERAL_PURPOSE2, "General purpose 2", 0x31)
MIDI_CC_DEF(LSB_GENERAL_PURPOSE3, "General purpose 3", 0x32)
MIDI_CC_DEF(LSB_GENERAL_PURPOSE4, "General purpose 4", 0x33)
MIDI_CC_DEF(SUSTAIN, "Sustain pedal", 0x40)
MIDI_CC_DEF(PORTAMENTO, "Portamento", 0x41)
MIDI_CC_DEF(SOSTENUTO, "Sostenuto", 0x42)
MIDI_CC_DEF(SUSTENUTO, "Sostenuto (a typo in the older version)", 0x42)
MIDI_CC_DEF(SOFT_PEDAL, "Soft pedal", 0x43)
MIDI_CC_DEF(LEGATO_FOOTSWITCH, "Legato foot switch", 0x44)
MIDI_CC_DEF(HOLD2, "Hold2", 0x45)
MIDI_CC_DEF(SC1_SOUND_VARIATION, "SC1 Sound Variation", 0x46)
MIDI_CC_DEF(SC2_TIMBRE, "SC2 Timbre", 0x47)
MIDI_CC_DEF(SC3_RELEASE_TIME, "SC3 Release Time", 0x48)
MIDI_CC_DEF(SC4_ATTACK_TIME, "SC4 Attack Time", 0x49)
MIDI_CC_DEF(SC5_BRIGHTNESS, "SC5 Brightness", 0x4a)
MIDI_CC_DEF(SC6, "SC6", 0x4b)
MIDI_CC_DEF(SC7, "SC7", 0x4c)
MIDI_CC_DEF(SC8, "SC8", 0x4d)
MIDI_CC_DEF(SC9, "SC9", 0x4e)
MIDI_CC_DEF(SC10, "SC10", 0x4f)
MIDI_CC_DEF(GENERAL_PURPOSE5, "General purpose 5", 0x50)
MIDI_CC_DEF(GENERAL_PURPOSE6, "General purpose 6", 0x51)
MIDI_CC_DEF(GENERAL_PURPOSE7, "General purpose 7", 0x52)
MIDI_CC_DEF(GENERAL_PURPOSE8, "General purpose 8", 0x53)
MIDI_CC_DEF(PORTAMENTO_CONTROL, "Portamento control", 0x54)
MIDI_CC_DEF(E1_REVERB_DEPTH, "E1 Reverb Depth", 0x5b)
MIDI_CC_DEF(E2_TREMOLO_DEPTH, "E2 Tremolo Depth", 0x5c)
MIDI_CC_DEF(E3_CHORUS_DEPTH, "E3 Chorus Depth", 0x5d)
MIDI_CC_DEF(E4_DETUNE_DEPTH, "E4 Detune Depth", 0x5e)
MIDI_CC_DEF(E5_PHASER_DEPTH, "E5 Phaser Depth", 0x5f)
MIDI_CC_DEF(DATA_INCREMENT, "Data Increment", 0x60)
MIDI_CC_DEF(DATA_DECREMENT, "Data Decrement", 0x61)
MIDI_CC_DEF(NONREG_PARM_NUM_LSB, "Non-registered parameter number", 0x62)
MIDI_CC_DEF(NONREG_PARM_NUM_MSB, "Non-registered parameter number", 0x63)
MIDI_CC_DEF(REGIST_PARM_NUM_LSB, "Registered parameter number", 0x64)
MIDI_CC_DEF(REGIST_PARM_NUM_MSB, "Registered parameter number", 0x65)
MIDI_CC_DEF(ALL_SOUNDS_OFF, "All sounds off", 0x78)
MIDI_CC_DEF(RESET_CONTROLLERS, "Reset Controllers", 0x79)
MIDI_CC_DEF(LOCAL_CONTROL_SWITCH, "Local control switch", 0x7a)
MIDI_CC_DEF(ALL_NOTES_OFF, "All notes off", 0x7b)
MIDI_CC_DEF(OMNI_OFF, "Omni off", 0x7c)
MIDI_CC_DEF(OMNI_ON, "Omni on", 0x7d)
MIDI_CC_DEF(MONO1, "Mono1", 0x7e)
MIDI_CC_DEF(MONO2, "Mono2", 0x7f)

#undef MIDI_CC_DEF