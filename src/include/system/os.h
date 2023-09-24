/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "types.h"
#include "atom.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define OS_MAX_PRIORITY   (16)
#define OS_STACK_CHECKING (FALSE)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * @brief Data structure required for each thread.
 */
typedef struct {
  ATOM_TCB       tcb;        // Thread control block for atomthreads rtos.
  uint8_t* const stack;      // Pointer to thread stack (user-allocated)
  const uint16_t stack_size; // Stack size in bytes
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
 * @brief Initialise and start execution of a new thread.
 *
 * @param data Thread data structure.
 * @param priority Thread priority.
 * @param arg Argument to pass to thread entry function.
 * @param entry Thread entry function.
 * @return int 0 on success, otherwise sys err code.
 */
int os_thread_new(os_thread_t* const data, uint8_t priority, uint32_t arg,
                  void (*entry)(uint32_t));

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
