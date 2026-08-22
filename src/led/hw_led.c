/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Documentation ~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * Drives the 256 encoder LEDs through a chain of 32 8-bit shift registers.
 *
 * Brightness is binary code modulated. led.c renders NUM_BCM_PLANES bit-planes
 * into gFRAME_BUFFER - plane p holds bit p of every channel's brightness - and
 * this module displays plane p for a slot weighted 2^p. An LED lit in the
 * planes matching value v is therefore on for v/255 of the cycle, giving 256
 * levels from 8 rendering frames.
 *
 * A slot cannot be shorter than the time to clock the next plane into the
 * shift registers, since that transfer runs during the current slot. That
 * shift time is what caps the achievable depth:
 *
 *   shift-out       = 32 bytes @ 16 MHz = 16 us
 *   slot unit       = 32 us  (2x margin over the shift)
 *   cycle           = 255 units = 8.16 ms  -> 122.5 Hz
 *
 * The two heaviest planes are split into 32-unit chunks and interleaved with
 * the light ones (see bcm_schedule below). This costs no RAM - the same plane
 * is simply clocked out more than once - but it keeps any single slot down to
 * ~1 ms, so the light is chopped at ~1.5 kHz rather than showing one 4 ms
 * pulse per cycle, which is what makes plain BCM prone to visible break-up
 * when the eye moves across the panel.
 */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>

#include "system/types.h"

#include "hal/gpio.h"
#include "hal/dma.h"
#include "hal/usart.h"

#include "led/led.h"

#include "system/hardware.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PORT_SR_LED					(PORTD)		// IO port for led shift registers
#define USART_LED						(USARTD0) // USART on D0
#define TIMER_LED						(TCD0)		// Timer on D0 - see ISR() below
#define DMA_CH_LED					(DMA.CH0) // DMA channel feeding the USART

#define SLOT_UNIT_TICKS			(4)		 // Timer ticks in one BCM weight unit (32 us)
#define NUM_BCM_SLOTS				(12)	 // Schedule entries, see bcm_schedule below
#define TIMER_TOP						(1019) // 255 units x SLOT_UNIT_TICKS, minus one

#define PIN_SR_LED_ENABLE_N (0)
#define PIN_SR_LED_CLOCK		(1)
#define PIN_SR_LED_DATA_OUT (3)
#define PIN_SR_LED_LATCH		(4)
#define PIN_SR_LED_RESET_N	(5)

// F_CPU/2 is the USART master-SPI ceiling. The BCM slot floor scales directly
// with this, so halving the shift time is what buys the eighth plane.
#define USART_BAUD					(16000000)

// Every defined interrupt flag of a type-0 timer - bits 2 and 3 are reserved
// and must be written as zero.
#define TC_INTFLAGS_ALL                                                        \
	(TC0_OVFIF_bm | TC0_ERRIF_bm | TC0_CCAIF_bm | TC0_CCBIF_bm | TC0_CCCIF_bm |  \
	 TC0_CCDIF_bm)

_Static_assert(NUM_BCM_PLANES == 8, "the BCM schedule is built for 8 planes");

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Variables ~~~~~~~~~~~~~~~~~~~~~~~~ */

// LED frame buffer - one bit-plane per row, written by led.c and animation.c,
// read by the DMA controller.
volatile u16 gFRAME_BUFFER[NUM_BCM_PLANES][NUM_ENCODERS];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

/**
 * The display schedule, as (plane, weight in units) pairs:
 *
 *   7:32  0:1  6:32  1:2  7:32  2:4  6:32  3:8  7:32  4:16  7:32  5:32
 *
 * Plane 7 appears four times and plane 6 twice, so every plane still gets its
 * binary share (128 and 64 units), but spread across the cycle. The tables
 * below are that schedule pre-resolved for the ISR:
 *
 *   bcm_slot_end - timer count at which each slot ends
 *   bcm_plane    - plane displayed during each slot
 */
static const u16 bcm_slot_end[NUM_BCM_SLOTS] PROGMEM = {
		127, 131, 259, 267, 395, 411, 539, 571, 699, 763, 891, 1019};

// One entry longer than the schedule: the extra element repeats slot 0, so the
// ISR can read the next slot's plane as bcm_plane[slot + 1] with no wrap test.
static const u8 bcm_plane[NUM_BCM_SLOTS + 1] PROGMEM = {7, 0, 6, 1, 7, 2, 6,
																												3, 7, 4, 7, 5, 7};

