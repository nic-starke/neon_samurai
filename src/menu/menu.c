// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// */
// /*                  Copyright (c) (2021 - 2024) Nicolaus Starke */
// /*                  https://github.com/nic-starke/neon_samurai */
// /*                         SPDX-License-Identifier: MIT */
// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// */
// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~
// */
// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// */

// #include "menu/menu.h"
// #include "core/core_error.h"
// #include "event/event.h"
// #include "event/events_core.h"

// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// */
// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// */
// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// */
// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// */
// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~
// */
// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~
// */

// static menu_s*			current_menu;
// static menu_page_s* previous_page;
// static menu_page_s* current_page;

// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~
// */

// int menu_start(menu_s* menu) {
// 	assert(menu);
// 	current_menu = menu;

// 	if (current_menu->on_start) {
// 		current_menu->on_start();
// 	}

// 	return menu_update(menu->entry);
// }

// int menu_stop(void) {
// 	RETURN_ERR_IF_NULL(current_menu);

// 	previous_page = current_page;
// 	current_page	= NULL;

// 	if (current_menu->on_exit) {
// 		current_menu->on_exit();
// 	}

// 	current_menu = NULL;

// 	return 0;
// }

// int menu_update(menu_page_s* page) {
// 	assert(page);

// 	previous_page = current_page;
// 	current_page	= page;
// 	current_page->on_enter();
// 	current_page->update_display();

// 	return 0;
// }

// int menu_process_action(menu_action_e action) {
// 	RETURN_ERR_IF_NULL(current_page);

// 	switch (action) {
// 		case MENU_ACTION_PARENT: {
// 			if (current_page->parent != NULL) {
// 				return menu_update(current_page->parent);
// 			}
// 			break;
// 		}

// 		case MENU_ACTION_PREVIOUS: {
// 			if (current_page->on_prev != NULL) {
// 				current_page->on_prev();
// 				return menu_update(current_page);
// 			}
// 			break;
// 		}

// 		case MENU_ACTION_NEXT: {
// 			if (current_page->on_next != NULL) {
// 				current_page->on_next();
// 				return menu_update(current_page);
// 			}
// 			break;
// 		}

// 		case MENU_ACTION_EXIT: {
// 			return menu_stop();
// 		}

// 		case MENU_ACTION_SAVE: {
// 			event_core_s evt;
// 			evt.type = EVT_CORE_REQ_CFG_SAVE;
// 			int ret = event_post_rt(EVENT_CHANNEL_CORE, &evt);
// 			RETURN_ON_ERR(ret);

// 			if (current_menu->on_save != NULL) {
// 				current_menu->on_save();
// 			}
// 		}

// 		default: return ERR_BAD_PARAM;
// 	}

// 	return 0;
// }

// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~
// */
