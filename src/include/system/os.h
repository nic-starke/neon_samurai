/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "atom.h"
#include "atommutex.h"
#include "atomqueue.h"

#include "system/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef ATOM_MUTEX os_mutex_t;
typedef ATOM_TCB   os_tcb_t;
typedef ATOM_TIMER os_timer_t;
typedef ATOM_QUEUE os_queue_t;

struct os_thread_s;

typedef struct {
  u8* const      stack;      // Pointer to thread stack (user-allocated)
  const unsigned int  stack_size; // Stack size in bytes
  unsigned int        priority;   // Current thread priority
  os_tcb_t            tcb;        // (private) Thread control block.
  struct os_thread_s* next;       // (private) Pointer to next thread
} os_thread_t;

enum {
  OS_TIMEOUT_NOBLOCK = -1, // Call will return immediately if blocked
  OS_TIMEOUT_BLOCK   = 0,  // Call will block indefinitely until data ready
  OS_TIMEOUT_WAIT    = 1,  // Call will block until timeout expires
};

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
int os_thread_start(os_thread_t* t, void (*entry)(u32), u32 arg);

int os_mutex_init(os_mutex_t* mutex);
int os_timer_init(os_timer_t* timer);
u32 os_time_get(void);

// OS thread-safe message queue functions (wrappers around atomqueue)
int os_queue_create(os_queue_t* qptr, uint8_t* buff_ptr, uint32_t unit_size,
                    uint32_t max_num_msgs);
int os_queue_delete(os_queue_t* qptr);
int os_queue_get(os_queue_t* qptr, int32_t timeout, uint8_t* msgptr);
int os_queue_put(os_queue_t* qptr, int32_t timeout, uint8_t* msgptr);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
