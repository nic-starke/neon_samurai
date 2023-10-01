/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "atom.h"
#include "atommutex.h"

#include "system/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef ATOM_MUTEX os_mutex_t;
typedef ATOM_TCB   os_tcb_t;

struct os_thread_s;

typedef struct {
  uint8_t* const      stack;      // Pointer to thread stack (user-allocated)
  const unsigned int  stack_size; // Stack size in bytes
  unsigned int        priority;   // Current thread priority
  os_tcb_t            tcb;        // (private) Thread control block.
  struct os_thread_s* next;       // (private) Pointer to next thread
} os_thread_t;

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
 * @param t Pointer to thread structure.
 * @param entry The entry function.
 * @param arg Data to pass to the entry function.
 * @return int 0 = success, anything else = failure.
 */
int os_thread_start(os_thread_t* t, void (*entry)(uint32_t), uint32_t arg);

int os_mutex_init(os_mutex_t* mutex);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
