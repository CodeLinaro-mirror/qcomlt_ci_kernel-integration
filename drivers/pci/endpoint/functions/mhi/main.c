#include <linux/delay.h>
#include <linux/dmaengine.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/pci_ids.h>
#include <linux/random.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include <linux/pci_regs.h>

#include "internal.h"

/* Wait time on the device for Host to set M0 state */
#define PCI_EPF_MHI_M0_MAX_CNT		30
/* Wait time before suspend/resume is complete */
#define MHI_SUSPEND_MIN			100
#define MHI_SUSPEND_TIMEOUT		600
/* Wait time on the device for Host to set BHI_INTVEC */
#define MHI_BHI_INTVEC_MAX_CNT			200
#define MHI_BHI_INTVEC_WAIT_MS		50
#define MHI_WAKEUP_TIMEOUT_CNT		20
#define MHI_MASK_CH_EV_LEN		32
#define MHI_RING_CMD_ID			0
#define MHI_RING_PRIMARY_EVT_ID		1
#define MHI_1K_SIZE			0x1000
/* Updated Specification for event start is NER - 2 and end - NER -1 */
#define MHI_HW_ACC_EVT_RING_END		1

#define MHI_HOST_REGION_NUM             2

#define MHI_MMIO_CTRL_INT_STATUS_A7_MSK	0x1
#define MHI_MMIO_CTRL_CRDB_STATUS_MSK	0x2

#define HOST_ADDR(lsb, msb)		((lsb) | ((u64)(msb) << 32))
#define HOST_ADDR_LSB(addr)		(addr & 0xFFFFFFFF)
#define HOST_ADDR_MSB(addr)		((addr >> 32) & 0xFFFFFFFF)

#define MHI_IPC_LOG_PAGES		(100)
#define MHI_REGLEN			0x100
#define MHI_INIT			0
#define MHI_REINIT			1

#define TR_RING_ELEMENT_SZ	sizeof(struct pci_epf_mhi_transfer_ring_element)
#define RING_ELEMENT_TYPE_SZ	sizeof(union pci_epf_mhi_ring_element_type)

#define PCI_EPF_MHI_CH_CLOSE_TIMEOUT_MIN	5000
#define PCI_EPF_MHI_CH_CLOSE_TIMEOUT_MAX	5100
#define PCI_EPF_MHI_CH_CLOSE_TIMEOUT_COUNT	30

static struct pci_epf_header mhi_header = {
	.vendorid		= 0x17cb,
	.deviceid		= 0x0306,
	.revid			= 0x0,
	.progif_code		= 0x0,
	.subclass_code		= 0x0,
	.baseclass_code		= 0xff,
	.cache_line_size	= 0x10,
	.subsys_vendor_id	= 0x0,
	.subsys_id		= 0x0,
};

