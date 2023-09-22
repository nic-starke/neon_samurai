/* Copyright (2021 - 2023) Nicolaus Starke
 * SPDX-License-Identifier: GPL-3.0-or-later
 * https://github.com/nic-starke/muffintwister */

/* =========================== Includes ============================ */

#include <stdbool.h>

#include "atom.h"
#include "atomport-private.h"

/* =========================== Defines ============================= */

/*
 * Idle thread stack size
 *
 * This needs to be large enough to handle any interrupt handlers
 * and callbacks called by interrupt handlers (e.g. user-created
 * timer callbacks) as well as the saving of all context when
 * switching away from this thread.
 *
 * In this case, the idle stack is allocated on the BSS via the
 * idle_thread_stack[] byte array.
 */

#define IDLE_STACK_SIZE 128
#define MAIN_STACK_SIZE 128
#define MAIN_PRIORITY   16

/* =========================== Extern ============================== */
/* =========================== Types =============================== */
/* =========================== Prototypes ========================== */

static void main_thread(uint32_t data);

/* =========================== Local Variables ===================== */

static uint8_t  main_stack[MAIN_STACK_SIZE];
static uint8_t  idle_stack[IDLE_STACK_SIZE];
static ATOM_TCB main_tcb;

/* =========================== Global Functions ==================== */

int main(void) {
  int8_t status = atomOSInit(&idle_stack[0], IDLE_STACK_SIZE, true);

  if (status == ATOM_OK) {
    avrInitSystemTickTimer();
    status = atomThreadCreate(&main_tcb, MAIN_PRIORITY, main_thread, 0,
                              &main_stack[0], MAIN_STACK_SIZE, TRUE);

    if (status == ATOM_OK) {
      atomOSStart();
    }
  }

  while (1)
    ;

  return (0);
}

/* =========================== Local Functions ===================== */

static void main_thread(uint32_t data) {
  return;
}
