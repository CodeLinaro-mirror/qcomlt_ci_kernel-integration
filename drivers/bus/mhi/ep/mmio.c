// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Linaro Ltd.
 * Author: Manivannan Sadhasivam <manivannan.sadhasivam@linaro.org>
 */

#include <linux/bitfield.h>
#include <linux/io.h>
#include <linux/mhi_ep.h>

#include "internal.h"

u32 mhi_ep_mmio_read(struct mhi_ep_cntrl *mhi_cntrl, u32 offset)
{
	return readl(mhi_cntrl->mmio + offset);
}

void mhi_ep_mmio_write(struct mhi_ep_cntrl *mhi_cntrl, u32 offset, u32 val)
{
	writel(val, mhi_cntrl->mmio + offset);
}

void mhi_ep_mmio_masked_write(struct mhi_ep_cntrl *mhi_cntrl, u32 offset, u32 mask, u32 val)
{
	u32 regval;

	regval = mhi_ep_mmio_read(mhi_cntrl, offset);
	regval &= ~mask;
	regval |= ((val << __ffs(mask)) & mask);
	mhi_ep_mmio_write(mhi_cntrl, offset, regval);
}

u32 mhi_ep_mmio_masked_read(struct mhi_ep_cntrl *dev, u32 offset, u32 mask)
{
	u32 regval;

	regval = mhi_ep_mmio_read(dev, offset);
	regval &= mask;
	regval >>= __ffs(mask);

	return regval;
}

void mhi_ep_mmio_get_mhi_state(struct mhi_ep_cntrl *mhi_cntrl, enum mhi_state *state,
				bool *mhi_reset)
{
	u32 regval;

	regval = mhi_ep_mmio_read(mhi_cntrl, MHICTRL);
	*state = FIELD_GET(MHICTRL_MHISTATE_MASK, regval);
	*mhi_reset = !!FIELD_GET(MHICTRL_RESET_MASK, regval);
}

static void mhi_ep_mmio_mask_set_chdb_int_a7(struct mhi_ep_cntrl *mhi_cntrl,
						u32 chdb_id, bool enable)
{
	u32 chid_mask, chid_idx, chid_shift, val = 0;

	chid_shift = chdb_id % 32;
	chid_mask = BIT(chid_shift);
	chid_idx = chdb_id / 32;

	WARN_ON(chid_idx >= MHI_MASK_ROWS_CH_EV_DB);

	if (enable)
		val = 1;

	mhi_ep_mmio_masked_write(mhi_cntrl, MHI_CHDB_INT_MASK_A7_n(chid_idx),
				  chid_mask, val);

	/* Update the local copy of the channel mask */
	mhi_cntrl->chdb[chid_idx].mask &= ~chid_mask;
	mhi_cntrl->chdb[chid_idx].mask |= val << chid_shift;
}

void mhi_ep_mmio_enable_chdb_a7(struct mhi_ep_cntrl *mhi_cntrl, u32 chdb_id)
{
	mhi_ep_mmio_mask_set_chdb_int_a7(mhi_cntrl, chdb_id, true);
}

void mhi_ep_mmio_disable_chdb_a7(struct mhi_ep_cntrl *mhi_cntrl, u32 chdb_id)
{
	mhi_ep_mmio_mask_set_chdb_int_a7(mhi_cntrl, chdb_id, false);
}

static void mhi_ep_mmio_set_chdb_interrupts(struct mhi_ep_cntrl *mhi_cntrl, bool enable)
{
	u32 val = 0, i;

	if (enable)
		val = MHI_CHDB_INT_MASK_A7_n_EN_ALL;

	for (i = 0; i < MHI_MASK_ROWS_CH_EV_DB; i++) {
		mhi_ep_mmio_write(mhi_cntrl, MHI_CHDB_INT_MASK_A7_n(i), val);
		mhi_cntrl->chdb[i].mask = val;
	}
}

void mhi_ep_mmio_enable_chdb_interrupts(struct mhi_ep_cntrl *mhi_cntrl)
{
	mhi_ep_mmio_set_chdb_interrupts(mhi_cntrl, true);
}