static int pci_epf_mhi_cache_host_cfg(struct pci_epf_mhi *epf_mhi)
{
	struct pci_epf *epf = epf_mhi->epf;
	struct pci_epc *epc = epf->epc;
	struct device *dev = &epf->dev;
	struct device *pdev = epc->dev.parent;
	struct pci_epf_mhi_addr data_transfer;
	int ret = 0;
	u64 addr1 = 0;

	/* Get host memory region configuration */
	pci_epf_mhi_get_mhi_addr(epf_mhi);

	epf_mhi->ctrl_base.host_pa  = HOST_ADDR(epf_mhi->host_addr.ctrl_base_lsb,
						epf_mhi->host_addr.ctrl_base_msb);
	epf_mhi->data_base.host_pa  = HOST_ADDR(epf_mhi->host_addr.data_base_lsb,
						epf_mhi->host_addr.data_base_msb);

	addr1 = HOST_ADDR(epf_mhi->host_addr.ctrl_limit_lsb,
					epf_mhi->host_addr.ctrl_limit_msb);
	epf_mhi->ctrl_base.size = addr1 - epf_mhi->ctrl_base.host_pa;
	addr1 = HOST_ADDR(epf_mhi->host_addr.data_limit_lsb,
					epf_mhi->host_addr.data_limit_msb);
	epf_mhi->data_base.size = addr1 - epf_mhi->data_base.host_pa;

	/* Get the channel context base pointer from host */
	ret = pci_epf_mhi_mmio_get_chc_base(epf_mhi);
	if (ret) {
		dev_err(dev, "Fetching channel context failed\n");
		return ret;
	}

	epf_mhi->ch_ctx_cache = pci_epc_mem_alloc_addr(epc, &epf_mhi->ch_ctx_cache_dma_handle,
							sizeof(struct pci_epf_mhi_ch_ctx));
	if (!epf_mhi->ch_ctx_cache) {
		dev_err(dev, "Failed to allocate ch_ctx_cache address\n");
		return -ENOMEM;
	}

	ret = pci_epc_map_addr(epc, epf->func_no, epf_mhi->ch_ctx_cache_dma_handle,
			       epf_mhi->ch_ctx_shadow.host_pa, sizeof(struct pci_epf_mhi_ch_ctx));
	if (ret) {
		dev_err(dev, "Failed to map ch_ctx_cache address\n");
		goto err_ch_ctx;
	}

	/* Get the event context base pointer from host */
	ret = pci_epf_mhi_mmio_get_erc_base(epf_mhi);
	if (ret) {
		dev_err(dev, "Fetching event ring context failed\n");
		goto err_ch_ctx_map;
	}

	epf_mhi->ev_ctx_cache = pci_epc_mem_alloc_addr(epc, &epf_mhi->ev_ctx_cache_dma_handle,
							sizeof(struct pci_epf_mhi_ev_ctx));
	if (!epf_mhi->ev_ctx_cache) {
		dev_err(dev, "Failed to allocate ev_ctx_cache address\n");
		ret = -ENOMEM;
		goto err_ch_ctx_map;
	}

	ret = pci_epc_map_addr(epc, epf->func_no, epf_mhi->ev_ctx_cache_dma_handle,
			       epf_mhi->ev_ctx_shadow.host_pa, sizeof(struct pci_epf_mhi_ev_ctx));
	if (ret) {
		dev_err(dev, "Failed to map ev_ctx_cache address\n");
		goto err_ev_ctx;
	}

	/* Get the command context base pointer from host */
	ret = pci_epf_mhi_mmio_get_crc_base(epf_mhi);
	if (ret) {
		dev_err(dev, "Fetching command ring context failed\n");
		goto err_ev_ctx_map;
	}

	epf_mhi->cmd_ctx_cache = pci_epc_mem_alloc_addr(epc, &epf_mhi->cmd_ctx_cache_dma_handle,
							sizeof(struct pci_epf_mhi_cmd_ctx));
	if (!epf_mhi->cmd_ctx_cache) {
		dev_err(dev, "Failed to allocate cmd_ctx_cache address\n");
		ret = -ENOMEM;
		goto err_ev_ctx_map;
	}

	ret = pci_epc_map_addr(epc, epf->func_no, epf_mhi->cmd_ctx_cache_dma_handle,
			       epf_mhi->cmd_ctx_shadow.host_pa, sizeof(struct pci_epf_mhi_cmd_ctx));
	if (ret) {
		dev_err(dev, "Failed to map address\n");
		goto err_cmd_ctx;
	}

	ret = pci_epf_mhi_update_ner(epf_mhi);
	if (ret) {
		dev_err(dev, "Fetching NER failed\n");
		goto err_cmd_ctx_map;
	}

	epf_mhi->ch_ctx_shadow.size = sizeof(struct pci_epf_mhi_ch_ctx) *
					epf_mhi->cfg.channels;
	epf_mhi->ev_ctx_shadow.size = sizeof(struct pci_epf_mhi_ev_ctx) *
					epf_mhi->cfg.event_rings;
	epf_mhi->cmd_ctx_shadow.size = sizeof(struct pci_epf_mhi_cmd_ctx);
	dev_info(dev, "Number of Event rings: %d, HW Event rings: %d\n",
			epf_mhi->cfg.event_rings, epf_mhi->cfg.hw_event_rings);

	dev_info(dev, 
			"cmd ring_base:0x%llx, rp:0x%llx, wp:0x%llx\n",
					epf_mhi->cmd_ctx_cache->rbase,
					epf_mhi->cmd_ctx_cache->rp,
					epf_mhi->cmd_ctx_cache->wp);
	dev_info(dev, 
			"ev ring_base:0x%llx, rp:0x%llx, wp:0x%llx\n",
					epf_mhi->ev_ctx_cache->rbase,
					epf_mhi->ev_ctx_cache->rp,
					epf_mhi->ev_ctx_cache->wp);

//	ret = pci_epf_mhi_ring_start(&epf_mhi->ring[0],
//			(union pci_epf_mhi_ring_ctx *)epf_mhi->cmd_ctx_cache, epf_mhi);
	if (ret) {
		dev_err(dev, "Failed to start the MHI ring\n");
		goto err_cmd_ctx_map;
	}

	return ret;

err_cmd_ctx_map:
	pci_epc_unmap_addr(epc, epf->func_no, epf_mhi->cmd_ctx_cache_dma_handle);

err_cmd_ctx:
	pci_epc_mem_free_addr(epc, epf_mhi->cmd_ctx_cache_dma_handle,
			      epf_mhi->cmd_ctx_cache, sizeof(struct pci_epf_mhi_cmd_ctx));

err_ev_ctx_map:
	pci_epc_unmap_addr(epc, epf->func_no, epf_mhi->ev_ctx_cache_dma_handle);

err_ev_ctx:
	pci_epc_mem_free_addr(epc, epf_mhi->ev_ctx_cache_dma_handle,
			      epf_mhi->ev_ctx_cache, sizeof(struct pci_epf_mhi_ev_ctx));

err_ch_ctx_map:
	pci_epc_unmap_addr(epc, epf->func_no, epf_mhi->ch_ctx_cache_dma_handle);

err_ch_ctx:
	pci_epc_mem_free_addr(epc, epf_mhi->ch_ctx_cache_dma_handle,
			      epf_mhi->ch_ctx_cache, sizeof(struct pci_epf_mhi_ch_ctx));

	return ret;
}

