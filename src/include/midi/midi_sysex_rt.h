/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
	SYSEX_RT_MTC						 = 0x01,
	SYSEX_RT_NOTATION				 = 0x03,
	SYSEX_RT_DEV_CTRL				 = 0x04,
	SYSEX_RT_MTC_CUEING			 = 0x05,
	SYSEX_RT_MMC						 = 0x06,
	SYSEX_RT_TUNING_STANDARD = 0x08,
} midi_sysex_rt_e;

typedef enum {
	SYSEX_RT_MTC_FULL_MSG	 = 0x01,
	SYSEX_RT_MTC_USER_BITS = 0x02,
} midi_sysex_rt_mtc_e;

typedef enum {
	SYSEX_RT_NOTATION_BAR_MARKER				 = 0x01,
	SYSEX_RT_NOTATION_TIME_SIG_IMMEDIATE = 0x02,
	SYSEX_RT_NOTATION_TIME_SIG_DELAYED	 = 0x42,
} midi_sysex_rt_notation_e;

typedef enum {
	SYSEX_RT_DEV_CTRL_MASTER_VOL				 = 0x01,
	SYSEX_RT_DEV_CTRL_MASTER_BAL				 = 0x02,
	SYSEX_RT_DEV_CTRL_MASTER_FINE_TUNE	 = 0x03,
	SYSEX_RT_DEV_CTRL_MASTER_COARSE_TUNE = 0x04,
} midi_sysex_rt_dev_ctrl_e;

typedef enum {
	SYSEX_RT_MTC_CUE_PUNCH_IN_POINTS				= 0x01,
	SYSEX_RT_MTC_CUE_PUNCH_OUT_POINTS				= 0x02,
	SYSEX_RT_MTC_CUE_EVENT_START_POINT			= 0x05,
	SYSEX_RT_MTC_CUE_EVENT_STOP_POINT				= 0x06,
	SYSEX_RT_MTC_CUE_EVENT_START_ADDT_POINT = 0x07,
	SYSEX_RT_MTC_CUE_EVENT_STOP_ADDT_POINT	= 0x08,
	SYSEX_RT_MTC_CUE_POINTS									= 0x0B,
	SYSEX_RT_MTC_CUE_POINTS_ADDT						= 0x0C,
	SYSEX_RT_MTC_CUE_EVENT_NAME_ADDT				= 0x0E,
} midi_sysex_rt_mtc_cueing_e;

typedef enum {
	SYSEX_RT_MMC_STOP								 = 0x01,
	SYSEX_RT_MMC_PLAY								 = 0x02,
	SYSEX_RT_MMC_DEFERRED_PLAY			 = 0x03,
	SYSEX_RT_MMC_FAST_FORWARD				 = 0x04,
	SYSEX_RT_MMC_REWIND							 = 0x05,
	SYSEX_RT_MMC_RECORD_STROBE			 = 0x06,
	SYSEX_RT_MMC_RECORD_EXIT				 = 0x07,
	SYSEX_RT_MMC_RECORD_PAUSE				 = 0x08,
	SYSEX_RT_MMC_PAUSE							 = 0x09,
	SYSEX_RT_MMC_EJECT							 = 0x0A,
	SYSEX_RT_MMC_CHASE							 = 0x0B,
	SYSEX_RT_MMC_COMMAND_ERROR_RESET = 0x0C,
	SYSEX_RT_MMC_RESET							 = 0x0D,
	SYSEX_RT_MMC_LOCATE							 = 0x44,
	SYSEX_RT_MMC_SHUTTLE						 = 0x47,
} midi_sysex_rt_mmc_e;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
