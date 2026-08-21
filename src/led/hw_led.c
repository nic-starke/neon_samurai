/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * Drives the 256 encoder LEDs through a chain of 32 8-bit shift registers.
 *
 * Brightness is bit-code modulated in software: led.c renders NUM_PWM_FRAMES
 * frames into gFRAME_BUFFER, and this module shifts one frame out per timer
 * compare interrupt. TCD0 runs at F_CPU/256 (125 kHz, 8 us per tick) over a
 * 256-tick cycle, and channel B is stepped by SOFT_PWM_PERIOD ticks to give
 * eight evenly spaced interrupts per cycle:
 *
 *   frame interval  = 32 ticks       = 256 us   (3.9 kHz)
 *   full BCM cycle  = 32 frames      = 8.192 ms (122 Hz refresh)
 *   DMA burst       = 32 bytes @ 8M  = 32 us    (12.5% of a frame interval)
 *
 * The DMA burst therefore always completes well before the next interrupt
 * re-points the channel at the following frame.
 */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "system/types.h"

#include "hal/gpio.h"
#include "hal/dma.h"
#include "hal/usart.h"

#include "system/hardware.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PORT_SR_LED					(PORTD)		// IO port for led shift registers
#define USART_LED						(USARTD0) // USART on D0
#define TIMER_LED						(TCD0)		// Timer on D0 - see ISR() below
#define DMA_CH_LED					(DMA.CH0) // DMA channel feeding the USART

#define TIMER_TOP						(255)						// Timer PER (counter wraps here)
#define TIMER_TICKS					(TIMER_TOP + 1) // Counter states per cycle
#define SOFT_PWM_PERIOD			(32)						// Ticks between frames

#define PIN_SR_LED_ENABLE_N (0)
#define PIN_SR_LED_CLOCK		(1)
#define PIN_SR_LED_DATA_OUT (3)
#define PIN_SR_LED_LATCH		(4)
#define PIN_SR_LED_RESET_N	(5)

#define USART_BAUD					(8000000)

// The compare value is advanced with 8-bit wraparound, which only lands on
// evenly spaced slots if the counter has exactly 256 states.
_Static_assert(TIMER_TICKS == 256, "CCB wraparound assumes an 8-bit counter");
_Static_assert((TIMER_TICKS % SOFT_PWM_PERIOD) == 0,
							 "soft PWM period must divide the timer period evenly");

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */

// LED frame buffer - written by led.c, read by the DMA controller.
volatile u16 gFRAME_BUFFER[NUM_PWM_FRAMES][NUM_ENCODERS];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// Frame index (the current frame being transmitted)
static vu8 mf_frame = 0;

// Timer compare value of the next frame interrupt. Biased so that the slot
// sequence is 31, 63, ... 255 - it never reaches 0, which would place the
// compare match on the counter reload.
static vu8 pwm_slot = SOFT_PWM_PERIOD - 1;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void hw_led_init(void) {

	// Set all LEDs off. The frame buffer is written in inverted/active-low
	// form (see mf_draw_encoder()'s "Write to Frame Buffer (Inverted)" step
	// in led.c: it stores ~final_state, i.e. bit=1 means the LED is OFF) -
	// so the correct all-off fill is 0xFF per byte, not 0x00. memset() fills
	// byte-by-byte regardless of the int argument's width, so 0xFF is passed
	// directly rather than via a 16-bit-looking 0xFFFF that could misread as
	// "the wrong endian half was taken".
	memset((u16*)gFRAME_BUFFER, 0xFF, (size_t)sizeof(gFRAME_BUFFER));

	// Configure GPIO for LED shift registers
	gpio_dir(&PORT_SR_LED, PIN_SR_LED_ENABLE_N, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_LED, PIN_SR_LED_CLOCK, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_LED, PIN_SR_LED_DATA_OUT, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_LED, PIN_SR_LED_LATCH, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_LED, PIN_SR_LED_RESET_N, GPIO_OUTPUT);

	// Configure USART (SPI) for LED shift registers
	struct usart_config usart_cfg = {
			.baudrate = USART_BAUD,
			.endian		= ENDIAN_LSB,
			.mode			= SPI_MODE_CLK_LO_PHA_LO,
	};

	// Configure DMA to transfer display frames to the USARTs 1-byte tx buffer
	// The configuration will transmit 1 byte at a time, for a total of:
	// 32 bytes (block count) x 1 times (repeat count).
	// The trigger is set to USART data buffer being empty.
	struct dma_channel_cfg dma_cfg = {
			.repeat_count		 = 1,
			.block_size			 = NUM_LED_SHIFT_REGISTERS,
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

	// Configure timer in single slope waveform mode. Compare channel A drives
	// pin 0 (the shift register output enable) and could PWM the global LED
	// brightness, but the output is left disabled (no TC0_CCAEN_bm) - led.c
	// does brightness in software, so the OE pin stays permanently asserted.
	TIMER_LED.CTRLA = TC_CLKSEL_OFF_gc; // Stop the timer while configuring
	TIMER_LED.CTRLB = TC_WGMODE_SINGLESLOPE_gc;
	TIMER_LED.PER		= TIMER_TOP;
	TIMER_LED.CNT		= 0;

	// Channel B -> Software PWM tick (RGB colour generation)
	TIMER_LED.CCB = pwm_slot;

	// Enable interrupts on compare match for channel B
	TIMER_LED.INTFLAGS = TC0_CCBIF_bm; // Discard any stale pending match
	TIMER_LED.INTCTRLB = (u8)((TIMER_LED.INTCTRLB & (u8)~TC0_CCBINTLVL_gm) |
														(u8)(PRIORITY_MED << TC0_CCBINTLVL_gp));

	dma_channel_init(&DMA_CH_LED, &dma_cfg);
	usart_module_init(&USART_LED, &usart_cfg);
	TIMER_LED.CTRLA = TC_CLKSEL_DIV256_gc; // Start the timer!
}

// Must match TIMER_LED - the vector name cannot be derived from the macro.
ISR(TCD0_CCB_vect) {
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		gpio_set(&PORT_SR_LED, PIN_SR_LED_LATCH, 1);
		gpio_set(&PORT_SR_LED, PIN_SR_LED_LATCH, 0);

		uptr ptr = (uptr)&gFRAME_BUFFER[mf_frame][0];

		if (++mf_frame >= NUM_PWM_FRAMES) {
			mf_frame = 0;
		}

		// This ISR needs to trigger every SOFT_PWM_PERIOD ticks, so the compare
		// value is advanced by that much each time. The counter has 256 states,
		// so the 8-bit wraparound lands exactly on the next slot - taking the
		// modulo of TIMER_TOP instead would stretch one interval in every eight
		// to 33 ticks and drift the phase by a tick per cycle.
		pwm_slot += SOFT_PWM_PERIOD;
		TIMER_LED.CCB = pwm_slot;

		DMA_CH_LED.SRCADDR0 = (u8)(ptr >> 0);
		DMA_CH_LED.SRCADDR1 = (u8)(ptr >> 8);
		DMA_CH_LED.CTRLA |= DMA_CH_ENABLE_bm;
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