static int pci_epf_mhi_ring_cfg(struct pci_epf_mhi *epf_mhi)
{
	int i = 0;

	epf_mhi->cmd_ring_idx = 0;
	epf_mhi->ev_ring_start = 1;
	epf_mhi->ch_ring_start = epf_mhi->ev_ring_start + epf_mhi->cfg.event_rings;

	/* Initialize Command ring */
	pci_epf_mhi_ring_init(&epf_mhi->ring[epf_mhi->cmd_ring_idx],
				RING_TYPE_CMD, epf_mhi->cmd_ring_idx);
//	pci_epf_mhi_ring_set_cb(&epf_mhi->ring[epf_mhi->cmd_ring_idx],
//				pci_epf_mhi_process_cmd_ring);

	/* Initialize Event ring */
	for (i = epf_mhi->ev_ring_start; i < (epf_mhi->cfg.event_rings
					+ epf_mhi->ev_ring_start); i++)
		pci_epf_mhi_ring_init(&epf_mhi->ring[i], RING_TYPE_ER, i);

	/* Initialize Channel ring */
	for (i = epf_mhi->ch_ring_start; i < (epf_mhi->cfg.channels
					+ epf_mhi->ch_ring_start); i++) {
		pci_epf_mhi_ring_init(&epf_mhi->ring[i], RING_TYPE_CH, i);
//		pci_epf_mhi_ring_set_cb(&epf_mhi->ring[i], pci_epf_mhi_process_tre_ring);
	}

	return 0;
}

static int mhi_enable_int(struct pci_epf_mhi *epf_mhi)
{
	struct pci_epf *epf = epf_mhi->epf;
	struct device *dev = &epf->dev;
	int ret;

	ret = pci_epf_mhi_mmio_enable_chdb_interrupts(epf_mhi);
	if (ret) {
		dev_err(dev, "Failed to enable channel db: %d\n", ret);
		return ret;
	}

	ret = pci_epf_mhi_mmio_enable_ctrl_interrupt(epf_mhi);
	if (ret) {
		dev_err(dev, "Failed to enable control interrupt: %d\n", ret);
		return ret;
	}

	ret = pci_epf_mhi_mmio_enable_cmdb_interrupt(epf_mhi);
	if (ret) {
		dev_err(dev, "Failed to enable command db: %d\n", ret);
		return ret;
	}

	return ret;
}

