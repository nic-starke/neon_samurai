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

// #include "midifighter/midifighter_display.h"
// #include "platform/midifighter/midifighter.h"
// #include "menu/menu.h"

// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// */
// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// */
// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// */
// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// */

// static display_f display_home;
// static display_f display_encoder;
// static display_f display_vmap;

// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~
// */
// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~
// */

// static menu_page_s menu_home;
// static menu_page_s menu_encoder;
// static menu_page_s menu_virtmap;

// static menu_page_s menu_home = {
// 	.parent = NULL,
// 	.child = &menu_encoder,
// 	.update_display = display_home,
// };

// static menu_page_s menu_encoder = {
// 	.parent = &menu_home,
// 	.child = &menu_virtmap,
// 	.update_display = display_encoder,
// };

// static menu_page_s menu_virtmap = {
// 	.parent = &menu_encoder,
// 	.child = NULL,
// 	.update_display = display_vmap,
// };

// static menu_s menu = {
// 	.entry = &menu_home,
// 	.on_save = NULL,
// 	.on_start = NULL,
// 	.on_exit = NULL,
// };

// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~
// */
// /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~
// */

// static void display_home (void* ctx) {
// 	assert(ctx);

// 	for (uint i = 0; i < MF_NUM_ENCODERS; i++) {
// 		mf_display_draw_encoder(i);
// 	}
// }

// static void display_encoder(void* ctx) {
// 	assert(ctx);

// 	input_dev_encoder_s* dev = (input_dev_encoder_s*)ctx;

// 	// last encoder is a preview of the new encoder configuration
// 	mf_display_draw_encoder_on_dest(dev->idx, MF_NUM_ENCODERS);

// 	// First row displays the virtmaps
// 	mf_display_draw_vmap(0, dev->vmap);
// 	mf_display_draw_vmap(1, dev->vmap->next);
// 	mf_display_draw_indicator(2, dev->vmap_mode);
// 	mf_display_draw_indicator(3, dev->vmap_display_mode);

// 	// Second row for encoder config
// 	quadrature_s* enc = (quadrature_s*) dev->ctx;
// 	mf_display_draw_indicator(4, enc->accel_mode);
// 	mf_display_draw_indicator(5, enc->detent);
// }
