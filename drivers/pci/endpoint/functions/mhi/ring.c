#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/completion.h>

#include "internal.h"

static size_t mhi_ring_addr2ofst(struct pci_epf_mhi_ring *ring, uint64_t p)
{
	uint64_t rbase;

	rbase = ring->ring_ctx->generic.rbase;

	return (p - rbase)/sizeof(union pci_epf_mhi_ring_element_type);
}

static uint32_t mhi_ring_num_elems(struct pci_epf_mhi_ring *ring)
{
	return ring->ring_ctx->generic.rlen/
			sizeof(union pci_epf_mhi_ring_element_type);
}

#if 0
int pci_epf_mhi_fetch_ring_elements(struct pci_epf_mhi_ring *ring,
					size_t start, size_t end)
{
	struct mhi_addr host_addr;
	struct pci_epf_mhi *mhi_ctx;

	mhi_ctx = ring->pci_epf_mhi;

	/* fetch ring elements from start->end, take care of wrap-around case */
	if (MHI_USE_DMA(mhi_ctx)) {
		host_addr.host_pa = ring->ring_shadow.host_pa
			+ sizeof(union pci_epf_mhi_ring_element_type) * start;
		host_addr.phy_addr = ring->ring_cache_dma_handle +
			(sizeof(union pci_epf_mhi_ring_element_type) * start);
	} else {
		host_addr.device_va = ring->ring_shadow.device_va
			+ sizeof(union pci_epf_mhi_ring_element_type) * start;
		host_addr.virt_addr = &ring->ring_cache[start];
	}
	host_addr.size = (end-start) * sizeof(union pci_epf_mhi_ring_element_type);
	if (start < end) {
		mhi_ctx->read_from_host(ring->pci_epf_mhi, &host_addr);
	} else if (start > end) {
		/* copy from 'start' to ring end, then ring start to 'end'*/
		host_addr.size = (ring->ring_size-start) *
					sizeof(union pci_epf_mhi_ring_element_type);
		mhi_ctx->read_from_host(ring->pci_epf_mhi, &host_addr);
		if (end) {
			/* wrapped around */
			host_addr.device_pa = ring->ring_shadow.device_pa;
			host_addr.device_va = ring->ring_shadow.device_va;
			host_addr.host_pa = ring->ring_shadow.host_pa;
			host_addr.virt_addr = &ring->ring_cache[0];
			host_addr.phy_addr = ring->ring_cache_dma_handle;
			host_addr.size = (end *
				sizeof(union pci_epf_mhi_ring_element_type));
			mhi_ctx->read_from_host(ring->pci_epf_mhi,
							&host_addr);
		}
	}
	return 0;
}