extern void atu_dump(void);
static int pci_epf_mhi_ipa_init(struct pci_epf_mhi *epf_mhi)
{
	/* TODO: Setup IPA */
	return mhi_enable_int(epf_mhi);
}

static int pci_epf_mhi_enable(struct pci_epf_mhi *epf_mhi)
{
	struct pci_epf *epf = epf_mhi->epf;
	struct pci_epc *epc = epf->epc;
	struct device *dev = &epf->dev;
	enum pci_epf_mhi_state state;
	u32 max_cnt = 0, bhi_intvec = 0;
	u32 mhi_reset;
	int ret = 0;

	/* TODO: IPA DMA init */

	ret = pci_epf_mhi_ring_cfg(epf_mhi);
	if (ret) {
		dev_err(dev, "MHI dev ring init failed\n");
		return ret;
	}

	ret = pci_epf_mhi_mmio_read(epf_mhi, BHI_INTVEC, &bhi_intvec);
	if (ret)
		return ret;

	if (bhi_intvec != 0xffffffff) {
		/* Indicate the host that the device is ready */
		pci_epc_raise_irq(epc, epf->func_no, PCI_EPC_IRQ_MSI, bhi_intvec + 1);
	}

	ret = pci_epf_mhi_mmio_get_mhi_state(epf_mhi, &state, &mhi_reset);
	if (ret) {
		dev_err(dev, "%s: get mhi state failed\n", __func__);
		return ret;
	}

	if (mhi_reset) {
		pci_epf_mhi_mmio_clear_reset(epf_mhi);
		dev_info(dev, "Cleared reset before waiting for M0\n");
	}

	while (state != PCI_EPF_MHI_M0_STATE && max_cnt < MHI_SUSPEND_TIMEOUT) {
		/* Wait for Host to set the M0 state */
		msleep(MHI_SUSPEND_MIN);
		ret = pci_epf_mhi_mmio_get_mhi_state(epf_mhi, &state, &mhi_reset);
		if (ret) {
			dev_err(dev, "%s: get mhi state failed\n", __func__);
			return ret;
		}
		if (mhi_reset) {
			pci_epf_mhi_mmio_clear_reset(epf_mhi);
			dev_info(dev, "Cleared reset while waiting for M0\n");
		}
		max_cnt++;
	}

	dev_info(dev, "state:%d\n", state);

	if (state == PCI_EPF_MHI_M0_STATE) {
		ret = pci_epf_mhi_cache_host_cfg(epf_mhi);
		if (ret) {
			dev_err(dev, "Failed to cache the host config\n");
			return ret;
		}

		ret = pci_epf_mhi_mmio_set_env(epf_mhi, PCI_EPF_MHI_AMSS_EE);
		if (ret) {
			dev_err(dev, "%s: env setting failed\n", __func__);
			return ret;
		}
	} else {
		dev_err(dev, "MHI device failed to enter M0\n");
		return ret;
	}

	ret = pci_epf_mhi_ipa_init(epf_mhi);
	if (ret) {
		dev_err(dev, "error during hwc_init\n");
		return ret;
	}

	enable_irq(epf_mhi->irq);

	/*
	 * ctrl_info might already be set to CONNECTED state in the
	 * callback function mhi_hwc_cb triggered from IPA when mhi_hwc_init
	 * is called above, so set to CONFIGURED state only when it
	 * is not already set to CONNECTED
	 */
	if (epf_mhi->ctrl_info != PCI_EPF_MHI_STATE_CONNECTED)
		epf_mhi->ctrl_info = PCI_EPF_MHI_STATE_CONFIGURED;

	return 0;
}

