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

#ifndef MIDI_CMD_DEF
#define MIDI_CMD_DEF(type, val, str, dSize)
#endif

MIDI_CMD_DEF(NOTE_OFF, 0x80, "Note Off", 2)
MIDI_CMD_DEF(NOTE_ON, 0x90, "Note On", 2)
MIDI_CMD_DEF(AFTERTOUCH, 0xa0, "Aftertouch", 2)
MIDI_CMD_DEF(CC, 0xb0, "Control Change", 2)
MIDI_CMD_DEF(PC, 0xc0, "Program Change", 1)
MIDI_CMD_DEF(CHANNEL_PRESSURE, 0xd0, "Channel Pressure", 1)
MIDI_CMD_DEF(PITCH_BEND, 0xe0, "Pitch Bend", 2)
MIDI_CMD_DEF(SYSEX_START, 0xf0, "SysEx Start", 0)
MIDI_CMD_DEF(MTC_QUARTER, 0xf1, "MTC Quarter Frame", 2)
MIDI_CMD_DEF(SONG_POS, 0xf2, "Song Position", 2)
MIDI_CMD_DEF(SONG_SELECT, 0xf3, "Song Select", 2)
MIDI_CMD_DEF(TUNE_REQUEST, 0xf6, "Tune Request", 0)
MIDI_CMD_DEF(SYSEX_END, 0xf7, "SysEx End", 0)
MIDI_CMD_DEF(CLOCK, 0xf8, "Clock", 0)
MIDI_CMD_DEF(START, 0xfa, "Start", 0)
MIDI_CMD_DEF(CONTINUE, 0xfb, "Continue", 0)
MIDI_CMD_DEF(STOP, 0xfc, "Stop", 0)
MIDI_CMD_DEF(SENSING, 0xfe, "Active Sensing", 0)
MIDI_CMD_DEF(RESET, 0xff, "Reset", 0)

#undef MIDI_CMD_DEF

#ifndef MIDI_CC_DEF
#define MIDI_CC_DEF(type, val, str)
#endif

