/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "hal/avr/xmega/128a4u/gpio.h"
#include "hal/avr/xmega/128a4u/dma.h"
#include "hal/avr/xmega/128a4u/usart.h"

#include "board/djtt/midifighter.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PORT_SR_LED (PORTD) // IO port for led shift registers

#define PIN_SR_LED_ENABLE_N (0)
#define PIN_SR_LED_CLOCK    (1)
#define PIN_SR_LED_DATA_OUT (3)
#define PIN_SR_LED_LATCH    (4)
#define PIN_SR_LED_RESET_N  (5)

#define USART_BAUD (4000000)
#define USART_LED  (USARTD0)

#define FRAME_COUNT (32)
#define NUM_SR      (MF_NUM_LED_SHIFT_REGISTERS)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// LED frame buffer - intialise to logic high (LEDs are active low)
static uint16_t frame_buf[FRAME_COUNT][NUM_SR];

static uint8_t index = 0;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void mf_led_init(void) {

  // Set all LEDS off
  memset(frame_buf, 0xFF, sizeof(frame_buf));

  // TODO - Do i really need to use int16_t   here
  //     ? And how many frames can I get away with
  //     ?

  // Configure GPIO for LED shift registers
  gpio_dir(&PORT_SR_LED, PIN_SR_LED_ENABLE_N, GPIO_OUTPUT);
  gpio_set(&PORT_SR_LED, PIN_SR_LED_ENABLE_N, 1); // Disable shift registers
  gpio_dir(&PORT_SR_LED, PIN_SR_LED_CLOCK, GPIO_OUTPUT);
  gpio_dir(&PORT_SR_LED, PIN_SR_LED_DATA_OUT, GPIO_OUTPUT);
  gpio_dir(&PORT_SR_LED, PIN_SR_LED_LATCH, GPIO_OUTPUT);
  gpio_dir(&PORT_SR_LED, PIN_SR_LED_RESET_N, GPIO_OUTPUT);

  // Reset shift registers
  gpio_set(&PORT_SR_LED, PIN_SR_LED_RESET_N, 0);
  gpio_set(&PORT_SR_LED, PIN_SR_LED_RESET_N, 1);

  // Enable

  // Configure USART (SPI) for LED shift registers
  const usart_config_t usart_cfg = {
      .baudrate = USART_BAUD,
      .endian   = ENDIAN_LSB,
      .mode     = SPI_MODE_CLK_LO_PHA_LO,
  };

  // Configure DMA to transfer display frames to the USARTs 1-byte tx buffer
  // The configuration will transmit 1 byte at a time, for a total of:
  // 32 bytes (block count) x 32 times (repeat count).
  // The trigger is set to USART data buffer being empty.
  // Once all data is transmitted the transaction complete ISR fires.
  const dma_channel_cfg_t dma_cfg = {
      .repeat_count    = FRAME_COUNT,
      .block_size      = NUM_SR,
      .burst_len       = DMA_CH_BURSTLEN_1BYTE_gc,
      .trig_source     = DMA_CH_TRIGSRC_USARTD0_DRE_gc, // empty usart buffer
      .dbuf_mode       = DMA_DBUFMODE_DISABLED_gc,
      .int_prio        = PRIORITY_MED,
      .err_prio        = PRIORITY_OFF,
      .src_ptr         = frame_buf,
      .src_addr_mode   = DMA_CH_SRCDIR_INC_gc,
      .src_reload_mode = DMA_CH_SRCRELOAD_TRANSACTION_gc,
      .dst_ptr         = &USART_LED.DATA,
      .dst_addr_mode   = DMA_CH_DESTDIR_FIXED_gc,
      .dst_reload_mode = DMA_CH_DESTRELOAD_NONE_gc,
  };

  // Go!
  gpio_set(&PORT_SR_LED, PIN_SR_LED_ENABLE_N, 0); // Enable shift registers
  usart_module_init(&USART_LED, &usart_cfg);
  dma_channel_init(&DMA.CH0, &dma_cfg);
}

static bool enabled = 1;
static uint32_t count   = 0;

// ISR for DMA transaction completed (after blocks x repeats has been
// transferred)
ISR(DMA_CH0_vect) {
  // Toggle the SR latch
  gpio_set(&PORT_SR_LED, PIN_SR_LED_LATCH, 1);
  gpio_set(&PORT_SR_LED, PIN_SR_LED_LATCH, 0);

  // Re-set the repeat count
  DMA.CH0.REPCNT = FRAME_COUNT;

  // Clear the interrupt status for channel 0 (set bit:0 to 1)
  DMA.INTFLAGS |= 0x01;

  // Enable next transaction
  DMA.CH0.CTRLA |= DMA_CH_ENABLE_bm;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