static int pci_epf_mhi_recover(struct pci_epf_mhi *epf_mhi)
{
	struct pci_epf *epf = epf_mhi->epf;
	struct device *dev = &epf->dev;
	enum pci_epf_mhi_state state;
	u32 syserr, max_cnt = 0, bhi_intvec = 0, bhi_max_cnt = 0;
	u32 mhi_reset;
	int ret = 0;

	/* Check if MHI is in syserr */
	pci_epf_mhi_mmio_masked_read(epf_mhi, MHISTATUS,
				MHISTATUS_SYSERR_MASK,
				MHISTATUS_SYSERR_SHIFT, &syserr);

	if (syserr) {
		/* Poll for the host to set the reset bit */
		ret = pci_epf_mhi_mmio_get_mhi_state(epf_mhi, &state, &mhi_reset);
		if (ret) {
			dev_err(dev, "%s: get mhi state failed\n", __func__);
			return ret;
		}

		dev_info(dev, "mhi_state = 0x%X, reset = %d\n",
				state, mhi_reset);

		ret = pci_epf_mhi_mmio_read(epf_mhi, BHI_INTVEC, &bhi_intvec);
		if (ret)
			return ret;

		while (bhi_intvec == 0xffffffff &&
				bhi_max_cnt < MHI_BHI_INTVEC_MAX_CNT) {
			/* Wait for Host to set the bhi_intvec */
			msleep(MHI_BHI_INTVEC_WAIT_MS);
			dev_info(dev, "Wait for Host to set BHI_INTVEC\n");
			ret = pci_epf_mhi_mmio_read(epf_mhi, BHI_INTVEC, &bhi_intvec);
			if (ret) {
				dev_err(dev, "%s: Get BHI_INTVEC failed\n", __func__);
				return ret;
			}
			bhi_max_cnt++;
		}

		if (bhi_max_cnt == MHI_BHI_INTVEC_MAX_CNT) {
			dev_err(dev, "Host failed to set BHI_INTVEC\n");
			return -EINVAL;
		}

		/* Indicate the host that the device is ready */
		if (bhi_intvec != 0xffffffff)
			pci_epc_raise_irq(epf->epc, epf->func_no, PCI_EPC_IRQ_MSI, bhi_intvec + 1);

		/* Poll for the host to set the reset bit */
		ret = pci_epf_mhi_mmio_get_mhi_state(epf_mhi, &state, &mhi_reset);
		if (ret) {
			dev_err(dev, "%s: get mhi state failed\n", __func__);
			return ret;
		}

		dev_info(dev, "mhi_state = 0x%X, reset = %d\n",
				state, mhi_reset);

		while (mhi_reset != 0x1 && max_cnt < MHI_SUSPEND_TIMEOUT) {
			/* Wait for Host to set the reset */
			msleep(MHI_SUSPEND_MIN);
			ret = pci_epf_mhi_mmio_get_mhi_state(epf_mhi, &state,
								&mhi_reset);
			if (ret) {
				dev_err(dev, "%s: get mhi state failed\n", __func__);
				return ret;
			}
			max_cnt++;
		}

		if (!mhi_reset) {
			dev_info(dev, "Host failed to set reset\n");
			return -EINVAL;
		}
	}

	dev_info(dev, "MHI in reset state!\n");

	return 0;
}

static irqreturn_t pci_epf_mhi_db_handler(int irq, void *data)
{
	struct pci_epf_mhi *epf_mhi = data;
	struct pci_epf *epf = epf_mhi->epf;
	struct device *dev = &epf->dev;

	dev_info(dev, "MHI triggered\n");

	disable_irq_nosync(irq);
	return IRQ_HANDLED;
}

