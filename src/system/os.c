/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/sys.h"
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
#define IDLE_STACK_SIZE 128

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static uint8_t idle_stack[IDLE_STACK_SIZE];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int os_init(void) {
  int8_t status = atomOSInit(&idle_stack[0], IDLE_STACK_SIZE, true);

  if (status != ATOM_OK) {
    return status;
  }

  avrInitSystemTickTimer();

  return 0;
}

void os_start(void) {
  atomOSStart();
}

int os_thread_new(os_thread_t* const data, uint8_t priority, uint32_t arg,
                  void (*entry)(uint32_t)) {
  if (data == NULL || entry == NULL) {
    return ERR_BAD_PARAM;
  }

  if (priority > OS_MAX_PRIORITY) {
    return ERR_BAD_PARAM;
  }

  int status =
      atomThreadCreate(&data->tcb, priority, entry, arg, &data->stack[0],
                       data->stack_size, OS_STACK_CHECKING);

  return status;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~
 */
