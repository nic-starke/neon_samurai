/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "sys/types.h"

#include "hal/avr/xmega/128a4u/gpio.h"
#include "hal/avr/xmega/128a4u/dma.h"
#include "hal/avr/xmega/128a4u/usart.h"
#include "hal/avr/xmega/128a4u/timer.h"

#include "platform/midifighter/midifighter.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PORT_SR_LED					(PORTD)		// IO port for led shift registers
#define USART_LED						(USARTD0) // USART on D0
#define TIMER_LED						(TCD0)		// Timer on D0
#define TIMER_PERIOD				(255)
#define SOFT_PWM_PERIOD			(32)

#define PIN_SR_LED_ENABLE_N (0)
#define PIN_SR_LED_CLOCK		(1)
#define PIN_SR_LED_DATA_OUT (3)
#define PIN_SR_LED_LATCH		(4)
#define PIN_SR_LED_RESET_N	(5)

#define USART_BAUD					(8000000)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void mf_led_set_max_brightness(u8 brightness);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// LED frame buffer
volatile u16 gFRAME_BUFFER[MF_NUM_PWM_FRAMES][MF_NUM_gENCODERS];

// Frame index (the current frame being transmitted)
volatile u8 mf_frame = 0;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void hw_led_init(void) {

	// Set all LEDS off
	memset((u16*)gFRAME_BUFFER, (int)0xFFFF, (size_t)sizeof(gFRAME_BUFFER));
	// memset(gFRAME_BUFFER, 0x0000, sizeof(gFRAME_BUFFER));

	// Configure GPIO for LED shift registers
	gpio_dir(&PORT_SR_LED, PIN_SR_LED_ENABLE_N, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_LED, PIN_SR_LED_CLOCK, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_LED, PIN_SR_LED_DATA_OUT, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_LED, PIN_SR_LED_LATCH, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_LED, PIN_SR_LED_RESET_N, GPIO_OUTPUT);

	// Configure USART (SPI) for LED shift registers
	usart_config_s usart_cfg = {
			.baudrate = USART_BAUD,
			.endian		= ENDIAN_LSB,
			.mode			= SPI_MODE_CLK_LO_PHA_LO,
	};

	// Configure DMA to transfer display frames to the USARTs 1-byte tx buffer
	// The configuration will transmit 1 byte at a time, for a total of:
	// 32 bytes (block count) x 1 times (repeat count).
	// The trigger is set to USART data buffer being empty.
	dma_channel_cfg_s dma_cfg = {
			.repeat_count		 = 1,
			.block_size			 = MF_NUM_LED_SHIFT_REGISTERS,
			.burst_len			 = DMA_CH_BURSTLEN_1BYTE_gc,
			.trig_source		 = DMA_CH_TRIGSRC_USARTD0_DRE_gc, // empty usart buffer
			.dbuf_mode			 = DMA_DBUFMODE_DISABLED_gc,
			.int_prio				 = PRIORITY_OFF,
			.err_prio				 = PRIORITY_OFF,
			.src_ptr				 = (uptr)&gFRAME_BUFFER[0][0],
			.src_addr_mode	 = DMA_CH_SRCDIR_INC_gc,
			.src_reload_mode = DMA_CH_SRCRELOAD_NONE_gc,
			.dst_ptr				 = (uptr)&USART_LED.DATA,
			.dst_addr_mode	 = DMA_CH_DESTDIR_FIXED_gc,
			.dst_reload_mode = DMA_CH_DESTRELOAD_NONE_gc,
	};

	// Reset shift registers
	gpio_set(&PORT_SR_LED, PIN_SR_LED_ENABLE_N, 1);
	gpio_set(&PORT_SR_LED, PIN_SR_LED_RESET_N, 0);
	gpio_set(&PORT_SR_LED, PIN_SR_LED_RESET_N, 1);
	gpio_set(&PORT_SR_LED, PIN_SR_LED_ENABLE_N, 0);

	// Configure timer in single slope waveform mode
	TCD0.CTRLB |= TC_WGMODE_SINGLESLOPE_gc;
	TCD0.PER = TIMER_PERIOD; // Timer max tick count (timer resets at this value)
	TCD0.CCA = 0; // Channel A -> Global brightness (0 = max, 255 = min)
	TCD0.CCB = SOFT_PWM_PERIOD; // Channel B -> Software PWM tick (RGB colour
															// generation)

	// Enable timer compare channel A to generate PWM on pin 0 (shift register
	// output enable pin). The duty cycle of this PWM signal determines the
	// maximum brightness of ALL leds
	// TCD0.CTRLB |= TC0_CCAEN_bm;

	// Enable interrupts on compare match for channel B
	TCD0.INTCTRLB |= (PRIORITY_MED << (2)) & TC0_CCBINTLVL_gm;

	dma_channel_init(&DMA.CH0, &dma_cfg);
	usart_module_init(&USART_LED, &usart_cfg);
	TCD0.CTRLA |= TC_CLKSEL_DIV256_gc; // Start the timer!
}

void mf_led_set_max_brightness(u8 brightness) {
	if (brightness > MF_MAX_BRIGHTNESS) {
		brightness = MF_MAX_BRIGHTNESS;
	}

	// Post an event to the event queue
	// core_event_s evt = {
	// 		.id									 =
	// EVT_CORE_CORE_CORE_MAX_BRIGHTNESS, 		.data.max_brightness =
	// brightness,
	// };
	// event_post(&evt);
}

ISR(TCD0_CCB_vect) {
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		gpio_set(&PORT_SR_LED, PIN_SR_LED_LATCH, 1);
		gpio_set(&PORT_SR_LED, PIN_SR_LED_LATCH, 0);

		uptr ptr = (uptr)&gFRAME_BUFFER[mf_frame][0];

		if (++mf_frame >= MF_NUM_PWM_FRAMES) {
			mf_frame = 0;
		}
		// This ISR needs to trigger every 32 ticks, therefore the
		// compare value must be incremented by 32 ticks each time the interrupt
		// fires. The CCB value must wrap around at the TOP/PER value of the
		// timer.
		TCD0.CCB				 = (TCD0.CCB + SOFT_PWM_PERIOD) % TIMER_PERIOD;
		DMA.CH0.SRCADDR0 = (u8)(ptr >> 0) & 0xFF;
		DMA.CH0.SRCADDR1 = (u8)(ptr >> 8) & 0xFF;
		DMA.CH0.CTRLA |= DMA_CH_ENABLE_bm;
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