static void mhi_ep_mmio_mask_chdb_interrupts(struct mhi_ep_cntrl *mhi_cntrl)
{
	mhi_ep_mmio_set_chdb_interrupts(mhi_cntrl, false);
}

void mhi_ep_mmio_read_chdb_status_interrupts(struct mhi_ep_cntrl *mhi_cntrl)
{
	u32 i;

	for (i = 0; i < MHI_MASK_ROWS_CH_EV_DB; i++)
		mhi_cntrl->chdb[i].status = mhi_ep_mmio_read(mhi_cntrl,
							     MHI_CHDB_INT_STATUS_A7_n(i));
}

static void mhi_ep_mmio_set_erdb_interrupts(struct mhi_ep_cntrl *mhi_cntrl, bool enable)
{
	u32 val = 0, i;

	if (enable)
		val = MHI_ERDB_INT_MASK_A7_n_EN_ALL;

	for (i = 0; i < MHI_MASK_ROWS_CH_EV_DB; i++)
		mhi_ep_mmio_write(mhi_cntrl, MHI_ERDB_INT_MASK_A7_n(i), val);
}

static void mhi_ep_mmio_mask_erdb_interrupts(struct mhi_ep_cntrl *mhi_cntrl)
{
	mhi_ep_mmio_set_erdb_interrupts(mhi_cntrl, false);
}

void mhi_ep_mmio_enable_ctrl_interrupt(struct mhi_ep_cntrl *mhi_cntrl)
{
	mhi_ep_mmio_masked_write(mhi_cntrl, MHI_CTRL_INT_MASK_A7,
				  MHI_CTRL_MHICTRL_MASK, 1);
}

void mhi_ep_mmio_disable_ctrl_interrupt(struct mhi_ep_cntrl *mhi_cntrl)
{
	mhi_ep_mmio_masked_write(mhi_cntrl, MHI_CTRL_INT_MASK_A7,
				  MHI_CTRL_MHICTRL_MASK, 0);
}

void mhi_ep_mmio_enable_cmdb_interrupt(struct mhi_ep_cntrl *mhi_cntrl)
{
	mhi_ep_mmio_masked_write(mhi_cntrl, MHI_CTRL_INT_MASK_A7,
				  MHI_CTRL_CRDB_MASK, 1);
}

void mhi_ep_mmio_disable_cmdb_interrupt(struct mhi_ep_cntrl *mhi_cntrl)
{
	mhi_ep_mmio_masked_write(mhi_cntrl, MHI_CTRL_INT_MASK_A7,
				  MHI_CTRL_CRDB_MASK, 0);
}

void mhi_ep_mmio_mask_interrupts(struct mhi_ep_cntrl *mhi_cntrl)
{
	mhi_ep_mmio_disable_ctrl_interrupt(mhi_cntrl);
	mhi_ep_mmio_disable_cmdb_interrupt(mhi_cntrl);
	mhi_ep_mmio_mask_chdb_interrupts(mhi_cntrl);
	mhi_ep_mmio_mask_erdb_interrupts(mhi_cntrl);
}

static void mhi_ep_mmio_clear_interrupts(struct mhi_ep_cntrl *mhi_cntrl)
{
	u32 i = 0;

	for (i = 0; i < MHI_MASK_ROWS_CH_EV_DB; i++)
		mhi_ep_mmio_write(mhi_cntrl, MHI_CHDB_INT_CLEAR_A7_n(i),
				   MHI_CHDB_INT_CLEAR_A7_n_CLEAR_ALL);

	for (i = 0; i < MHI_MASK_ROWS_CH_EV_DB; i++)
		mhi_ep_mmio_write(mhi_cntrl, MHI_ERDB_INT_CLEAR_A7_n(i),
				   MHI_ERDB_INT_CLEAR_A7_n_CLEAR_ALL);

	mhi_ep_mmio_write(mhi_cntrl, MHI_CTRL_INT_CLEAR_A7,
			   MHI_CTRL_INT_MMIO_WR_CLEAR |
			   MHI_CTRL_INT_CRDB_CLEAR |
			   MHI_CTRL_INT_CRDB_MHICTRL_CLEAR);
}

