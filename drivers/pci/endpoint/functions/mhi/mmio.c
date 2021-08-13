#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/platform_device.h>

#include "internal.h"

int pci_epf_mhi_mmio_read(struct pci_epf_mhi *epf_mhi, u32 offset,
			u32 *reg_value)
{
	void __iomem *addr;

	addr = epf_mhi->mmio + offset;

	*reg_value = readl_relaxed(addr);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_read);

int pci_epf_mhi_mmio_write(struct pci_epf_mhi *epf_mhi, u32 offset,
				u32 val)
{
	void __iomem *addr;

	addr = epf_mhi->mmio + offset;

	writel_relaxed(val, addr);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_write);

int pci_epf_mhi_mmio_masked_write(struct pci_epf_mhi *epf_mhi, u32 offset,
						u32 mask, u32 shift,
						u32 val)
{
	u32 reg_val;

	pci_epf_mhi_mmio_read(epf_mhi, offset, &reg_val);

	reg_val &= ~mask;
	reg_val |= ((val << shift) & mask);

	pci_epf_mhi_mmio_write(epf_mhi, offset, reg_val);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_masked_write);

int pci_epf_mhi_mmio_masked_read(struct pci_epf_mhi *epf_mhi, u32 offset,
						u32 mask, u32 shift,
						u32 *reg_val)
{
	pci_epf_mhi_mmio_read(epf_mhi, offset, reg_val);

	*reg_val &= mask;
	*reg_val >>= shift;

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_masked_read);

int pci_epf_mhi_mmio_get_mhi_state(struct pci_epf_mhi *epf_mhi, enum pci_epf_mhi_state *state,
						u32 *mhi_reset)
{
	u32 reg_value = 0;

	pci_epf_mhi_mmio_masked_read(epf_mhi, MHICTRL,
		MHISTATUS_MHISTATE_MASK, MHISTATUS_MHISTATE_SHIFT, state);

	pci_epf_mhi_mmio_read(epf_mhi, MHICTRL, &reg_value);

	if (reg_value & MHICTRL_RESET_MASK)
		*mhi_reset = 1;
	else
		*mhi_reset = 0;

	pr_info("MHICTRL is 0x%x, reset:%d\n",
			reg_value, *mhi_reset);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_get_mhi_state);

static int pci_epf_mhi_mmio_mask_set_chdb_int_a7(struct pci_epf_mhi *mhi,
						u32 chdb_id, bool enable)
{
	u32 chid_mask, chid_idx, chid_shft, val = 0;

	chid_shft = chdb_id%32;
	chid_mask = (1 << chid_shft);
	chid_idx = chdb_id/32;

	if (chid_idx >= MHI_MASK_ROWS_CH_EV_DB) {
		pr_err("Invalid channel id:%d\n", chid_idx);
		return -EINVAL;
	}

	if (enable)
		val = 1;

	pci_epf_mhi_mmio_masked_write(mhi, MHI_CHDB_INT_MASK_A7_n(chid_idx),
					chid_mask, chid_shft, val);

	pci_epf_mhi_mmio_read(mhi, MHI_CHDB_INT_MASK_A7_n(chid_idx),
						&mhi->chdb[chid_idx].mask);

	return 0;
}

int pci_epf_mhi_mmio_enable_chdb_a7(struct pci_epf_mhi *mhi, u32 chdb_id)
{
	pci_epf_mhi_mmio_mask_set_chdb_int_a7(mhi, chdb_id, true);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_enable_chdb_a7);

int pci_epf_mhi_mmio_disable_chdb_a7(struct pci_epf_mhi *mhi, u32 chdb_id)
{
	pci_epf_mhi_mmio_mask_set_chdb_int_a7(mhi, chdb_id, false);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_disable_chdb_a7);

static int pci_epf_mhi_mmio_set_erdb_int_a7(struct pci_epf_mhi *mhi,
					u32 erdb_ch_id, bool enable)
{
	u32 erdb_id_shft, erdb_id_mask, erdb_id_idx, val = 0;

	erdb_id_shft = erdb_ch_id%32;
	erdb_id_mask = (1 << erdb_id_shft);
	erdb_id_idx = erdb_ch_id/32;

	if (enable)
		val = 1;

	pci_epf_mhi_mmio_masked_write(mhi,
			MHI_ERDB_INT_MASK_A7_n(erdb_id_idx),
			erdb_id_mask, erdb_id_shft, val);

	return 0;
}

