#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2024) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai                */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "system/types.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
enum menu_action {
	MENU_ACTION_PARENT,
	MENU_ACTION_PREVIOUS,
	MENU_ACTION_NEXT,
	MENU_ACTION_EXIT,
	MENU_ACTION_SAVE,

	MENU_ACTION_NB
};

typedef void(transition_f)(void* ctx);
typedef void(display_f)(void* ctx);

struct menu_page {
	struct struct menu_page* parent;
	struct struct menu_page* child;
	display_f*					update_display;
	transition_f*				on_enter;
	transition_f*				on_next;
	transition_f*				on_prev;
};

struct menu {
	struct struct menu_page* entry;
	transition_f*				on_start;
	transition_f*				on_exit;
	transition_f*				on_save;
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

int menu_start(struct menu* menu);
int menu_stop(void);
int menu_update(struct menu_page* page);
int menu_process_action(enum menu_action action);
