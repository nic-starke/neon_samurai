/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
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

static u8 sys_stack[STACK_SIZE_SYS];

static os_thread_t sys_thread = {
    .next       = NULL,
    .priority   = 16,
    .stack      = sys_stack,
    .stack_size = sizeof(sys_stack),
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int os_init(void) {
  static u8 idle_stack[STACK_SIZE_IDLE];
  i8        status = atomOSInit(&idle_stack[0], STACK_SIZE_IDLE, true);

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

int os_thread_start(os_thread_t* t, void (*entry)(u32), u32 arg) {
  assert(t);
  assert(entry);

  return atomThreadCreate(&t->tcb, t->priority, entry, arg, &t->stack[0],
                          t->stack_size, OS_STACK_CHECKING);
}

int os_mutex_init(os_mutex_t* mutex) {
  assert(mutex);
  return atomMutexCreate(mutex);
}

int os_timer_init(os_timer_t* timer) {
  assert(timer);
  return atomTimerRegister(timer);
}

u32 os_time_get(void) {
  return atomTimeGet();
}

int os_queue_create(os_queue_t* qptr, uint8_t* buff_ptr, uint32_t unit_size,
                    uint32_t max_num_msgs) {
  assert(qptr);
  assert(buff_ptr);
  assert(unit_size);
  assert(max_num_msgs);

  int ret = atomQueueCreate(qptr, buff_ptr, unit_size, max_num_msgs);

  if (ret == ATOM_OK) {
    return 0;
  } else {
    return ret;
  }
}

int os_queue_delete(os_queue_t* qptr) {
  return atomQueueDelete(qptr);
}

int os_queue_get(os_queue_t* qptr, int32_t timeout, uint8_t* msgptr) {
  return atomQueueGet(qptr, timeout, msgptr);
}

int os_queue_put(os_queue_t* qptr, int32_t timeout, uint8_t* msgptr) {
  return atomQueuePut(qptr, timeout, msgptr);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