int pci_epf_mhi_mmio_enable_erdb_a7(struct pci_epf_mhi *mhi, u32 erdb_id)
{
	pci_epf_mhi_mmio_set_erdb_int_a7(mhi, erdb_id, true);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_enable_erdb_a7);

int pci_epf_mhi_mmio_disable_erdb_a7(struct pci_epf_mhi *mhi, u32 erdb_id)
{
	pci_epf_mhi_mmio_set_erdb_int_a7(mhi, erdb_id, false);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_disable_erdb_a7);

static int pci_epf_mhi_mmio_set_chdb_interrupts(struct pci_epf_mhi *mhi, bool enable)
{
	u32 mask = 0, i = 0;

	if (enable)
		mask = MHI_CHDB_INT_MASK_A7_n_MASK_MASK;

	for (i = 0; i < MHI_MASK_ROWS_CH_EV_DB; i++) {
		pci_epf_mhi_mmio_write(mhi,
				MHI_CHDB_INT_MASK_A7_n(i), mask);
		mhi->chdb[i].mask = mask;
	}

	return 0;
}

int pci_epf_mhi_mmio_enable_chdb_interrupts(struct pci_epf_mhi *mhi)
{
	pci_epf_mhi_mmio_set_chdb_interrupts(mhi, true);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_enable_chdb_interrupts);

int pci_epf_mhi_mmio_mask_chdb_interrupts(struct pci_epf_mhi *mhi)
{
	pci_epf_mhi_mmio_set_chdb_interrupts(mhi, false);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_mask_chdb_interrupts);

int pci_epf_mhi_mmio_read_chdb_status_interrupts(struct pci_epf_mhi *mhi)
{
	u32 i;

	for (i = 0; i < MHI_MASK_ROWS_CH_EV_DB; i++)
		pci_epf_mhi_mmio_read(mhi,
			MHI_CHDB_INT_STATUS_A7_n(i), &mhi->chdb[i].status);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_read_chdb_status_interrupts);

static int pci_epf_mhi_mmio_set_erdb_interrupts(struct pci_epf_mhi *mhi, bool enable)
{
	u32 mask = 0, i;

	if (enable)
		mask = MHI_ERDB_INT_MASK_A7_n_MASK_MASK;

	for (i = 0; i < MHI_MASK_ROWS_CH_EV_DB; i++)
		pci_epf_mhi_mmio_write(mhi,
				MHI_ERDB_INT_MASK_A7_n(i), mask);

	return 0;
}

int pci_epf_mhi_mmio_enable_erdb_interrupts(struct pci_epf_mhi *mhi)
{
	pci_epf_mhi_mmio_set_erdb_interrupts(mhi, true);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_enable_erdb_interrupts);

int pci_epf_mhi_mmio_mask_erdb_interrupts(struct pci_epf_mhi *mhi)
{
	pci_epf_mhi_mmio_set_erdb_interrupts(mhi, false);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_mask_erdb_interrupts);

int pci_epf_mhi_mmio_read_erdb_status_interrupts(struct pci_epf_mhi *mhi)
{
	u32 i;

	for (i = 0; i < MHI_MASK_ROWS_CH_EV_DB; i++)
		pci_epf_mhi_mmio_read(mhi, MHI_ERDB_INT_STATUS_A7_n(i),
						&mhi->evdb[i].status);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_read_erdb_status_interrupts);

int pci_epf_mhi_mmio_enable_ctrl_interrupt(struct pci_epf_mhi *mhi)
{
	pci_epf_mhi_mmio_masked_write(mhi, MHI_CTRL_INT_MASK_A7,
			MHI_CTRL_MHICTRL_MASK, MHI_CTRL_MHICTRL_SHFT, 1);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_enable_ctrl_interrupt);

int pci_epf_mhi_mmio_disable_ctrl_interrupt(struct pci_epf_mhi *mhi)
{
	pci_epf_mhi_mmio_masked_write(mhi, MHI_CTRL_INT_MASK_A7,
			MHI_CTRL_MHICTRL_MASK, MHI_CTRL_MHICTRL_SHFT, 0);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_disable_ctrl_interrupt);

int pci_epf_mhi_mmio_read_ctrl_status_interrupt(struct pci_epf_mhi *mhi)
{
	pci_epf_mhi_mmio_read(mhi, MHI_CTRL_INT_STATUS_A7, &mhi->ctrl_int);

	mhi->ctrl_int &= 0x1;

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_read_ctrl_status_interrupt);