void pci_epf_mhi_hw_init(struct work_struct *work)
{
	struct pci_epf_mhi *epf_mhi = container_of(work, struct pci_epf_mhi, init_work);
	struct pci_epf *epf = epf_mhi->epf;
	struct pci_epc *epc = epf->epc;
	struct device *dev = &epf->dev;
	struct device *pdev = epc->dev.parent;
	int ret, i;

	/* TODO: Init workqueues */
	ret = pci_epf_mhi_recover(epf_mhi);
	if (ret)
		return;

	ret = pci_epf_mhi_mmio_init(epf_mhi);
	if (ret)
		return;

	/* TODO set the env before setting the ready bit */
	ret = pci_epf_mhi_mmio_set_env(epf_mhi, PCI_EPF_MHI_AMSS_EE);
	if (ret) {
		dev_err(dev, "%s: env setting failed\n", __func__);
		return;
	}

	epf_mhi->ring = devm_kzalloc(dev,
				(sizeof(struct pci_epf_mhi_ring) *
				(epf_mhi->cfg.channels + epf_mhi->cfg.event_rings + 1)),
				GFP_KERNEL);
	if (!epf_mhi->ring)
		return;

	epf_mhi->ch = devm_kzalloc(dev, (sizeof(struct pci_epf_mhi_channel) *
				   (epf_mhi->cfg.channels)), GFP_KERNEL);
	if (!epf_mhi->ch)
		return;

	for (i = 0; i < epf_mhi->cfg.channels; i++) {
		epf_mhi->ch[i].ch_id = i;
		mutex_init(&epf_mhi->ch[i].ch_lock);
	}

	spin_lock_init(&epf_mhi->lock);

	epf_mhi->mmio_backup = devm_kzalloc(dev,
			PCI_EPF_MHI_MMIO_RANGE, GFP_KERNEL);
	if (!epf_mhi->mmio_backup)
		return;

	epf_mhi->dma_cache = dma_alloc_coherent(pdev,
			(TRB_MAX_DATA_SIZE * 4),
			&epf_mhi->cache_dma_handle, GFP_KERNEL);
	if (!epf_mhi->dma_cache)
		return;

	epf_mhi->read_handle = dma_alloc_coherent(pdev,
			(TRB_MAX_DATA_SIZE * 4),
			&epf_mhi->read_dma_handle,
			GFP_KERNEL);
	if (!epf_mhi->read_handle)
		return;

	epf_mhi->write_handle = dma_alloc_coherent(pdev,
			(TRB_MAX_DATA_SIZE * 24),
			&epf_mhi->write_dma_handle,
			GFP_KERNEL);
	if (!epf_mhi->write_handle)
		return;

	ret = pci_epf_mhi_mmio_write(epf_mhi, MHIVER, 0x1000000);
	if (ret) {
		dev_err(dev, "Failed to update the MHI version\n");
		return;
	}

	/* Init state machine */
	ret = pci_epf_mhi_sm_init(epf_mhi);
	if (ret) {
		dev_err(dev, "Failed to init state machine: %d\n", ret);
		return;
	}

	/* set the env before setting the ready bit */
	ret = pci_epf_mhi_mmio_set_env(epf_mhi, PCI_EPF_MHI_AMSS_EE);
	if (ret) {
		dev_err(dev, "%s: env setting failed\n", __func__);
		return;
	}

	/* All set, notify the host */
	ret = pci_epf_mhi_sm_set_ready(epf_mhi);
	if (ret) {
		dev_err(dev, "%s: Failed to set ready\n", __func__);
		return;
	}

	irq_set_status_flags(epf_mhi->irq, IRQ_NOAUTOEN);
	ret = devm_request_irq(dev, epf_mhi->irq, pci_epf_mhi_db_handler,
			       IRQF_TRIGGER_HIGH, "doorbell_irq", epf_mhi);
	if (ret) {
		dev_err(dev, "Failed to request Doorbell IRQ\n");
		return;
	}

	ret = pci_epf_mhi_mmio_get_cfg(epf_mhi);
	if (ret)
		return;

	pci_epf_mhi_enable(epf_mhi);
}

static int pci_epf_mhi_notifier(struct notifier_block *nb, unsigned long val,
				 void *data)
{
	struct pci_epf *epf = container_of(nb, struct pci_epf, nb);
	struct pci_epf_mhi *epf_mhi = epf_get_drvdata(epf);
	struct pci_epf_bar *epf_bar = &epf->bar[0];
	struct pci_epc *epc = epf->epc;
	struct device *dev = &epf->dev;
	int ret;
	u32 dstate;

	switch (val) {
	case CORE_INIT:
		ret = pci_epc_write_header(epc, 0, &mhi_header);
		if (ret) {
			dev_err(dev, "Configuration header write failed\n");
			return NOTIFY_BAD;
		}

		epf_bar->phys_addr = epf_mhi->mmio_phys;
		epf_bar->size = epf_mhi->mmio_size;
		epf_bar->barno = BAR_0;
		epf_bar->flags = PCI_BASE_ADDRESS_MEM_TYPE_32;
		ret = pci_epc_set_bar(epc, 0, epf_bar);
		if (ret) {
			dev_err(dev, "Failed to set BAR0\n");
			return NOTIFY_BAD;
		}

		ret = pci_epc_set_msi(epc, 0, order_base_2(4));
		if (ret) {
			dev_err(dev, "MSI configuration failed\n");
			return NOTIFY_BAD;
		}
		break;
	case LINK_UP:
		break;
	case BME:
		queue_work(epf_mhi->init_wq, &epf_mhi->init_work);
		break;
	case D_STATE:
		dstate = (int)data;

		if (dstate == 0)
			dev_info(dev, "Received D0 event\n");
		break;
	default:
		dev_err(&epf->dev, "Invalid EPF mhi notifier event\n");
		return NOTIFY_BAD;
	}

	return NOTIFY_OK;
}

