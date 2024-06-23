#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "sys/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
typedef enum {
	MENU_ACTION_PARENT,
	MENU_ACTION_PREVIOUS,
	MENU_ACTION_NEXT,
	MENU_ACTION_EXIT,
	MENU_ACTION_SAVE,

	MENU_ACTION_NB
} menu_action_e;

typedef void(transition_f)(void* ctx);
typedef void(display_f)(void* ctx);

typedef struct menu_page_s {
	struct menu_page_s* parent;
	struct menu_page_s* child;
	display_f*			update_display;
	transition_f*		on_enter;
	transition_f*		on_next;
	transition_f*		on_prev;
} menu_page_s;

typedef struct {
	struct menu_page_s* entry;
	transition_f*		on_start;
	transition_f*		on_exit;
	transition_f*		on_save;
} menu_s;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int menu_start(menu_s* menu);
int menu_stop(void);
int menu_update(menu_page_s* page);
int menu_process_action(menu_action_e action);