int pci_epf_mhi_mmio_read_cmdb_status_interrupt(struct pci_epf_mhi *mhi)
{
	pci_epf_mhi_mmio_read(mhi, MHI_CTRL_INT_STATUS_A7, &mhi->cmd_int);

	mhi->cmd_int &= 0x10;

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_read_cmdb_status_interrupt);

int pci_epf_mhi_mmio_enable_cmdb_interrupt(struct pci_epf_mhi *mhi)
{
	pci_epf_mhi_mmio_masked_write(mhi, MHI_CTRL_INT_MASK_A7,
			MHI_CTRL_CRDB_MASK, MHI_CTRL_CRDB_SHFT, 1);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_enable_cmdb_interrupt);

int pci_epf_mhi_mmio_disable_cmdb_interrupt(struct pci_epf_mhi *mhi)
{
	pci_epf_mhi_mmio_masked_write(mhi, MHI_CTRL_INT_MASK_A7,
			MHI_CTRL_CRDB_MASK, MHI_CTRL_CRDB_SHFT, 0);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_disable_cmdb_interrupt);

void pci_epf_mhi_mmio_mask_interrupts(struct pci_epf_mhi *mhi)
{
	pci_epf_mhi_mmio_disable_ctrl_interrupt(mhi);

	pci_epf_mhi_mmio_disable_cmdb_interrupt(mhi);

	pci_epf_mhi_mmio_mask_chdb_interrupts(mhi);

	pci_epf_mhi_mmio_mask_erdb_interrupts(mhi);
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_mask_interrupts);

int pci_epf_mhi_mmio_clear_interrupts(struct pci_epf_mhi *epf_mhi)
{
	u32 i = 0;

	for (i = 0; i < MHI_MASK_ROWS_CH_EV_DB; i++)
		pci_epf_mhi_mmio_write(epf_mhi, MHI_CHDB_INT_CLEAR_A7_n(i),
				MHI_CHDB_INT_CLEAR_A7_n_CLEAR_MASK);

	for (i = 0; i < MHI_MASK_ROWS_CH_EV_DB; i++)
		pci_epf_mhi_mmio_write(epf_mhi, MHI_ERDB_INT_CLEAR_A7_n(i),
				MHI_ERDB_INT_CLEAR_A7_n_CLEAR_MASK);

	pci_epf_mhi_mmio_write(epf_mhi, MHI_CTRL_INT_CLEAR_A7,
		(MHI_CTRL_INT_MMIO_WR_CLEAR | MHI_CTRL_INT_CRDB_CLEAR |
		MHI_CTRL_INT_CRDB_MHICTRL_CLEAR));

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_clear_interrupts);

int pci_epf_mhi_mmio_get_chc_base(struct pci_epf_mhi *epf_mhi)
{
	u32 ccabap_value = 0, offset = 0;

	pci_epf_mhi_mmio_read(epf_mhi, CCABAP_HIGHER, &ccabap_value);

	epf_mhi->ch_ctx_shadow.host_pa = ccabap_value;
	epf_mhi->ch_ctx_shadow.host_pa <<= 32;

	pci_epf_mhi_mmio_read(epf_mhi, CCABAP_LOWER, &ccabap_value);

	epf_mhi->ch_ctx_shadow.host_pa |= ccabap_value;

	offset = (u32)(epf_mhi->ch_ctx_shadow.host_pa -
					epf_mhi->ctrl_base.host_pa);

	epf_mhi->ch_ctx_shadow.device_pa = epf_mhi->ctrl_base.device_pa + offset;
	epf_mhi->ch_ctx_shadow.device_va = epf_mhi->ctrl_base.device_va + offset;

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_get_chc_base);

int pci_epf_mhi_mmio_get_erc_base(struct pci_epf_mhi *epf_mhi)
{
	u32 ecabap_value = 0, offset = 0;

	pci_epf_mhi_mmio_read(epf_mhi, ECABAP_HIGHER, &ecabap_value);

	epf_mhi->ev_ctx_shadow.host_pa = ecabap_value;
	epf_mhi->ev_ctx_shadow.host_pa <<= 32;

	pci_epf_mhi_mmio_read(epf_mhi, ECABAP_LOWER, &ecabap_value);

	epf_mhi->ev_ctx_shadow.host_pa |= ecabap_value;

	offset = (u32)(epf_mhi->ev_ctx_shadow.host_pa -
					epf_mhi->ctrl_base.host_pa);

	epf_mhi->ev_ctx_shadow.device_pa = epf_mhi->ctrl_base.device_pa + offset;
	epf_mhi->ev_ctx_shadow.device_va = epf_mhi->ctrl_base.device_va + offset;

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_get_erc_base);

