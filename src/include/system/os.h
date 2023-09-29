/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "atom.h"
#include "atommutex.h"

#include "types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum {
  THREAD_SYS,
  THREAD_INPUT,
  THREAD_FILES,
  THREAD_DISPLAY,
  THREAD_MIDI,
  THREAD_NETWORK,
  THREAD_USB,

  THREAD_NB,
} thread_id;

typedef ATOM_MUTEX os_mutex_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Initialises the operating system. Does not start the first thread.
 *
 * @return int 0 on success, otherwise sys err code.
 */
int os_init(void);

/**
 * @brief Start the OS and begin scheduling any threads that have been created.
 * @note No return unless there is a problem!
 */
void os_start(void);

/**
 * @brief Creates and starts the thread.
 *
 * @param thread The thread ID.
 * @param entry The entry function.
 * @param arg Data to pass to the entry function.
 * @return int
 * @retval 0 - success.
 * @retval Anything else - error.
 */
int os_thread_start(thread_id thread, void (*entry)(uint32_t), uint32_t arg);

int os_mutex_init(os_mutex_t* mutex);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