int pci_epf_mhi_cache_ring(struct pci_epf_mhi_ring *ring, size_t wr_offset)
{
	size_t old_offset = 0;
	struct pci_epf_mhi *mhi_ctx;

	if (WARN_ON(!ring))
		return -EINVAL;

	mhi_ctx = ring->pci_epf_mhi;

	if (ring->wr_offset == wr_offset) {
		mhi_log(MHI_MSG_VERBOSE,
			"nothing to cache for ring %d, local wr_ofst %d\n",
			ring->id, ring->wr_offset);
		mhi_log(MHI_MSG_VERBOSE,
			"new wr_offset %d\n", wr_offset);
		return 0;
	}

	old_offset = ring->wr_offset;

	/*
	 * copy the elements starting from old_offset to wr_offset
	 * take in to account wrap around case event rings are not
	 * cached, not required
	 */
	if (ring->id >= mhi_ctx->ev_ring_start &&
		ring->id < (mhi_ctx->ev_ring_start +
				mhi_ctx->cfg.event_rings)) {
		mhi_log(MHI_MSG_VERBOSE,
				"not caching event ring %d\n", ring->id);
		return 0;
	}

	mhi_log(MHI_MSG_VERBOSE, "caching ring %d, start %d, end %d\n",
			ring->id, old_offset, wr_offset);

	if (pci_epf_mhi_fetch_ring_elements(ring, old_offset, wr_offset)) {
		mhi_log(MHI_MSG_ERROR,
		"failed to fetch elements for ring %d, start %d, end %d\n",
		ring->id, old_offset, wr_offset);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_cache_ring);

int pci_epf_mhi_ring_start(struct pci_epf_mhi_ring *ring, union pci_epf_mhi_ring_ctx *ctx,
							struct pci_epf_mhi *epf_mhi)
{
	struct pci_epf *epf = epf_mhi->epf;
	struct pci_epc *epc = epf->epc;
	struct device *pdev = epc->dev.parent;
	size_t wr_offset = 0;
	size_t offset = 0;
	int ret = 0;

	ring->ring_ctx = ctx;
	ring->ring_size = mhi_ring_num_elems(ring);
	ring->rd_offset = mhi_ring_addr2ofst(ring,
					ring->ring_ctx->generic.rp);
	ring->wr_offset = mhi_ring_addr2ofst(ring,
					ring->ring_ctx->generic.rp);
	ring->epf_mhi = epf_mhi;

	pci_epf_mhi_ring_set_state(ring, RING_STATE_IDLE);

	wr_offset = mhi_ring_addr2ofst(ring,
					ring->ring_ctx->generic.wp);

	if (!ring->ring_cache) {
		ring->ring_cache = dma_alloc_coherent(pdev,
				ring->ring_size *
				sizeof(union pci_epf_mhi_ring_element_type),
				&ring->ring_cache_dma_handle,
				GFP_KERNEL);
		if (!ring->ring_cache) {
			pr_err(
				"Failed to allocate ring cache\n");
			return -ENOMEM;
		}
	}

	if (ring->type == RING_TYPE_ER) {
		if (!ring->evt_rp_cache) {
			ring->evt_rp_cache = dma_alloc_coherent(pdev,
				sizeof(uint64_t) * ring->ring_size,
				&ring->evt_rp_cache_dma_handle,
				GFP_KERNEL);
			if (!ring->evt_rp_cache) {
				pr_err(
					"Failed to allocate evt rp cache\n");
				ret = -ENOMEM;
				goto cleanup;
			}
		}
		if (!ring->msi_buf) {
			ring->msi_buf = dma_alloc_coherent(pdev,
				sizeof(uint32_t),
				&ring->msi_buf_dma_handle,
				GFP_KERNEL);
			if (!ring->msi_buf) {
				pr_err(
					"Failed to allocate msi buf\n");
				ret = -ENOMEM;
				goto cleanup;
			}
		}
	}

	offset = (size_t)(ring->ring_ctx->generic.rbase -
					epf_mhi->ctrl_base.host_pa);

	ring->ring_shadow.device_pa = epf_mhi->ctrl_base.device_pa + offset;
	ring->ring_shadow.device_va = epf_mhi->ctrl_base.device_va + offset;
	ring->ring_shadow.host_pa = epf_mhi->ctrl_base.host_pa + offset;

	if (ring->type == RING_TYPE_ER)
		ring->ring_ctx_shadow =
		(union pci_epf_mhi_ring_ctx *) (epf_mhi->ev_ctx_shadow.device_va +
			(ring->id - epf_mhi->ev_ring_start) *
			sizeof(union pci_epf_mhi_ring_ctx));
	else if (ring->type == RING_TYPE_CMD)
		ring->ring_ctx_shadow =
		(union pci_epf_mhi_ring_ctx *) epf_mhi->cmd_ctx_shadow.device_va;
	else if (ring->type == RING_TYPE_CH)
		ring->ring_ctx_shadow =
		(union pci_epf_mhi_ring_ctx *) (epf_mhi->ch_ctx_shadow.device_va +
		(ring->id - epf_mhi->ch_ring_start)*sizeof(union pci_epf_mhi_ring_ctx));

	ring->ring_ctx_shadow = ring->ring_ctx;

	if (ring->type != RING_TYPE_ER || ring->type != RING_TYPE_CH) {
		ret = pci_epf_mhi_cache_ring(ring, wr_offset);
		if (ret)
			return ret;
	}

	pr_err( "ctx ring_base:0x%x, rp:0x%x, wp:0x%x\n",
			(size_t)ring->ring_ctx->generic.rbase,
			(size_t)ring->ring_ctx->generic.rp,
			(size_t)ring->ring_ctx->generic.wp);
	ring->wr_offset = wr_offset;

	return ret;

cleanup:
	dma_free_coherent(pdev,
		ring->ring_size *
		sizeof(union pci_epf_mhi_ring_element_type),
		ring->ring_cache,
		ring->ring_cache_dma_handle);
	ring->ring_cache = NULL;

	if (ring->evt_rp_cache) {
		dma_free_coherent(pdev,
			sizeof(uint64_t) * ring->ring_size,
			ring->evt_rp_cache,
			ring->evt_rp_cache_dma_handle);
		ring->evt_rp_cache = NULL;
	}

	return ret;
}
EXPORT_SYMBOL(pci_epf_mhi_ring_start);
#endif

void pci_epf_mhi_ring_init(struct pci_epf_mhi_ring *ring, enum pci_epf_mhi_ring_type type,
								int id)
{
	if (WARN_ON(!ring))
		return;

	ring->id = id;
	ring->state = RING_STATE_UINT;
	ring->ring_cb = NULL;
	ring->type = type;
	mutex_init(&ring->event_lock);
}
EXPORT_SYMBOL(pci_epf_mhi_ring_init);

void pci_epf_mhi_ring_set_cb(struct pci_epf_mhi_ring *ring,
			void (*ring_cb)(struct pci_epf_mhi *dev,
			union pci_epf_mhi_ring_element_type *el, void *ctx))
{
	if (WARN_ON(!ring || !ring_cb))
		return;

	ring->ring_cb = ring_cb;
}
EXPORT_SYMBOL(pci_epf_mhi_ring_set_cb);

void pci_epf_mhi_ring_set_state(struct pci_epf_mhi_ring *ring,
				enum pci_epf_mhi_ring_state state)
{
	if (WARN_ON(!ring))
		return;

	if (state > RING_STATE_PENDING) {
		pr_err("%s: Invalid ring state\n", __func__);
		return;
	}

	ring->state = state;
}
EXPORT_SYMBOL(pci_epf_mhi_ring_set_state);

enum pci_epf_mhi_ring_state pci_epf_mhi_ring_get_state(struct pci_epf_mhi_ring *ring)
{
	if (WARN_ON(!ring))
		return -EINVAL;

	return ring->state;
}
EXPORT_SYMBOL(pci_epf_mhi_ring_get_state);
