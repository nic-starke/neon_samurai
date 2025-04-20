/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/neon_samurai               */
/*                         SPDX-License-Identifier: MIT                       */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include <avr/io.h>
#include <util/atomic.h>

#include "hal/dma.h"

#include "system/types.h"
#include "system/error.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */

void dma_peripheral_init(void) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		PR.PRGEN &= (u8)~PR_DMA_bm;			// Enable power to the DMA controller
		DMA.CTRL &= (u8)~DMA_ENABLE_bm; // Disable
		DMA.CTRL |= DMA_RESET_bm;				// Reset (all registers cleared)
		DMA.CTRL |= DMA_ENABLE_bm;			// Enable
	}
}

int dma_channel_init(DMA_CH_t* ch, struct dma_channel_cfg* cfg) {
	if (ch == NULL || cfg == NULL) {
		return ERR_NULL_PTR;
	}

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		ch->CTRLA &= (u8)~DMA_CH_ENABLE_bm; // Disable DMA channel
		ch->CTRLA |= DMA_CH_RESET_bm;				// Reset the channel (and registers)

		// Source configuration
		ch->SRCADDR0 = (u8)(cfg->src_ptr >> 0) & 0xFF;
		ch->SRCADDR1 = (u8)(cfg->src_ptr >> 8) & 0xFF;
		// ch->SRCADDR2 = (cfg->src_ptr >> 16) & 0xFF;

		ch->ADDRCTRL |= (u8)cfg->src_addr_mode;
		ch->ADDRCTRL |= (u8)cfg->src_reload_mode;

		// Destination configuration
		ch->DESTADDR0 = (u8)(cfg->dst_ptr >> 0) & 0xFF;
		ch->DESTADDR1 = (u8)(cfg->dst_ptr >> 8) & 0xFF;
		// ch->DESTADDR2 = (cfg->dst_ptr >> 16) & 0xFF;

		ch->ADDRCTRL |= (u8)cfg->dst_addr_mode;
		ch->ADDRCTRL |= (u8)cfg->dst_reload_mode;

		// Configure the transaction
		ch->TRIGSRC = cfg->trig_source;
		ch->CTRLA |= (u8)cfg->burst_len;
		ch->TRFCNT = (register16_t)cfg->block_size;

		if (cfg->repeat_count > 1) {
			ch->REPCNT = cfg->repeat_count;
			ch->CTRLA |= DMA_CH_REPEAT_bm;
		} else {
			ch->CTRLA |= DMA_CH_SINGLE_bm;
		}

		ch->CTRLB |= (u8)((cfg->err_prio << DMA_CH_ERRINTLVL_gp) |
											(cfg->int_prio << DMA_CH_TRNINTLVL_gp));

		/**
		 * Set the double buffer mode for the DMA controller.
		 * If enabled - this will "interlink" DMA channel 0 and 1, or channel 2
		 * and 3 After the primary channel is complete the secondary channel
		 * will fire a dma transaction, which then re-enables the primary
		 * channel...
		 */
		DMA.CTRL = (DMA.CTRL & (u8)~DMA_DBUFMODE_gm) | (u8)cfg->dbuf_mode;

		// Enable Channel
		ch->CTRLA |= DMA_CH_ENABLE_bm;
	} //   ATOMIC_BLOCK(ATOMIC_RESTORESTATE)

	return 0;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