int pci_epf_mhi_mmio_get_crc_base(struct pci_epf_mhi *epf_mhi)
{
	u32 crcbap_value = 0, offset = 0;

	pci_epf_mhi_mmio_read(epf_mhi, CRCBAP_HIGHER, &crcbap_value);

	epf_mhi->cmd_ctx_shadow.host_pa = crcbap_value;
	epf_mhi->cmd_ctx_shadow.host_pa <<= 32;

	pci_epf_mhi_mmio_read(epf_mhi, CRCBAP_LOWER, &crcbap_value);

	epf_mhi->cmd_ctx_shadow.host_pa |= crcbap_value;

	offset = (u32)(epf_mhi->cmd_ctx_shadow.host_pa -
					epf_mhi->ctrl_base.host_pa);

	epf_mhi->cmd_ctx_shadow.device_pa = epf_mhi->ctrl_base.device_pa + offset;
	epf_mhi->cmd_ctx_shadow.device_va = epf_mhi->ctrl_base.device_va + offset;

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_get_crc_base);

int pci_epf_mhi_mmio_set_env(struct pci_epf_mhi *epf_mhi, u32 value)
{
	pci_epf_mhi_mmio_write(epf_mhi, BHI_EXECENV, value);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_set_env);

int pci_epf_mhi_mmio_clear_reset(struct pci_epf_mhi *epf_mhi)
{
	pci_epf_mhi_mmio_masked_write(epf_mhi, MHICTRL,
		MHICTRL_RESET_MASK, MHICTRL_RESET_SHIFT, 0);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_clear_reset);

int pci_epf_mhi_mmio_reset(struct pci_epf_mhi *epf_mhi)
{
	pci_epf_mhi_mmio_write(epf_mhi, MHICTRL, 0);
	pci_epf_mhi_mmio_write(epf_mhi, MHISTATUS, 0);
	pci_epf_mhi_mmio_clear_interrupts(epf_mhi);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_reset);

int pci_epf_mhi_mmio_get_cfg(struct pci_epf_mhi *epf_mhi)
{
	int ret = 0;

	pci_epf_mhi_mmio_read(epf_mhi, MHIREGLEN, &epf_mhi->cfg.mhi_reg_len);

	pci_epf_mhi_mmio_masked_read(epf_mhi, MHICFG, MHICFG_NER_MASK,
				MHICFG_NER_SHIFT, &epf_mhi->cfg.event_rings);

	ret = pci_epf_mhi_mmio_masked_read(epf_mhi, MHICFG, MHICFG_NHWER_MASK,
				MHICFG_NHWER_SHIFT, &epf_mhi->cfg.hw_event_rings);
	if (ret)
		return ret;

	ret = pci_epf_mhi_mmio_read(epf_mhi, CHDBOFF, &epf_mhi->cfg.chdb_offset);
	if (ret)
		return ret;

	pci_epf_mhi_mmio_read(epf_mhi, ERDBOFF, &epf_mhi->cfg.erdb_offset);

	epf_mhi->cfg.channels = NUM_CHANNELS;

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_get_cfg);

int pci_epf_mhi_restore_mmio(struct pci_epf_mhi *epf_mhi)
{
	u32 i, reg_cntl_value;
	void *reg_cntl_addr;

	pci_epf_mhi_mmio_mask_interrupts(epf_mhi);

	for (i = 0; i < (PCI_EPF_MHI_MMIO_RANGE/4); i++) {
		reg_cntl_addr = epf_mhi->mmio_base_addr +
				PCI_EPF_MHI_MMIO_OFFSET + (i * 4);
		reg_cntl_value = epf_mhi->mmio_backup[i];
		writel_relaxed(reg_cntl_value, reg_cntl_addr);
	}

	pci_epf_mhi_mmio_clear_interrupts(epf_mhi);

	/* Mask and enable control interrupt */
	pci_epf_mhi_mmio_enable_ctrl_interrupt(epf_mhi);

	/*Enable chdb interrupt*/
	pci_epf_mhi_mmio_enable_chdb_interrupts(epf_mhi);

	/*Enable cmdb interrupt*/
	pci_epf_mhi_mmio_enable_cmdb_interrupt(epf_mhi);

	mb();

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_restore_mmio);

