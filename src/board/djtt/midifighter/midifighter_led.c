/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include "system/system.h"

#include "hal/avr/xmega/128a4u/gpio.h"
#include "hal/avr/xmega/128a4u/dma.h"
#include "hal/avr/xmega/128a4u/usart.h"
#include "hal/avr/xmega/128a4u/timer.h"

#include "board/djtt/midifighter.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define PORT_SR_LED (PORTD)   // IO port for led shift registers
#define USART_LED   (USARTD0) // USART on D0
#define TIMER_LED   (TCD0)    // Timer on D0

#define PIN_SR_LED_ENABLE_N (0)
#define PIN_SR_LED_CLOCK    (1)
#define PIN_SR_LED_DATA_OUT (3)
#define PIN_SR_LED_LATCH    (4)
#define PIN_SR_LED_RESET_N  (5)

#define USART_BAUD (8000000)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */

// LED frame buffer
volatile uint16_t mf_frame_buf[MF_NUM_PWM_FRAMES][MF_NUM_ENCODERS];

// Frame index (the current frame being transmitted)
volatile uint8_t mf_frame = 0;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void mf_led_init(void) {

  // Set all LEDS off
  memset(mf_frame_buf, 0xFFFF, sizeof(mf_frame_buf));

  // Configure GPIO for LED shift registers
  gpio_dir(&PORT_SR_LED, PIN_SR_LED_ENABLE_N, GPIO_OUTPUT);
  gpio_dir(&PORT_SR_LED, PIN_SR_LED_CLOCK, GPIO_OUTPUT);
  gpio_dir(&PORT_SR_LED, PIN_SR_LED_DATA_OUT, GPIO_OUTPUT);
  gpio_dir(&PORT_SR_LED, PIN_SR_LED_LATCH, GPIO_OUTPUT);
  gpio_dir(&PORT_SR_LED, PIN_SR_LED_RESET_N, GPIO_OUTPUT);

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
  // Once all data is transmitted the DMA_CH0_vect ISR fires.
  const dma_channel_cfg_t dma_cfg = {
      .repeat_count    = 1,
      .block_size      = MF_NUM_LED_SHIFT_REGISTERS,
      .burst_len       = DMA_CH_BURSTLEN_1BYTE_gc,
      .trig_source     = DMA_CH_TRIGSRC_USARTD0_DRE_gc, // empty usart buffer
      .dbuf_mode       = DMA_DBUFMODE_DISABLED_gc,
      .int_prio        = PRIORITY_OFF,
      .err_prio        = PRIORITY_OFF,
      .src_ptr         = &mf_frame_buf[0][0],
      .src_addr_mode   = DMA_CH_SRCDIR_INC_gc,
      .src_reload_mode = DMA_CH_SRCRELOAD_NONE_gc,
      .dst_ptr         = &USART_LED.DATA,
      .dst_addr_mode   = DMA_CH_DESTDIR_FIXED_gc,
      .dst_reload_mode = DMA_CH_DESTRELOAD_NONE_gc,
  };

  // Reset shift registers
  gpio_set(&PORT_SR_LED, PIN_SR_LED_ENABLE_N, 1);
  gpio_set(&PORT_SR_LED, PIN_SR_LED_RESET_N, 0);
  gpio_set(&PORT_SR_LED, PIN_SR_LED_RESET_N, 1);
  gpio_set(&PORT_SR_LED, PIN_SR_LED_ENABLE_N, 0);

  // Configure the timer to generate interrupts on channel A
  TCD0.CTRLB |= TC_WGMODE_SINGLESLOPE_gc;
  // Timer period (lower numbers == more interrupts == less flicker)
  TCD0.PER = 64;
  TCD0.CCA = 0;
  TCD0.INTCTRLB |= (PRIORITY_LOW << (TIMER_CHANNEL_A << 1));

  dma_channel_init(&DMA.CH0, &dma_cfg);
  usart_module_init(&USART_LED, &usart_cfg);
  TCD0.CTRLA |= TC_CLKSEL_DIV256_gc;
}

// Enable the DMA to transfer the frame buffer
void mf_led_transmit(void) {
  // DMA.CH0.CTRLA |= DMA_CH_ENABLE_bm;
}

// Software PWM...
ISR(TCD0_CCA_vect) {
  gpio_set(&PORT_SR_LED, PIN_SR_LED_LATCH, 1);
  gpio_set(&PORT_SR_LED, PIN_SR_LED_LATCH, 0);

  uintptr_t ptr = &mf_frame_buf[mf_frame][0];

  if (++mf_frame >= MF_NUM_PWM_FRAMES) {
    mf_frame = 0;
  }

  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    DMA.CH0.SRCADDR0 = (ptr >> 0) & 0xFF;
    DMA.CH0.SRCADDR1 = (ptr >> 8) & 0xFF;
    DMA.CH0.CTRLA |= DMA_CH_ENABLE_bm;
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
