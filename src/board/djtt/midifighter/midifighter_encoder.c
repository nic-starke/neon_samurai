/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "core/core_error.h"
#include "core/core_rgb.h"
#include "core/core_types.h"
#include "core/core_utility.h"
#include "hal/avr/xmega/128a4u/gpio.h"
#include "io/io_device.h"
#include "io/encoder/encoder_quadrature.h"
#include "io/switch/switch.h"
#include "lfo/lfo.h"
#include "virtmap/virtmap.h"

#include "board/djtt/midifighter.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PORT_SR_ENC				 (PORTC) // IO port for encoder IO shift registers

#define PIN_SR_ENC_LATCH	 (0) // 74HC595N
#define PIN_SR_ENC_CLOCK	 (1)
#define PIN_SR_ENC_DATA_IN (2)

#define NUM_ENC_DEVS			 (MF_NUM_ENC_BANKS * MF_NUM_ENCODERS)
#define NUM_VIRTMAPS                                                           \
	(MF_NUM_ENC_BANKS * MF_NUM_ENCODERS * MF_NUM_VIRTMAPS_PER_ENC)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// PROGMEM static const midifighter_encoder_s default_config = {
// 		.enabled					= true,
// 		.detent						= false,
// 		.encoder_ctx			= {0},
// 		.hwenc_id					= 0,
// 		.led_style				= LED_STYLE_SINGLE,
// 		.led_detent.value = 0,
// 		.led_rgb.value		= 0,
// 		.midi.channel			= 0,
// 		.midi.mode				= MIDI_MODE_CC,
// 		.midi.data.cc			= 0,
// };

static iodev_s	 enc_dev[MF_NUM_ENC_BANKS][MF_NUM_ENCODERS];
static encoder_s enc_ctx[MF_NUM_ENC_BANKS][MF_NUM_ENCODERS];
static virtmap_s enc_vmap[MF_NUM_ENC_BANKS][MF_NUM_ENCODERS]
												 [MF_NUM_VIRTMAPS_PER_ENC];

static lfo_s enc_lfo[3];

// A single switch context is required for all 16 switches (in the encoders)
static switch_x16_ctx_s switch_ctx;

static uint active_bank = 0;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

int mf_encoder_init(void) {
	int ret = 0;

	// Configure GPIO for encoder IO shift regsiters
	gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_LATCH, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN, GPIO_INPUT);

	// Latch initial encoder data
	gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 0);
	gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 1);

	// Initialise encoder devices and virtual parameter mappings
	midi_cc_e cc = MIDI_CC_MIN;
	for (uint b = 0; b < MF_NUM_ENC_BANKS; b++) {
		for (uint e = 0; e < MF_NUM_ENCODERS; e++) {
			ret = iodev_init(DEV_TYPE_ENCODER, &enc_dev[b][e], (void*)&enc_ctx[b][e],
											 e);
			RETURN_ON_ERR(ret);

#warning "This virtmap assignment applies a simple default CC mapping"
			for (uint v = 0; v < MF_NUM_VIRTMAPS_PER_ENC; v++) {
				virtmap_s* map = &enc_vmap[b][e][v];

				const u16 inc				= ENC_RANGE / MF_NUM_VIRTMAPS_PER_ENC;
				map->position.start = inc * v;
				map->position.stop	= inc * (v + 1);
				map->range.lower		= MIDI_CC_MIN;
				map->range.upper		= MIDI_CC_MAX;
				map->next						= NULL;

				map->proto.type					= PROTOCOL_MIDI;
				map->proto.midi.channel = 0;
				map->proto.midi.mode		= MIDI_MODE_CC;
				map->proto.midi.data.cc = cc++;

				virtmap_assign(map, &enc_dev[b][e]);
				RETURN_ON_ERR(ret);
			}
		}
	}

	for (uint i = 0; i < COUNTOF(enc_lfo); i++) {
		enc_lfo[i].amplitude	= ENC_MAX;
		enc_lfo[i].frequency	= 5 * i;
		enc_lfo[i].phase			= 0;
		enc_lfo[i].sampleRate = 1000;
		enc_lfo[i].waveform		= WAVEFORM_SINE;
	}

	// ret = iodev_register_arr(DEV_TYPE_ENCODER, &enc_dev);
	// RETURN_ON_ERR(ret);

	return 0;
}

// Scan the hardware state of the midifighter and update local contexts
void mf_encoder_update(void) {
	// Latch the IO levels into the shift registers
	gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 1);

	// Clock the 16 data bits for the encoder switches
	u16 swstates = 0;
	for (size_t i = 0; i < MF_NUM_ENCODER_SWITCHES; i++) {
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);
		u8 state = !(bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
		swstates |= (state << i);
	}

	// Execute the debounce and update routine for the switches
	switch_x16_update(&switch_ctx, swstates);
	switch_x16_debounce(&switch_ctx);

	// Clock the 32 bits for the 2x16 quadrature encoder signals, and update
	// encoder state.
	for (int i = 0; i < MF_NUM_ENCODERS; ++i) {
		iodev_s* dev = &enc_dev[active_bank][i];

		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
		u8 ch_a = (bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 0);
		u8 ch_b = (bool)gpio_get(&PORT_SR_ENC, PIN_SR_ENC_DATA_IN);
		gpio_set(&PORT_SR_ENC, PIN_SR_ENC_CLOCK, 1);

		encoder_update(dev, ch_a, ch_b);
	}

	// Close the door!
	gpio_set(&PORT_SR_ENC, PIN_SR_ENC_LATCH, 0);
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
