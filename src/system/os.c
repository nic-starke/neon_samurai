/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/system.h"
#include "system/os.h"
#include "atom.h"
#include "atomport-private.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*
 * Idle thread stack size
 *
 * This needs to be large enough to handle any interrupt handlers and callbacks
 * called by interrupt handlers (e.g. user-created timer callbacks) as well as
 * the saving of all context when switching away from this thread.
 *
 * In this case, the idle stack is allocated on the BSS via the
 * idle_thread_stack[] byte array.
 */
#define STACK_SIZE_IDLE (128)
#define STACK_SIZE_SYS  (128)

#define OS_MAX_PRIORITY   (255)
#define OS_STACK_CHECKING (FALSE)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static uint8_t sys_stack[STACK_SIZE_SYS];

static os_thread_t sys_thread = {
    .next       = NULL,
    .priority   = 16,
    .stack      = sys_stack,
    .stack_size = sizeof(sys_stack),
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int os_init(void) {
  static uint8_t idle_stack[STACK_SIZE_IDLE];
  int8_t         status = atomOSInit(&idle_stack[0], STACK_SIZE_IDLE, true);

  if (status != ATOM_OK) {
    return status;
  }

  avrInitSystemTickTimer();

  return 0;
}

void os_start(void) {
  os_thread_start(&sys_thread, system_thread, 0);
  atomOSStart();
}

int os_thread_start(os_thread_t* t, void (*entry)(uint32_t), uint32_t arg) {
  assert(t);
  assert(entry);

  return atomThreadCreate(&t->tcb, t->priority, entry, arg, &t->stack[0],
                          t->stack_size, OS_STACK_CHECKING);
}

int os_mutex_init(os_mutex_t* mutex) {
  assert(mutex);
  return atomMutexCreate(mutex);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