static int pci_epf_mhi_bind(struct pci_epf *epf)
{
	struct pci_epf_mhi *epf_mhi = epf_get_drvdata(epf);
	struct pci_epc *epc = epf->epc;
	struct platform_device *pdev = to_platform_device(epc->dev.parent);
	struct device *dev = &epf->dev;
	struct resource *res;
	int ret;

	if (WARN_ON_ONCE(!epc))
		return -EINVAL;

	/* Get MMIO physical and virtual address from controller device */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "mmio");
	epf_mhi->mmio_phys = res->start;
	epf_mhi->mmio_size = resource_size(res);

	epf_mhi->mmio = devm_ioremap_wc(dev, epf_mhi->mmio_phys, epf_mhi->mmio_size);
	if (IS_ERR(epf_mhi->mmio))
		return PTR_ERR(epf_mhi->mmio);

	ret = platform_get_irq_byname(pdev, "doorbell");
	if (ret < 0) {
		dev_err(dev, "Failed to get Doorbell IRQ\n");
		return ret;
	}

	epf_mhi->irq = ret;
	epf->nb.notifier_call = pci_epf_mhi_notifier;
	pci_epc_register_notifier(epc, &epf->nb);

	return 0;
}

static void pci_epf_mhi_unbind(struct pci_epf *epf)
{
	struct pci_epc *epc = epf->epc;
	struct pci_epf_bar *epf_bar = &epf->bar[0];

	pci_epc_clear_bar(epc, epf->func_no, epf_bar);
}

static int pci_epf_mhi_probe(struct pci_epf *epf)
{
	struct pci_epf_mhi *epf_mhi;
	struct device *dev = &epf->dev;

	epf_mhi = devm_kzalloc(dev, sizeof(*epf_mhi), GFP_KERNEL);
	if (!epf_mhi)
		return -ENOMEM;

	INIT_WORK(&epf_mhi->init_work, pci_epf_mhi_hw_init);

	epf_mhi->init_wq = alloc_workqueue("pci_epf_mhi_init_wq",
							WQ_HIGHPRI, 0);
	if (!epf_mhi->init_wq)
		return -ENOMEM;

	epf_set_drvdata(epf, epf_mhi);
	epf_mhi->epf = epf;

	return 0;
}

static const struct pci_epf_device_id pci_epf_mhi_ids[] = {
	{
		.name = "pci_epf_mhi",
	},
	{},
};

static struct pci_epf_ops ops = {
	.unbind	= pci_epf_mhi_unbind,
	.bind	= pci_epf_mhi_bind,
};

static struct pci_epf_driver mhi_driver = {
	.driver.name	= "pci_epf_mhi",
	.probe		= pci_epf_mhi_probe,
	.id_table	= pci_epf_mhi_ids,
	.ops		= &ops,
	.owner		= THIS_MODULE,
};

static int __init pci_epf_mhi_init(void)
{
	int ret;

	ret = pci_epf_register_driver(&mhi_driver);
	if (ret) {
		pr_err("Failed to register pci epf mhi driver --> %d\n", ret);
		return ret;
	}

	return 0;
}
module_init(pci_epf_mhi_init);

static void __exit pci_epf_mhi_exit(void)
{
	pci_epf_unregister_driver(&mhi_driver);
}
module_exit(pci_epf_mhi_exit);