MIDI_CC_DEF(MSB_BANK, 0x00, "Bank selection")
MIDI_CC_DEF(MSB_MODWHEEL, 0x01, "Modulation")
MIDI_CC_DEF(MSB_BREATH, 0x02, "Breath")
MIDI_CC_DEF(MSB_FOOT, 0x04, "Foot")
MIDI_CC_DEF(MSB_PORTAMENTO_TIME, 0x05, "Portamento time")
MIDI_CC_DEF(MSB_DATA_ENTRY, 0x06, "Data entry")
MIDI_CC_DEF(MSB_MAIN_VOLUME, 0x07, "Main volume")
MIDI_CC_DEF(MSB_BALANCE, 0x08, "Balance")
MIDI_CC_DEF(MSB_PAN, 0x0a, "Panpot")
MIDI_CC_DEF(MSB_EXPRESSION, 0x0b, "Expression")
MIDI_CC_DEF(MSB_EFFECT1, 0x0c, "Effect1")
MIDI_CC_DEF(MSB_EFFECT2, 0x0d, "Effect2")
MIDI_CC_DEF(MSB_GENERAL_PURPOSE1, 0x10, "General purpose 1")
MIDI_CC_DEF(MSB_GENERAL_PURPOSE2, 0x11, "General purpose 2")
MIDI_CC_DEF(MSB_GENERAL_PURPOSE3, 0x12, "General purpose 3")
MIDI_CC_DEF(MSB_GENERAL_PURPOSE4, 0x13, "General purpose 4")
MIDI_CC_DEF(LSB_BANK, 0x20, "Bank selection")
MIDI_CC_DEF(LSB_MODWHEEL, 0x21, "Modulation")
MIDI_CC_DEF(LSB_BREATH, 0x22, "Breath")
MIDI_CC_DEF(LSB_FOOT, 0x24, "Foot")
MIDI_CC_DEF(LSB_PORTAMENTO_TIME, 0x25, "Portamento time")
MIDI_CC_DEF(LSB_DATA_ENTRY, 0x26, "Data entry")
MIDI_CC_DEF(LSB_MAIN_VOLUME, 0x27, "Main volume")
MIDI_CC_DEF(LSB_BALANCE, 0x28, "Balance")
MIDI_CC_DEF(LSB_PAN, 0x2a, "Panpot")
MIDI_CC_DEF(LSB_EXPRESSION, 0x2b, "Expression")
MIDI_CC_DEF(LSB_EFFECT1, 0x2c, "Effect1")
MIDI_CC_DEF(LSB_EFFECT2, 0x2d, "Effect2")
MIDI_CC_DEF(LSB_GENERAL_PURPOSE1, 0x30, "General purpose 1")
MIDI_CC_DEF(LSB_GENERAL_PURPOSE2, 0x31, "General purpose 2")
MIDI_CC_DEF(LSB_GENERAL_PURPOSE3, 0x32, "General purpose 3")
MIDI_CC_DEF(LSB_GENERAL_PURPOSE4, 0x33, "General purpose 4")
MIDI_CC_DEF(SUSTAIN, 0x40, "Sustain pedal")
MIDI_CC_DEF(PORTAMENTO, 0x41, "Portamento")
MIDI_CC_DEF(SOSTENUTO, 0x42, "Sostenuto")
MIDI_CC_DEF(SUSTENUTO, 0x42, "Sostenuto (a typo in the older version)")
MIDI_CC_DEF(SOFT_PEDAL, 0x43, "Soft pedal")
MIDI_CC_DEF(LEGATO_FOOTSWITCH, 0x44, "Legato foot switch")
MIDI_CC_DEF(HOLD2, 0x45, "Hold2")
MIDI_CC_DEF(SC1_SOUND_VARIATION, 0x46, "SC1 Sound Variation")
MIDI_CC_DEF(SC2_TIMBRE, 0x47, "SC2 Timbre")
MIDI_CC_DEF(SC3_RELEASE_TIME, 0x48, "SC3 Release Time")
MIDI_CC_DEF(SC4_ATTACK_TIME, 0x49, "SC4 Attack Time")
MIDI_CC_DEF(SC5_BRIGHTNESS, 0x4a, "SC5 Brightness")
MIDI_CC_DEF(SC6, 0x4b, "SC6")
MIDI_CC_DEF(SC7, 0x4c, "SC7")
MIDI_CC_DEF(SC8, 0x4d, "SC8")
MIDI_CC_DEF(SC9, 0x4e, "SC9")
MIDI_CC_DEF(SC10, 0x4f, "SC10")
MIDI_CC_DEF(GENERAL_PURPOSE5, 0x50, "General purpose 5")
MIDI_CC_DEF(GENERAL_PURPOSE6, 0x51, "General purpose 6")
MIDI_CC_DEF(GENERAL_PURPOSE7, 0x52, "General purpose 7")
MIDI_CC_DEF(GENERAL_PURPOSE8, 0x53, "General purpose 8")
MIDI_CC_DEF(PORTAMENTO_CONTROL, 0x54, "Portamento control")
MIDI_CC_DEF(E1_REVERB_DEPTH, 0x5b, "E1 Reverb Depth")
MIDI_CC_DEF(E2_TREMOLO_DEPTH, 0x5c, "E2 Tremolo Depth")
MIDI_CC_DEF(E3_CHORUS_DEPTH, 0x5d, "E3 Chorus Depth")
MIDI_CC_DEF(E4_DETUNE_DEPTH, 0x5e, "E4 Detune Depth")
MIDI_CC_DEF(E5_PHASER_DEPTH, 0x5f, "E5 Phaser Depth")
MIDI_CC_DEF(DATA_INCREMENT, 0x60, "Data Increment")
MIDI_CC_DEF(DATA_DECREMENT, 0x61, "Data Decrement")
MIDI_CC_DEF(NONREG_PARM_NUM_LSB, 0x62, "Non-registered parameter number")
MIDI_CC_DEF(NONREG_PARM_NUM_MSB, 0x63, "Non-registered parameter number")
MIDI_CC_DEF(REGIST_PARM_NUM_LSB, 0x64, "Registered parameter number")
MIDI_CC_DEF(REGIST_PARM_NUM_MSB, 0x65, "Registered parameter number")
MIDI_CC_DEF(ALL_SOUNDS_OFF, 0x78, "All sounds off")
MIDI_CC_DEF(RESET_CONTROLLERS, 0x79, "Reset Controllers")
MIDI_CC_DEF(LOCAL_CONTROL_SWITCH, 0x7a, "Local control switch")
MIDI_CC_DEF(ALL_NOTES_OFF, 0x7b, "All notes off")
MIDI_CC_DEF(OMNI_OFF, 0x7c, "Omni off")
MIDI_CC_DEF(OMNI_ON, 0x7d, "Omni on")
MIDI_CC_DEF(MONO1, 0x7e, "Mono1")
MIDI_CC_DEF(MONO2, 0x7f, "Mono2")

#undef MIDI_CC_DEF