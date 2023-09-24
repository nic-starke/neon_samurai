/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>
#include <util/atomic.h>

#include "system/system.h"
#include "hal/avr/xmega/128a4u/dma.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void dma_peripheral_init(void) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    PR.PRGEN &= ~PR_DMA_bm;      // Enable power to the DMA controller
    DMA.CTRL& ~ = DMA_ENABLE_bm; // Disable
    DMA.CTRL |= DMA_RESET_bm;    // Reset (all registers cleared)
    DMA.CTRL |= DMA_ENABLE_bm;   // Enable
  }
}

int dma_channel_init(DMA_CH_t* ch, dma_channel_cfg_t* cfg) {
  if (ch == NULL || cfg == NULL) {
    return ERR_NULL_PTR;
  }

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    ch->CTRLA &= ~DMA_CH_ENABLE_bm; // Disable DMA channel
    ch->CTRLA |= DMA_CH_RESET_bm;   // Reset the channel

    ch->SRCADDR0 = (cfg->src_ptr >> 0) & 0xFF;
    ch->SRCADDR1 = (cfg->src_ptr >> 8) & 0xFF;
    ch->SRCADDR2 = 0; //(cfg->src_ptr >> 16) & 0xFF;

    ch->ADDRCTRL |= cfg->src_addr_mode;
    ch->ADDRCTRL |= cfg->src_reload_mode;

    // Destination
    ch->DESTADDR0 = (cfg->dst_ptr >> 0) & 0xFF;
    ch->DESTADDR1 = (cfg->dst_ptr >> 8) & 0xFF;
    ch->DESTADDR2 = 0; //(cfg->dst_ptr >> 16) & 0xFF;

    ch->ADDRCTRL |= cfg->dst_addr_mode;
    ch->ADDRCTRL |= cfg->dst_reload_mode;

    // DMA Config
    ch->TRIGSRC = cfg->TriggerSource;

    ch->CTRLA |= cfg->BurstLength;

    ch->TRFCNT = cfg->BytesPerTransfer;

    if (cfg->Repeats > 1) {
      ch->REPCNT = cfg->Repeats;
      ch->CTRLA |= DMA_CH_REPEAT_bm;
    } else {
      ch->CTRLA |= DMA_CH_SINGLE_bm;
    }

    SET_REG(ch->CTRLB, (cfg->ErrInterruptPriority << DMA_CH_ERRINTLVL_gp) |
                           (cfg->InterruptPriority << DMA_CH_TRNINTLVL_gp));

    DMA_SetDoubleBufferMode(cfg->DoubleBufferMode);
  }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
