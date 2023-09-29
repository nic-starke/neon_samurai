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

#define OS_MAX_PRIORITY   (255)
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
  uint8_t        priority;   // Thread priority
} os_thread_t;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

static uint8_t stack_system[32];
static uint8_t stack_display[32];

static os_thread_t threads[THREAD_NB] = {
    [THREAD_SYS] =
        {
            .tcb        = {0},
            .stack      = &stack_system[0],
            .stack_size = sizeof(stack_system),
            .priority   = 0,
        },
    // [THREAD_INPUT] =
    //     {
    //         .tcb        = {0},
    //         .stack      = THREAD_INPUT_STACK,
    //         .stack_size = THREAD_INPUT_STACKSIZE,
    //     },
    // [THREAD_FILES] =
    //     {
    //         .tcb        = {0},
    //         .stack      = THREAD_FILES_STACK,
    //         .stack_size = THREAD_FILES_STACKSIZE,
    //     },
    [THREAD_DISPLAY] =
        {
            .tcb        = {0},
            .stack      = &stack_display[0],
            .stack_size = sizeof(stack_display),
            .priority   = 0,
        },
    // [THREAD_MIDI] =
    //     {
    //         .tcb        = {0},
    //         .stack      = THREAD_MIDI_STACK,
    //         .stack_size = THREAD_MIDI_STACKSIZE,
    //     },
    // [THREAD_NETWORK] =
    //     {
    //         .tcb        = {0},
    //         .stack      = THREAD_NETWORK_STACK,
    //         .stack_size = THREAD_NETWORK_STACKSIZE,
    //     },
    // [THREAD_USB] =
    //     {
    //         .tcb        = {0},
    //         .stack      = THREAD_USB_STACK,
    //         .stack_size = THREAD_USB_STACKSIZE,
    //     },
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
  os_thread_start(THREAD_SYS, system_thread, 0);
  atomOSStart();
}

int os_thread_start(thread_id id, void (*entry)(uint32_t), uint32_t arg) {
  if (entry == NULL || id >= THREAD_NB) {
    return ERR_BAD_PARAM;
  }

  os_thread_t* t = &threads[id];

  int status = atomThreadCreate(&t->tcb, t->priority, entry, arg, &t->stack[0],
                                t->stack_size, OS_STACK_CHECKING);

  return status;
}

int os_mutex_init(os_mutex_t* mutex) {
  assert(mutex);

  return atomMutexCreate(mutex);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