// Index of the slot currently being displayed.
static vu8 bcm_slot = 0;

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
			.baudrate	 = USART_BAUD,
			.endian		 = ENDIAN_LSB,
			.mode			 = SPI_MODE_CLK_LO_PHA_LO,
			.rx_enable = false,
	};

	// Configure DMA to transfer one bit-plane to the USARTs 1-byte tx buffer.
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

	// Reset shift registers. The outputs stay disabled until the first plane
	// has been latched - a reset leaves the registers all-zero, which in this
	// active-low wiring means every LED on.
	gpio_set(&PORT_SR_LED, PIN_SR_LED_ENABLE_N, 1);
	gpio_set(&PORT_SR_LED, PIN_SR_LED_RESET_N, 0);
	gpio_set(&PORT_SR_LED, PIN_SR_LED_RESET_N, 1);

	// Configure timer in single slope waveform mode. Compare channel A drives
	// pin 0 (the shift register output enable) and could PWM the global LED
	// brightness, but the output is left disabled (no TC0_CCAEN_bm) - led.c
	// does brightness in software, so the OE pin stays permanently asserted.
	TIMER_LED.CTRLA = TC_CLKSEL_OFF_gc; // Stop the timer while configuring
	TIMER_LED.CTRLB = TC_WGMODE_SINGLESLOPE_gc;
	TIMER_LED.PER		= TIMER_TOP;
	TIMER_LED.CNT		= 0;

	// Channel B -> end of the first BCM slot
	bcm_slot			= 0;
	TIMER_LED.CCB = pgm_read_word(&bcm_slot_end[0]);

	// The DMA is armed with the plane slot 0 displays; it is clocked out and
	// latched below, before the timer starts.
	dma_cfg.src_ptr = (uptr)&gFRAME_BUFFER[pgm_read_byte(&bcm_plane[0])][0];

	// Enable interrupts on compare match for channel B
	TIMER_LED.INTFLAGS = TC0_CCBIF_bm; // Discard any stale pending match
	TIMER_LED.INTCTRLB = (u8)((TIMER_LED.INTCTRLB & (u8)~TC0_CCBINTLVL_gm) |
														(u8)(PRIORITY_MED << TC0_CCBINTLVL_gp));

	dma_channel_init(&DMA_CH_LED, &dma_cfg);
	usart_module_init(&USART_LED, &usart_cfg);

	// Wait for that first plane to reach the shift registers (~16 us), latch
	// it, and only then enable the outputs. The guard bounds the spin so a
	// misconfigured DMA cannot wedge startup.
	for (u16 guard = 0; guard != UINT16_MAX; ++guard) {
		if ((DMA_CH_LED.CTRLA & DMA_CH_ENABLE_bm) == 0) {
			break;
		}
	}

	gpio_set(&PORT_SR_LED, PIN_SR_LED_LATCH, 1);
	gpio_set(&PORT_SR_LED, PIN_SR_LED_LATCH, 0);
	gpio_set(&PORT_SR_LED, PIN_SR_LED_ENABLE_N, 0);

	// Slot 0 must clock out the plane slot 1 will display.
	const uptr ptr			= (uptr)&gFRAME_BUFFER[pgm_read_byte(&bcm_plane[1])][0];
	DMA_CH_LED.SRCADDR0 = (u8)(ptr >> 0);
	DMA_CH_LED.SRCADDR1 = (u8)(ptr >> 8);
	DMA_CH_LED.CTRLA |= DMA_CH_ENABLE_bm;

	TIMER_LED.CTRLA = TC_CLKSEL_DIV256_gc; // Start the timer!
}

void hw_led_panic_red(void) {
	/*
		Runs before - or instead of - hw_led_init(), so it cannot rely on the
		USART, DMA or timer being configured, and it must work at whatever clock
		the CPU ended up on. Bit-bangs one frame straight into the shift
		registers and leaves it latched.
	*/
	const u16 red = (u16) ~(u16)(1u << RGB_RED_BIT);

	gpio_dir(&PORT_SR_LED, PIN_SR_LED_ENABLE_N, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_LED, PIN_SR_LED_CLOCK, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_LED, PIN_SR_LED_DATA_OUT, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_LED, PIN_SR_LED_LATCH, GPIO_OUTPUT);
	gpio_dir(&PORT_SR_LED, PIN_SR_LED_RESET_N, GPIO_OUTPUT);

	gpio_set(&PORT_SR_LED, PIN_SR_LED_ENABLE_N, 1);
	gpio_set(&PORT_SR_LED, PIN_SR_LED_RESET_N, 0);
	gpio_set(&PORT_SR_LED, PIN_SR_LED_RESET_N, 1);
	gpio_set(&PORT_SR_LED, PIN_SR_LED_LATCH, 0);

	// Same wire order the USART produces: bytes low-to-high, bits LSB first.
	for (u8 enc = 0; enc < NUM_ENCODERS; ++enc) {
		u16 word = red;

		for (u8 bit = 0; bit < 16u; ++bit) {
			gpio_set(&PORT_SR_LED, PIN_SR_LED_CLOCK, 0);
			gpio_set(&PORT_SR_LED, PIN_SR_LED_DATA_OUT, (u8)(word & 1u));
			gpio_set(&PORT_SR_LED, PIN_SR_LED_CLOCK, 1);
			word >>= 1;
		}
	}

	gpio_set(&PORT_SR_LED, PIN_SR_LED_LATCH, 1);
	gpio_set(&PORT_SR_LED, PIN_SR_LED_LATCH, 0);
	gpio_set(&PORT_SR_LED, PIN_SR_LED_ENABLE_N, 0);
}

// Must match TIMER_LED - the vector name cannot be derived from the macro.
ISR(TCD0_CCB_vect) {
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		// Latch the plane clocked out during the slot that just ended - it is
		// the one this new slot displays.
		gpio_set(&PORT_SR_LED, PIN_SR_LED_LATCH, 1);
		gpio_set(&PORT_SR_LED, PIN_SR_LED_LATCH, 0);

		u8 slot = bcm_slot + 1;
		if (slot >= NUM_BCM_SLOTS) {
			slot = 0;
		}
		bcm_slot = slot;

		TIMER_LED.CCB = pgm_read_word(&bcm_slot_end[slot]);

		// Clock out the plane the *following* slot will display.
		const u8	 plane = pgm_read_byte(&bcm_plane[slot + 1]);
		const uptr ptr	 = (uptr)&gFRAME_BUFFER[plane][0];

		DMA_CH_LED.SRCADDR0 = (u8)(ptr >> 0);
		DMA_CH_LED.SRCADDR1 = (u8)(ptr >> 8);
		DMA_CH_LED.CTRLA |= DMA_CH_ENABLE_bm;
	}
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