int pci_epf_mhi_backup_mmio(struct pci_epf_mhi *epf_mhi)
{
	u32 i = 0;
	void __iomem *reg_cntl_addr;

	for (i = 0; i < PCI_EPF_MHI_MMIO_RANGE/4; i++) {
		reg_cntl_addr = (void __iomem *) (epf_mhi->mmio_base_addr +
				PCI_EPF_MHI_MMIO_OFFSET + (i * 4));
		epf_mhi->mmio_backup[i] = readl_relaxed(reg_cntl_addr);
	}

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_backup_mmio);

int pci_epf_mhi_get_mhi_addr(struct pci_epf_mhi *epf_mhi)
{
	u32 data_value = 0;

	pci_epf_mhi_mmio_read(epf_mhi, MHICTRLBASE_LOWER, &data_value);
	epf_mhi->host_addr.ctrl_base_lsb = data_value;

	pci_epf_mhi_mmio_read(epf_mhi, MHICTRLBASE_HIGHER, &data_value);
	epf_mhi->host_addr.ctrl_base_msb = data_value;

	pci_epf_mhi_mmio_read(epf_mhi, MHICTRLLIMIT_LOWER, &data_value);
	epf_mhi->host_addr.ctrl_limit_lsb = data_value;

	pci_epf_mhi_mmio_read(epf_mhi, MHICTRLLIMIT_HIGHER, &data_value);
	epf_mhi->host_addr.ctrl_limit_msb = data_value;

	pci_epf_mhi_mmio_read(epf_mhi, MHIDATABASE_LOWER, &data_value);
	epf_mhi->host_addr.data_base_lsb = data_value;

	pci_epf_mhi_mmio_read(epf_mhi, MHIDATABASE_HIGHER, &data_value);
	epf_mhi->host_addr.data_base_msb = data_value;

	pci_epf_mhi_mmio_read(epf_mhi, MHIDATALIMIT_LOWER, &data_value);
	epf_mhi->host_addr.data_limit_lsb = data_value;

	pci_epf_mhi_mmio_read(epf_mhi, MHIDATALIMIT_HIGHER, &data_value);
	epf_mhi->host_addr.data_limit_msb = data_value;

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_get_mhi_addr);

int pci_epf_mhi_mmio_init(struct pci_epf_mhi *epf_mhi)
{
	int rc = 0;

	pci_epf_mhi_mmio_read(epf_mhi, MHIREGLEN, &epf_mhi->cfg.mhi_reg_len);

	pci_epf_mhi_mmio_masked_read(epf_mhi, MHICFG, MHICFG_NER_MASK,
				MHICFG_NER_SHIFT, &epf_mhi->cfg.event_rings);

	rc = pci_epf_mhi_mmio_masked_read(epf_mhi, MHICFG, MHICFG_NHWER_MASK,
				MHICFG_NHWER_SHIFT, &epf_mhi->cfg.hw_event_rings);
	if (rc)
		return rc;

	rc = pci_epf_mhi_mmio_read(epf_mhi, CHDBOFF, &epf_mhi->cfg.chdb_offset);
	if (rc)
		return rc;

	pci_epf_mhi_mmio_read(epf_mhi, ERDBOFF, &epf_mhi->cfg.erdb_offset);

	epf_mhi->cfg.channels = NUM_CHANNELS;

	if (!epf_mhi->mmio_initialized)
		pci_epf_mhi_mmio_reset(epf_mhi);

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_mmio_init);

int pci_epf_mhi_update_ner(struct pci_epf_mhi *epf_mhi)
{
	int rc = 0, mhi_cfg = 0;

	rc = pci_epf_mhi_mmio_read(epf_mhi, MHICFG, &mhi_cfg);
	if (rc)
		return rc;

	pr_debug("MHICFG: 0x%x", mhi_cfg);

	epf_mhi->cfg.event_rings =
		(mhi_cfg & MHICFG_NER_MASK) >> MHICFG_NER_SHIFT;
	epf_mhi->cfg.hw_event_rings =
		(mhi_cfg & MHICFG_NHWER_MASK) >> MHICFG_NHWER_SHIFT;

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_update_ner);