void mhi_ep_mmio_get_chc_base(struct mhi_ep_cntrl *mhi_cntrl)
{
	u32 ccabap_value;

	ccabap_value = mhi_ep_mmio_read(mhi_cntrl, CCABAP_HIGHER);
	mhi_cntrl->ch_ctx_host_pa = ccabap_value;
	mhi_cntrl->ch_ctx_host_pa <<= 32;

	ccabap_value = mhi_ep_mmio_read(mhi_cntrl, CCABAP_LOWER);
	mhi_cntrl->ch_ctx_host_pa |= ccabap_value;
}

void mhi_ep_mmio_get_erc_base(struct mhi_ep_cntrl *mhi_cntrl)
{
	u32 ecabap_value;

	ecabap_value = mhi_ep_mmio_read(mhi_cntrl, ECABAP_HIGHER);
	mhi_cntrl->ev_ctx_host_pa = ecabap_value;
	mhi_cntrl->ev_ctx_host_pa <<= 32;

	ecabap_value = mhi_ep_mmio_read(mhi_cntrl, ECABAP_LOWER);
	mhi_cntrl->ev_ctx_host_pa |= ecabap_value;
}

void mhi_ep_mmio_get_crc_base(struct mhi_ep_cntrl *mhi_cntrl)
{
	u32 crcbap_value;

	crcbap_value = mhi_ep_mmio_read(mhi_cntrl, CRCBAP_HIGHER);
	mhi_cntrl->cmd_ctx_host_pa = crcbap_value;
	mhi_cntrl->cmd_ctx_host_pa <<= 32;

	crcbap_value = mhi_ep_mmio_read(mhi_cntrl, CRCBAP_LOWER);
	mhi_cntrl->cmd_ctx_host_pa |= crcbap_value;
}

u64 mhi_ep_mmio_get_db(struct mhi_ep_ring *ring)
{
	struct mhi_ep_cntrl *mhi_cntrl = ring->mhi_cntrl;
	u64 db_offset;
	u32 regval;

	regval = mhi_ep_mmio_read(mhi_cntrl, ring->db_offset_h);
	db_offset = regval;
	db_offset <<= 32;

	regval = mhi_ep_mmio_read(mhi_cntrl, ring->db_offset_l);
	db_offset |= regval;

	return db_offset;
}

void mhi_ep_mmio_set_env(struct mhi_ep_cntrl *mhi_cntrl, u32 value)
{
	mhi_ep_mmio_write(mhi_cntrl, BHI_EXECENV, value);
}

void mhi_ep_mmio_clear_reset(struct mhi_ep_cntrl *mhi_cntrl)
{
	mhi_ep_mmio_masked_write(mhi_cntrl, MHICTRL, MHICTRL_RESET_MASK, 0);
}

void mhi_ep_mmio_reset(struct mhi_ep_cntrl *mhi_cntrl)
{
	mhi_ep_mmio_write(mhi_cntrl, MHICTRL, 0);
	mhi_ep_mmio_write(mhi_cntrl, MHISTATUS, 0);
	mhi_ep_mmio_clear_interrupts(mhi_cntrl);
}

void mhi_ep_mmio_init(struct mhi_ep_cntrl *mhi_cntrl)
{
	int mhi_cfg;

	mhi_cntrl->chdb_offset = mhi_ep_mmio_read(mhi_cntrl, CHDBOFF);
	mhi_cntrl->erdb_offset = mhi_ep_mmio_read(mhi_cntrl, ERDBOFF);

	mhi_cfg = mhi_ep_mmio_read(mhi_cntrl, MHICFG);
	mhi_cntrl->event_rings = FIELD_GET(MHICFG_NER_MASK, mhi_cfg);
	mhi_cntrl->hw_event_rings = FIELD_GET(MHICFG_NHWER_MASK, mhi_cfg);

	mhi_ep_mmio_reset(mhi_cntrl);
}

void mhi_ep_mmio_update_ner(struct mhi_ep_cntrl *mhi_cntrl)
{
	int mhi_cfg;

	mhi_cfg = mhi_ep_mmio_read(mhi_cntrl, MHICFG);
	mhi_cntrl->event_rings = FIELD_GET(MHICFG_NER_MASK, mhi_cfg);
	mhi_cntrl->hw_event_rings = FIELD_GET(MHICFG_NHWER_MASK, mhi_cfg);
}
