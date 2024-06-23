/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Sysex non-realtime subid #1
typedef enum {
	SYSEX_NRT_SAMPLE_DUMP_HDR	 = 0x01, // Sample dump header
	SYSEX_NRT_SAMPLE_DUMP_PACKET = 0x02, // Sample dump packet
	SYSEX_NRT_SAMPLE_DUMP_REQ	 = 0x03, // Sample dump request
	SYSEX_NRT_MTC_CUEING		 = 0x04, // MIDI time code cueing
	SYSEX_NRT_SAMPLE_DUMPEXT	 = 0x05, // Sample dump extension
	SYSEX_NRT_GENERAL_SYS		 = 0x06, // General information
	SYSEX_NRT_FILE_DUMP			 = 0x07, // File dump
	SYSEX_NRT_TUNING_STANDARD	 = 0x08, // MIDI tuning standard
	SYSEX_NRT_GM				 = 0x09, // General MIDI
	SYSEX_NRT_DLS				 = 0x0A, // Downloadable sounds
	SYSEX_NRT_END_OF_FILE		 = 0x7B, // End of file
	SYSEX_NRT_WAIT				 = 0x7C, // Wait
	SYSEX_NRT_CANCEL			 = 0x7D, // Cancel
	SYSEX_NRT_NAK				 = 0x7E, // NAK
	SYSEX_NRT_ACK				 = 0x7F, // ACK
} midi_sysex_nrt_e;

// Sysex non-realtime - midi timecode
typedef enum {
	SYSEX_NRT_MTC_SPECIAL			 = 0x00,
	SYSEX_NRT_MTC_PUNCH_IN_POINTS	 = 0x01,
	SYSEX_NRT_MTC_PUNCH_OUT_POINTS	 = 0x02,
	SYSEX_NRT_MTC_PUNCH_IN_DELETE	 = 0x03,
	SYSEX_NRT_MTC_PUNCH_OUT_DELETE	 = 0x04,
	SYSEX_NRT_MTC_EVENT_START_POINT	 = 0x05,
	SYSEX_NRT_MTC_EVENT_STOP_POINT	 = 0x06,
	SYSEX_NRT_MTC_EVENT_START_ADDT	 = 0x07,
	SYSEX_NRT_MTC_EVENT_STOP_ADDT	 = 0x08,
	SYSEX_NRT_MTC_EVENT_START_DELETE = 0x09,
	SYSEX_NRT_MTC_EVENT_STOP_DELETE	 = 0x0A,
	SYSEX_NRT_MTC_CUE_POINTS		 = 0x0B,
	SYSEX_NRT_MTC_CUE_POINTS_ADDT	 = 0x0C,
	SYSEX_NRT_MTC_CUE_POINT_DELETE	 = 0x0D,
	SYSEX_NRT_MTC_EVENT_NAME_ADDT	 = 0x0E,
} midi_sysex_nrt_mtc_e;

// Sysex non-realtime - sample dump extension
typedef enum {
	SYSEX_NRT_SAMPLE_DUMPEXT_LOOP_POINT_TX		= 0x01,
	SYSEX_NRT_SAMPLE_DUMPEXT_LOOP_POINT_REQ		= 0x02,
	SYSEX_NRT_SAMPLE_DUMPEXT_NAME_TX			= 0x03,
	SYSEX_NRT_SAMPLE_DUMPEXT_NAME_REQ			= 0x04,
	SYSEX_NRT_SAMPLE_DUMPEXT_HEADER				= 0x05,
	SYSEX_NRT_SAMPLE_DUMPEXT_LOOP_POINT_TX_EXT	= 0x06,
	SYSEX_NRT_SAMPLE_DUMPEXT_LOOP_POINT_REQ_EXT = 0x07,
} midi_sysex_nrt_sample_dump_e;

// Sysex non-realtime - general system information
typedef enum {
	SYSEX_NRT_GENERAL_SYS_ID_REQ = 0x01,
	SYSEX_NRT_GENERAL_SYS_ID_RES = 0x02,
} midi_general_nrt_sys_e;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
