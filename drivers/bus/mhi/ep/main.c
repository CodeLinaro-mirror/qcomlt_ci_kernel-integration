// SPDX-License-Identifier: GPL-2.0
/*
 * MHI Bus Endpoint stack
 *
 * Copyright (C) 2021 Linaro Ltd.
 * Author: Manivannan Sadhasivam <manivannan.sadhasivam@linaro.org>
 */

#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/dma-direction.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/mhi_ep.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include "internal.h"

#define MHI_SUSPEND_MIN			100
#define MHI_SUSPEND_TIMEOUT		600

static DEFINE_IDA(mhi_ep_cntrl_ida);

static int mhi_ep_destroy_device(struct device *dev, void *data);

static int mhi_ep_send_event(struct mhi_ep_cntrl *mhi_cntrl, u32 ring_idx,
			     struct mhi_ep_ring_element *el)
{
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	union mhi_ep_ring_ctx *ctx;
	struct mhi_ep_ring *ring;
	int ret;

	mutex_lock(&mhi_cntrl->event_lock);
	ring = &mhi_cntrl->mhi_event[ring_idx].ring;
	ctx = (union mhi_ep_ring_ctx *)&mhi_cntrl->ev_ctx_cache[ring_idx];
	if (!ring->started) {
		ret = mhi_ep_ring_start(mhi_cntrl, ring, ctx);
		if (ret) {
			dev_err(dev, "Error starting event ring (%d)\n", ring_idx);
			goto err_unlock;
		}
	}

	/* Add element to the event ring */
	ret = mhi_ep_ring_add_element(ring, el);
	if (ret) {
		dev_err(dev, "Error adding element to event ring (%d)\n", ring_idx);
		goto err_unlock;
	}

	/* Ensure that the ring pointer gets updated in host memory before triggering IRQ */
	smp_wmb();

	mutex_unlock(&mhi_cntrl->event_lock);

	/*
	 * Raise IRQ to host only if the BEI flag is not set in TRE. Host might
	 * set this flag for interrupt moderation as per MHI protocol.
	 */
	if (!MHI_EP_TRE_GET_BEI(el))
		mhi_cntrl->raise_irq(mhi_cntrl, ring->irq_vector);

	return 0;

err_unlock:
	mutex_unlock(&mhi_cntrl->event_lock);

	return ret;
}

static int mhi_ep_send_completion_event(struct mhi_ep_cntrl *mhi_cntrl,
					struct mhi_ep_ring *ring, u32 len,
					enum mhi_ev_ccs code)
{
	struct mhi_ep_ring_element event = {};
	__le32 tmp;

	event.ptr = le64_to_cpu(ring->ring_ctx->generic.rbase) +
			ring->rd_offset * sizeof(struct mhi_ep_ring_element);

	tmp = event.dword[0];
	tmp |= MHI_TRE_EV_DWORD0(code, len);
	event.dword[0] = tmp;

	tmp = event.dword[1];
	tmp |= MHI_TRE_EV_DWORD1(ring->ch_id, MHI_PKT_TYPE_TX_EVENT);
	event.dword[1] = tmp;

	return mhi_ep_send_event(mhi_cntrl, ring->er_index, &event);
}

int mhi_ep_send_state_change_event(struct mhi_ep_cntrl *mhi_cntrl, enum mhi_state state)
{
	struct mhi_ep_ring_element event = {};
	__le32 tmp;

	tmp = event.dword[0];
	tmp |= MHI_SC_EV_DWORD0(state);
	event.dword[0] = tmp;

	tmp = event.dword[1];
	tmp |= MHI_SC_EV_DWORD1(MHI_PKT_TYPE_STATE_CHANGE_EVENT);
	event.dword[1] = tmp;

	return mhi_ep_send_event(mhi_cntrl, 0, &event);
}

int mhi_ep_send_ee_event(struct mhi_ep_cntrl *mhi_cntrl, enum mhi_ep_execenv exec_env)
{
	struct mhi_ep_ring_element event = {};
	__le32 tmp;

	tmp = event.dword[0];
	tmp |= MHI_EE_EV_DWORD0(exec_env);
	event.dword[0] = tmp;

	tmp = event.dword[1];
	tmp |= MHI_SC_EV_DWORD1(MHI_PKT_TYPE_EE_EVENT);
	event.dword[1] = tmp;

	return mhi_ep_send_event(mhi_cntrl, 0, &event);
}

static int mhi_ep_send_cmd_comp_event(struct mhi_ep_cntrl *mhi_cntrl, enum mhi_ev_ccs code)
{
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	struct mhi_ep_ring_element event = {};
	__le32 tmp;

	if (code > MHI_EV_CC_BAD_TRE) {
		dev_err(dev, "Invalid command completion code (%d)\n", code);
		return -EINVAL;
	}

	event.ptr = le64_to_cpu(mhi_cntrl->cmd_ctx_cache->rbase)
			+ (mhi_cntrl->mhi_cmd->ring.rd_offset *
			(sizeof(struct mhi_ep_ring_element)));

	tmp = event.dword[0];
	tmp |= MHI_CC_EV_DWORD0(code);
	event.dword[0] = tmp;

	tmp = event.dword[1];
	tmp |= MHI_CC_EV_DWORD1(MHI_PKT_TYPE_CMD_COMPLETION_EVENT);
	event.dword[1] = tmp;

	return mhi_ep_send_event(mhi_cntrl, 0, &event);
}

int mhi_ep_alloc_map(struct mhi_ep_cntrl *mhi_cntrl, u64 pci_addr, size_t size,
		     phys_addr_t *phys_ptr, void __iomem **virt)
{
	size_t offset = pci_addr % 0x1000;
	void __iomem *buf;
	phys_addr_t phys;
	int ret;

	size += offset;

	buf = mhi_cntrl->alloc_addr(mhi_cntrl, &phys, size);
	if (!buf)
		return -ENOMEM;

	ret = mhi_cntrl->map_addr(mhi_cntrl, phys, pci_addr - offset, size);
	if (ret) {
		mhi_cntrl->free_addr(mhi_cntrl, phys, buf, size);
		return ret;
	}

	*phys_ptr = phys + offset;
	*virt = buf + offset;

	return 0;
}

void mhi_ep_unmap_free(struct mhi_ep_cntrl *mhi_cntrl, u64 pci_addr, phys_addr_t phys,
			void __iomem *virt, size_t size)
{
	size_t offset = pci_addr % 0x1000;

	size += offset;

	mhi_cntrl->unmap_addr(mhi_cntrl, phys - offset);
	mhi_cntrl->free_addr(mhi_cntrl, phys - offset, virt - offset, size);
}

static int mhi_ep_cache_host_cfg(struct mhi_ep_cntrl *mhi_cntrl)
{
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	int ret;

	/* Update the number of event rings (NER) programmed by the host */
	mhi_ep_mmio_update_ner(mhi_cntrl);

	dev_dbg(dev, "Number of Event rings: %d, HW Event rings: %d\n",
		 mhi_cntrl->event_rings, mhi_cntrl->hw_event_rings);

	mhi_cntrl->ch_ctx_host_size = sizeof(struct mhi_chan_ctxt) *
					mhi_cntrl->max_chan;
	mhi_cntrl->ev_ctx_host_size = sizeof(struct mhi_event_ctxt) *
					mhi_cntrl->event_rings;
	mhi_cntrl->cmd_ctx_host_size = sizeof(struct mhi_cmd_ctxt);

	/* Get the channel context base pointer from host */
	mhi_ep_mmio_get_chc_base(mhi_cntrl);

	/* Allocate and map memory for caching host channel context */
	ret = mhi_ep_alloc_map(mhi_cntrl, mhi_cntrl->ch_ctx_host_pa, mhi_cntrl->ch_ctx_host_size,
				&mhi_cntrl->ch_ctx_cache_phys,
				(void __iomem **)&mhi_cntrl->ch_ctx_cache);
	if (ret) {
		dev_err(dev, "Failed to allocate and map ch_ctx_cache\n");
		return ret;
	}

	/* Get the event context base pointer from host */
	mhi_ep_mmio_get_erc_base(mhi_cntrl);

	/* Allocate and map memory for caching host event context */
	ret = mhi_ep_alloc_map(mhi_cntrl, mhi_cntrl->ev_ctx_host_pa, mhi_cntrl->ev_ctx_host_size,
				&mhi_cntrl->ev_ctx_cache_phys,
				(void __iomem **)&mhi_cntrl->ev_ctx_cache);
	if (ret) {
		dev_err(dev, "Failed to allocate and map ev_ctx_cache\n");
		goto err_ch_ctx;
	}

	/* Get the command context base pointer from host */
	mhi_ep_mmio_get_crc_base(mhi_cntrl);

	/* Allocate and map memory for caching host command context */
	ret = mhi_ep_alloc_map(mhi_cntrl, mhi_cntrl->cmd_ctx_host_pa, mhi_cntrl->cmd_ctx_host_size,
				&mhi_cntrl->cmd_ctx_cache_phys,
				(void __iomem **)&mhi_cntrl->cmd_ctx_cache);
	if (ret) {
		dev_err(dev, "Failed to allocate and map cmd_ctx_cache\n");
		goto err_ev_ctx;
	}

	/* Initialize command ring */
	ret = mhi_ep_ring_start(mhi_cntrl, &mhi_cntrl->mhi_cmd->ring,
				(union mhi_ep_ring_ctx *)mhi_cntrl->cmd_ctx_cache);
	if (ret) {
		dev_err(dev, "Failed to start the command ring\n");
		goto err_cmd_ctx;
	}

	return ret;

err_cmd_ctx:
	mhi_ep_unmap_free(mhi_cntrl, mhi_cntrl->cmd_ctx_host_pa, mhi_cntrl->cmd_ctx_cache_phys,
			mhi_cntrl->cmd_ctx_cache, mhi_cntrl->cmd_ctx_host_size);

err_ev_ctx:
	mhi_ep_unmap_free(mhi_cntrl, mhi_cntrl->ev_ctx_host_pa, mhi_cntrl->ev_ctx_cache_phys,
			mhi_cntrl->ev_ctx_cache, mhi_cntrl->ev_ctx_host_size);

err_ch_ctx:
	mhi_ep_unmap_free(mhi_cntrl, mhi_cntrl->ch_ctx_host_pa, mhi_cntrl->ch_ctx_cache_phys,
			mhi_cntrl->ch_ctx_cache, mhi_cntrl->ch_ctx_host_size);

	return ret;
}

static void mhi_ep_free_host_cfg(struct mhi_ep_cntrl *mhi_cntrl)
{
	mhi_ep_unmap_free(mhi_cntrl, mhi_cntrl->cmd_ctx_host_pa, mhi_cntrl->cmd_ctx_cache_phys,
			mhi_cntrl->cmd_ctx_cache, mhi_cntrl->cmd_ctx_host_size);
	mhi_ep_unmap_free(mhi_cntrl, mhi_cntrl->ev_ctx_host_pa, mhi_cntrl->ev_ctx_cache_phys,
			mhi_cntrl->ev_ctx_cache, mhi_cntrl->ev_ctx_host_size);
	mhi_ep_unmap_free(mhi_cntrl, mhi_cntrl->ch_ctx_host_pa, mhi_cntrl->ch_ctx_cache_phys,
			mhi_cntrl->ch_ctx_cache, mhi_cntrl->ch_ctx_host_size);
}

static void mhi_ep_enable_int(struct mhi_ep_cntrl *mhi_cntrl)
{
	mhi_ep_mmio_enable_ctrl_interrupt(mhi_cntrl);
	mhi_ep_mmio_enable_cmdb_interrupt(mhi_cntrl);
}

static int mhi_ep_enable(struct mhi_ep_cntrl *mhi_cntrl)
{
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	enum mhi_state state;
	u32 max_cnt = 0;
	bool mhi_reset;
	int ret;

	/* Wait for Host to set the M0 state */
	do {
		msleep(MHI_SUSPEND_MIN);
		mhi_ep_mmio_get_mhi_state(mhi_cntrl, &state, &mhi_reset);
		if (mhi_reset) {
			/* Clear the MHI reset if host is in reset state */
			mhi_ep_mmio_clear_reset(mhi_cntrl);
			dev_dbg(dev, "Host initiated reset while waiting for M0\n");
		}
		max_cnt++;
	} while (state != MHI_STATE_M0 && max_cnt < MHI_SUSPEND_TIMEOUT);

	if (state == MHI_STATE_M0) {
		ret = mhi_ep_cache_host_cfg(mhi_cntrl);
		if (ret) {
			dev_err(dev, "Failed to cache host config\n");
			return ret;
		}

		mhi_ep_mmio_set_env(mhi_cntrl, MHI_EP_AMSS_EE);
	} else {
		dev_err(dev, "Host failed to enter M0\n");
		return -ETIMEDOUT;
	}

	/* Enable all interrupts now */
	mhi_ep_enable_int(mhi_cntrl);

	return 0;
}

static void mhi_ep_ring_worker(struct work_struct *work)
{
	struct mhi_ep_cntrl *mhi_cntrl = container_of(work,
				struct mhi_ep_cntrl, ring_work);
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	struct mhi_ep_ring_item *itr, *tmp;
	struct mhi_ep_ring *ring;
	struct mhi_ep_chan *chan;
	unsigned long flags;
	LIST_HEAD(head);
	int ret;

	/* Process the command ring first */
	ret = mhi_ep_process_ring(&mhi_cntrl->mhi_cmd->ring);
	if (ret) {
		dev_err(dev, "Error processing command ring: %d\n", ret);
		return;
	}

	spin_lock_irqsave(&mhi_cntrl->list_lock, flags);
	list_splice_tail_init(&mhi_cntrl->ch_db_list, &head);
	spin_unlock_irqrestore(&mhi_cntrl->list_lock, flags);

	/* Process the channel rings now */
	list_for_each_entry_safe(itr, tmp, &head, node) {
		list_del(&itr->node);
		ring = itr->ring;
		chan = &mhi_cntrl->mhi_chan[ring->ch_id];
		mutex_lock(&chan->lock);
		dev_dbg(dev, "Processing the ring for channel (%d)\n", ring->ch_id);
		ret = mhi_ep_process_ring(ring);
		if (ret) {
			dev_err(dev, "Error processing ring for channel (%d): %d\n",
				ring->ch_id, ret);
			mutex_unlock(&chan->lock);
			return;
		}
		mutex_unlock(&chan->lock);
		kfree(itr);
	}
}

static void mhi_ep_queue_channel_db(struct mhi_ep_cntrl *mhi_cntrl,
				    unsigned long ch_int, u32 ch_idx)
{
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	struct mhi_ep_ring_item *item;
	struct mhi_ep_ring *ring;
	unsigned int i;

	for_each_set_bit(i, &ch_int, 32) {
		/* Channel index varies for each register: 0, 32, 64, 96 */
		i += ch_idx;
		ring = &mhi_cntrl->mhi_chan[i].ring;

		item = kmalloc(sizeof(*item), GFP_ATOMIC);
		item->ring = ring;

		dev_dbg(dev, "Queuing doorbell interrupt for channel (%d)\n", i);
		spin_lock(&mhi_cntrl->list_lock);
		list_add_tail(&item->node, &mhi_cntrl->ch_db_list);
		spin_unlock(&mhi_cntrl->list_lock);

		queue_work(mhi_cntrl->ring_wq, &mhi_cntrl->ring_work);
	}
}

/*
 * Channel interrupt statuses are contained in 4 registers each of 32bit length.
 * For checking all interrupts, we need to loop through each registers and then
 * check for bits set.
 */
static void mhi_ep_check_channel_interrupt(struct mhi_ep_cntrl *mhi_cntrl)
{
	u32 ch_int, ch_idx;
	int i;

	mhi_ep_mmio_read_chdb_status_interrupts(mhi_cntrl);

	for (i = 0; i < MHI_MASK_ROWS_CH_EV_DB; i++) {
		ch_idx = i * MHI_MASK_CH_EV_LEN;

		/* Only process channel interrupt if the mask is enabled */
		ch_int = (mhi_cntrl->chdb[i].status & mhi_cntrl->chdb[i].mask);
		if (ch_int) {
			mhi_ep_queue_channel_db(mhi_cntrl, ch_int, ch_idx);
			mhi_ep_mmio_write(mhi_cntrl, MHI_CHDB_INT_CLEAR_A7_n(i),
							mhi_cntrl->chdb[i].status);
		}
	}
}

static void mhi_ep_state_worker(struct work_struct *work)
{
	struct mhi_ep_cntrl *mhi_cntrl = container_of(work, struct mhi_ep_cntrl, state_work);
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	struct mhi_ep_state_transition *itr, *tmp;
	unsigned long flags;
	LIST_HEAD(head);
	int ret;

	spin_lock_irqsave(&mhi_cntrl->list_lock, flags);
	list_splice_tail_init(&mhi_cntrl->st_transition_list, &head);
	spin_unlock_irqrestore(&mhi_cntrl->list_lock, flags);

	list_for_each_entry_safe(itr, tmp, &head, node) {
		list_del(&itr->node);
		dev_dbg(dev, "Handling MHI state transition to %s\n",
			 mhi_state_str(itr->state));

		switch (itr->state) {
		case MHI_STATE_M0:
			ret = mhi_ep_set_m0_state(mhi_cntrl);
			if (ret)
				dev_err(dev, "Failed to transition to M0 state\n");
			break;
		case MHI_STATE_M3:
			ret = mhi_ep_set_m3_state(mhi_cntrl);
			if (ret)
				dev_err(dev, "Failed to transition to M3 state\n");
			break;
		default:
			dev_err(dev, "Invalid MHI state transition: %d\n", itr->state);
			break;
		}
		kfree(itr);
	}
}

static void mhi_ep_process_ctrl_interrupt(struct mhi_ep_cntrl *mhi_cntrl,
					 enum mhi_state state)
{
	struct mhi_ep_state_transition *item = kmalloc(sizeof(*item), GFP_ATOMIC);

	item->state = state;
	spin_lock(&mhi_cntrl->list_lock);
	list_add_tail(&item->node, &mhi_cntrl->st_transition_list);
	spin_unlock(&mhi_cntrl->list_lock);

	queue_work(mhi_cntrl->state_wq, &mhi_cntrl->state_work);
}

/*
 * Interrupt handler that services interrupts raised by the host writing to
 * MHICTRL and Command ring doorbell (CRDB) registers for state change and
 * channel interrupts.
 */
static irqreturn_t mhi_ep_irq(int irq, void *data)
{
	struct mhi_ep_cntrl *mhi_cntrl = data;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	enum mhi_state state;
	u32 int_value;

	/* Acknowledge the interrupts */
	int_value = mhi_ep_mmio_read(mhi_cntrl, MHI_CTRL_INT_STATUS_A7);
	mhi_ep_mmio_write(mhi_cntrl, MHI_CTRL_INT_CLEAR_A7, int_value);

	/* Check for ctrl interrupt */
	if (FIELD_GET(MHI_CTRL_INT_STATUS_A7_MSK, int_value)) {
		dev_dbg(dev, "Processing ctrl interrupt\n");
		mhi_ep_process_ctrl_interrupt(mhi_cntrl, state);
	}

	/* Check for command doorbell interrupt */
	if (FIELD_GET(MHI_CTRL_INT_STATUS_CRDB_MSK, int_value)) {
		dev_dbg(dev, "Processing command doorbell interrupt\n");
		queue_work(mhi_cntrl->ring_wq, &mhi_cntrl->ring_work);
	}

	/* Check for channel interrupts */
	mhi_ep_check_channel_interrupt(mhi_cntrl);

	return IRQ_HANDLED;
}

static void mhi_ep_abort_transfer(struct mhi_ep_cntrl *mhi_cntrl)
{
	struct mhi_ep_ring *ch_ring, *ev_ring;
	struct mhi_result result = {};
	struct mhi_ep_chan *mhi_chan;
	int i;

	/* Stop all the channels */
	for (i = 0; i < mhi_cntrl->max_chan; i++) {
		ch_ring = &mhi_cntrl->mhi_chan[i].ring;
		if (!ch_ring->started)
			continue;

		mhi_chan = &mhi_cntrl->mhi_chan[i];
		mutex_lock(&mhi_chan->lock);
		/* Send channel disconnect status to client drivers */
		if (mhi_chan->xfer_cb) {
			result.transaction_status = -ENOTCONN;
			result.bytes_xferd = 0;
			mhi_chan->xfer_cb(mhi_chan->mhi_dev, &result);
		}

		/* Set channel state to DISABLED */
		mhi_chan->state = MHI_CH_STATE_DISABLED;
		mutex_unlock(&mhi_chan->lock);
	}

	flush_workqueue(mhi_cntrl->ring_wq);
	flush_workqueue(mhi_cntrl->state_wq);

	/* Destroy devices associated with all channels */
	device_for_each_child(&mhi_cntrl->mhi_dev->dev, NULL, mhi_ep_destroy_device);

	/* Stop and reset the transfer rings */
	for (i = 0; i < mhi_cntrl->max_chan; i++) {
		ch_ring = &mhi_cntrl->mhi_chan[i].ring;
		if (!ch_ring->started)
			continue;

		mhi_chan = &mhi_cntrl->mhi_chan[i];
		mutex_lock(&mhi_chan->lock);
		mhi_ep_ring_reset(mhi_cntrl, ch_ring);
		mutex_unlock(&mhi_chan->lock);
	}

	/* Stop and reset the event rings */
	for (i = 0; i < mhi_cntrl->event_rings; i++) {
		ev_ring = &mhi_cntrl->mhi_event[i].ring;
		if (!ev_ring->started)
			continue;

		mutex_lock(&mhi_cntrl->event_lock);
		mhi_ep_ring_reset(mhi_cntrl, ev_ring);
		mutex_unlock(&mhi_cntrl->event_lock);
	}

	/* Stop and reset the command ring */
	mhi_ep_ring_reset(mhi_cntrl, &mhi_cntrl->mhi_cmd->ring);

	mhi_ep_free_host_cfg(mhi_cntrl);
	mhi_ep_mmio_mask_interrupts(mhi_cntrl);

	mhi_cntrl->is_enabled = false;
}

int mhi_ep_power_up(struct mhi_ep_cntrl *mhi_cntrl)
{
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	int ret, i;

	/*
	 * Mask all interrupts until the state machine is ready. Interrupts will
	 * be enabled later with mhi_ep_enable().
	 */
	mhi_ep_mmio_mask_interrupts(mhi_cntrl);
	mhi_ep_mmio_init(mhi_cntrl);

	mhi_cntrl->mhi_event = kzalloc(mhi_cntrl->event_rings * (sizeof(*mhi_cntrl->mhi_event)),
					GFP_KERNEL);
	if (!mhi_cntrl->mhi_event)
		return -ENOMEM;

	/* Initialize command, channel and event rings */
	mhi_ep_ring_init(&mhi_cntrl->mhi_cmd->ring, RING_TYPE_CMD, 0);
	for (i = 0; i < mhi_cntrl->max_chan; i++)
		mhi_ep_ring_init(&mhi_cntrl->mhi_chan[i].ring, RING_TYPE_CH, i);
	for (i = 0; i < mhi_cntrl->event_rings; i++)
		mhi_ep_ring_init(&mhi_cntrl->mhi_event[i].ring, RING_TYPE_ER, i);

	spin_lock_bh(&mhi_cntrl->state_lock);
	mhi_cntrl->mhi_state = MHI_STATE_RESET;
	spin_unlock_bh(&mhi_cntrl->state_lock);

	/* Set AMSS EE before signaling ready state */
	mhi_ep_mmio_set_env(mhi_cntrl, MHI_EP_AMSS_EE);

	/* All set, notify the host that we are ready */
	ret = mhi_ep_set_ready_state(mhi_cntrl);
	if (ret)
		goto err_free_event;

	dev_dbg(dev, "READY state notification sent to the host\n");

	ret = mhi_ep_enable(mhi_cntrl);
	if (ret) {
		dev_err(dev, "Failed to enable MHI endpoint\n");
		goto err_free_event;
	}

	enable_irq(mhi_cntrl->irq);
	mhi_cntrl->is_enabled = true;

	return 0;

err_free_event:
	kfree(mhi_cntrl->mhi_event);

	return ret;
}
EXPORT_SYMBOL_GPL(mhi_ep_power_up);

void mhi_ep_power_down(struct mhi_ep_cntrl *mhi_cntrl)
{
	if (mhi_cntrl->is_enabled)
		mhi_ep_abort_transfer(mhi_cntrl);

	kfree(mhi_cntrl->mhi_event);
	disable_irq(mhi_cntrl->irq);
}
EXPORT_SYMBOL_GPL(mhi_ep_power_down);

static void mhi_ep_release_device(struct device *dev)
{
	struct mhi_ep_device *mhi_dev = to_mhi_ep_device(dev);

	if (mhi_dev->dev_type == MHI_DEVICE_CONTROLLER)
		mhi_dev->mhi_cntrl->mhi_dev = NULL;

	/*
	 * We need to set the mhi_chan->mhi_dev to NULL here since the MHI
	 * devices for the channels will only get created during start
	 * channel if the mhi_dev associated with it is NULL.
	 */
	if (mhi_dev->ul_chan)
		mhi_dev->ul_chan->mhi_dev = NULL;

	if (mhi_dev->dl_chan)
		mhi_dev->dl_chan->mhi_dev = NULL;

	kfree(mhi_dev);
}

static struct mhi_ep_device *mhi_ep_alloc_device(struct mhi_ep_cntrl *mhi_cntrl,
						 enum mhi_device_type dev_type)
{
	struct mhi_ep_device *mhi_dev;
	struct device *dev;

	mhi_dev = kzalloc(sizeof(*mhi_dev), GFP_KERNEL);
	if (!mhi_dev)
		return ERR_PTR(-ENOMEM);

	dev = &mhi_dev->dev;
	device_initialize(dev);
	dev->bus = &mhi_ep_bus_type;
	dev->release = mhi_ep_release_device;

	if (dev_type == MHI_DEVICE_CONTROLLER)
		/* for MHI controller device, parent is the bus device (e.g. PCI EPF) */
		dev->parent = mhi_cntrl->cntrl_dev;
	else
		/* for MHI client devices, parent is the MHI controller device */
		dev->parent = &mhi_cntrl->mhi_dev->dev;

	mhi_dev->mhi_cntrl = mhi_cntrl;
	mhi_dev->dev_type = dev_type;

	return mhi_dev;
}

/*
 * MHI channels are always defined in pairs with UL as the even numbered
 * channel and DL as odd numbered one.
 */
static int mhi_ep_create_device(struct mhi_ep_cntrl *mhi_cntrl, u32 ch_id)
{
	struct mhi_ep_chan *mhi_chan = &mhi_cntrl->mhi_chan[ch_id];
	struct mhi_ep_device *mhi_dev;
	int ret;

	/* Check if the channel name is same for both UL and DL */
	if (strcmp(mhi_chan->name, mhi_chan[1].name))
		return -EINVAL;

	mhi_dev = mhi_ep_alloc_device(mhi_cntrl, MHI_DEVICE_XFER);
	if (IS_ERR(mhi_dev))
		return PTR_ERR(mhi_dev);

	/* Configure primary channel */
	mhi_dev->ul_chan = mhi_chan;
	get_device(&mhi_dev->dev);
	mhi_chan->mhi_dev = mhi_dev;

	/* Configure secondary channel as well */
	mhi_chan++;
	mhi_dev->dl_chan = mhi_chan;
	get_device(&mhi_dev->dev);
	mhi_chan->mhi_dev = mhi_dev;

	/* Channel name is same for both UL and DL */
	mhi_dev->name = mhi_chan->name;
	dev_set_name(&mhi_dev->dev, "%s_%s",
		     dev_name(&mhi_cntrl->mhi_dev->dev),
		     mhi_dev->name);

	ret = device_add(&mhi_dev->dev);
	if (ret)
		put_device(&mhi_dev->dev);

	return ret;
}

static int mhi_ep_destroy_device(struct device *dev, void *data)
{
	struct mhi_ep_device *mhi_dev;
	struct mhi_ep_cntrl *mhi_cntrl;
	struct mhi_ep_chan *ul_chan, *dl_chan;

	if (dev->bus != &mhi_ep_bus_type)
		return 0;

	mhi_dev = to_mhi_ep_device(dev);
	mhi_cntrl = mhi_dev->mhi_cntrl;

	/* Only destroy devices created for channels */
	if (mhi_dev->dev_type == MHI_DEVICE_CONTROLLER)
		return 0;

	ul_chan = mhi_dev->ul_chan;
	dl_chan = mhi_dev->dl_chan;

	if (ul_chan)
		put_device(&ul_chan->mhi_dev->dev);

	if (dl_chan)
		put_device(&dl_chan->mhi_dev->dev);

	dev_dbg(&mhi_cntrl->mhi_dev->dev, "Destroying device for chan:%s\n",
		 mhi_dev->name);

	/* Notify the client and remove the device from MHI bus */
	device_del(dev);
	put_device(dev);

	return 0;
}

static int parse_ch_cfg(struct mhi_ep_cntrl *mhi_cntrl,
			const struct mhi_ep_cntrl_config *config)
{
	const struct mhi_ep_channel_config *ch_cfg;
	struct device *dev = mhi_cntrl->cntrl_dev;
	u32 chan, i;
	int ret = -EINVAL;

	mhi_cntrl->max_chan = config->max_channels;

	/*
	 * Allocate max_channels supported by the MHI endpoint and populate
	 * only the defined channels
	 */
	mhi_cntrl->mhi_chan = kcalloc(mhi_cntrl->max_chan, sizeof(*mhi_cntrl->mhi_chan),
				      GFP_KERNEL);
	if (!mhi_cntrl->mhi_chan)
		return -ENOMEM;

	for (i = 0; i < config->num_channels; i++) {
		struct mhi_ep_chan *mhi_chan;

		ch_cfg = &config->ch_cfg[i];

		chan = ch_cfg->num;
		if (chan >= mhi_cntrl->max_chan) {
			dev_err(dev, "Channel %d not available\n", chan);
			goto error_chan_cfg;
		}

		/* Bi-directional and direction less channels are not supported */
		if (ch_cfg->dir == DMA_BIDIRECTIONAL || ch_cfg->dir == DMA_NONE) {
			dev_err(dev, "Invalid channel configuration\n");
			goto error_chan_cfg;
		}

		mhi_chan = &mhi_cntrl->mhi_chan[chan];
		mhi_chan->name = ch_cfg->name;
		mhi_chan->chan = chan;
		mhi_chan->dir = ch_cfg->dir;
		mutex_init(&mhi_chan->lock);
	}

	return 0;

error_chan_cfg:
	kfree(mhi_cntrl->mhi_chan);

	return ret;
}

/*
 * Allocate channel and command rings here. Event rings will be allocated
 * in mhi_ep_power_up() as the config comes from the host.
 */
int mhi_ep_register_controller(struct mhi_ep_cntrl *mhi_cntrl,
				const struct mhi_ep_cntrl_config *config)
{
	struct mhi_ep_device *mhi_dev;
	int ret;

	if (!mhi_cntrl || !mhi_cntrl->cntrl_dev || !mhi_cntrl->mmio || !mhi_cntrl->irq)
		return -EINVAL;

	ret = parse_ch_cfg(mhi_cntrl, config);
	if (ret)
		return ret;

	mhi_cntrl->mhi_cmd = kcalloc(NR_OF_CMD_RINGS, sizeof(*mhi_cntrl->mhi_cmd), GFP_KERNEL);
	if (!mhi_cntrl->mhi_cmd) {
		ret = -ENOMEM;
		goto err_free_ch;
	}

	INIT_WORK(&mhi_cntrl->ring_work, mhi_ep_ring_worker);
	INIT_WORK(&mhi_cntrl->state_work, mhi_ep_state_worker);

	mhi_cntrl->ring_wq = alloc_workqueue("mhi_ep_ring_wq", 0, 0);
	if (!mhi_cntrl->ring_wq) {
		ret = -ENOMEM;
		goto err_free_cmd;
	}

	mhi_cntrl->state_wq = alloc_workqueue("mhi_ep_state_wq", 0, 0);
	if (!mhi_cntrl->state_wq) {
		ret = -ENOMEM;
		goto err_destroy_ring_wq;
	}

	INIT_LIST_HEAD(&mhi_cntrl->ch_db_list);
	INIT_LIST_HEAD(&mhi_cntrl->st_transition_list);
	spin_lock_init(&mhi_cntrl->list_lock);
	spin_lock_init(&mhi_cntrl->state_lock);
	mutex_init(&mhi_cntrl->event_lock);

	/* Set MHI version and AMSS EE before enumeration */
	mhi_ep_mmio_write(mhi_cntrl, MHIVER, config->mhi_version);
	mhi_ep_mmio_set_env(mhi_cntrl, MHI_EP_AMSS_EE);

	/* Set controller index */
	mhi_cntrl->index = ida_alloc(&mhi_ep_cntrl_ida, GFP_KERNEL);
	if (mhi_cntrl->index < 0) {
		ret = mhi_cntrl->index;
		goto err_destroy_state_wq;
	}

	irq_set_status_flags(mhi_cntrl->irq, IRQ_NOAUTOEN);
	ret = request_irq(mhi_cntrl->irq, mhi_ep_irq, IRQF_TRIGGER_HIGH,
			  "doorbell_irq", mhi_cntrl);
	if (ret) {
		dev_err(mhi_cntrl->cntrl_dev, "Failed to request Doorbell IRQ\n");
		goto err_ida_free;
	}

	/* Allocate the controller device */
	mhi_dev = mhi_ep_alloc_device(mhi_cntrl, MHI_DEVICE_CONTROLLER);
	if (IS_ERR(mhi_dev)) {
		dev_err(mhi_cntrl->cntrl_dev, "Failed to allocate controller device\n");
		ret = PTR_ERR(mhi_dev);
		goto err_free_irq;
	}

	dev_set_name(&mhi_dev->dev, "mhi_ep%d", mhi_cntrl->index);
	mhi_dev->name = dev_name(&mhi_dev->dev);

	ret = device_add(&mhi_dev->dev);
	if (ret)
		goto err_put_dev;

	mhi_cntrl->mhi_dev = mhi_dev;

	dev_dbg(&mhi_dev->dev, "MHI EP Controller registered\n");

	return 0;

err_put_dev:
	put_device(&mhi_dev->dev);
err_free_irq:
	free_irq(mhi_cntrl->irq, mhi_cntrl);
err_ida_free:
	ida_free(&mhi_ep_cntrl_ida, mhi_cntrl->index);
err_destroy_state_wq:
	destroy_workqueue(mhi_cntrl->state_wq);
err_destroy_ring_wq:
	destroy_workqueue(mhi_cntrl->ring_wq);
err_free_cmd:
	kfree(mhi_cntrl->mhi_cmd);
err_free_ch:
	kfree(mhi_cntrl->mhi_chan);

	return ret;
}
EXPORT_SYMBOL_GPL(mhi_ep_register_controller);

/*
 * It is expected that the controller drivers will power down the MHI EP stack
 * using "mhi_ep_power_down()" before calling this function to unregister themselves.
 */
void mhi_ep_unregister_controller(struct mhi_ep_cntrl *mhi_cntrl)
{
	struct mhi_ep_device *mhi_dev = mhi_cntrl->mhi_dev;

	destroy_workqueue(mhi_cntrl->state_wq);
	destroy_workqueue(mhi_cntrl->ring_wq);

	free_irq(mhi_cntrl->irq, mhi_cntrl);

	kfree(mhi_cntrl->mhi_cmd);
	kfree(mhi_cntrl->mhi_chan);

	device_del(&mhi_dev->dev);
	put_device(&mhi_dev->dev);

	ida_free(&mhi_ep_cntrl_ida, mhi_cntrl->index);
}
EXPORT_SYMBOL_GPL(mhi_ep_unregister_controller);

static int mhi_ep_driver_probe(struct device *dev)
{
	struct mhi_ep_device *mhi_dev = to_mhi_ep_device(dev);
	struct mhi_ep_driver *mhi_drv = to_mhi_ep_driver(dev->driver);
	struct mhi_ep_chan *ul_chan = mhi_dev->ul_chan;
	struct mhi_ep_chan *dl_chan = mhi_dev->dl_chan;

	/* Client drivers should have callbacks for both channels */
	if (!mhi_drv->ul_xfer_cb || !mhi_drv->dl_xfer_cb)
		return -EINVAL;

	ul_chan->xfer_cb = mhi_drv->ul_xfer_cb;
	dl_chan->xfer_cb = mhi_drv->dl_xfer_cb;

	return mhi_drv->probe(mhi_dev, mhi_dev->id);
}

static int mhi_ep_driver_remove(struct device *dev)
{
	struct mhi_ep_device *mhi_dev = to_mhi_ep_device(dev);
	struct mhi_ep_driver *mhi_drv = to_mhi_ep_driver(dev->driver);
	struct mhi_result result = {};
	struct mhi_ep_chan *mhi_chan;
	int dir;

	/* Skip if it is a controller device */
	if (mhi_dev->dev_type == MHI_DEVICE_CONTROLLER)
		return 0;

	/* Disconnect the channels associated with the driver */
	for (dir = 0; dir < 2; dir++) {
		mhi_chan = dir ? mhi_dev->ul_chan : mhi_dev->dl_chan;

		if (!mhi_chan)
			continue;

		mutex_lock(&mhi_chan->lock);
		/* Send channel disconnect status to the client driver */
		if (mhi_chan->xfer_cb) {
			result.transaction_status = -ENOTCONN;
			result.bytes_xferd = 0;
			mhi_chan->xfer_cb(mhi_chan->mhi_dev, &result);
		}

		/* Set channel state to DISABLED */
		mhi_chan->state = MHI_CH_STATE_DISABLED;
		mhi_chan->xfer_cb = NULL;
		mutex_unlock(&mhi_chan->lock);
	}

	/* Remove the client driver now */
	mhi_drv->remove(mhi_dev);

	return 0;
}

int __mhi_ep_driver_register(struct mhi_ep_driver *mhi_drv, struct module *owner)
{
	struct device_driver *driver = &mhi_drv->driver;

	if (!mhi_drv->probe || !mhi_drv->remove)
		return -EINVAL;

	driver->bus = &mhi_ep_bus_type;
	driver->owner = owner;
	driver->probe = mhi_ep_driver_probe;
	driver->remove = mhi_ep_driver_remove;

	return driver_register(driver);
}
EXPORT_SYMBOL_GPL(__mhi_ep_driver_register);

void mhi_ep_driver_unregister(struct mhi_ep_driver *mhi_drv)
{
	driver_unregister(&mhi_drv->driver);
}
EXPORT_SYMBOL_GPL(mhi_ep_driver_unregister);

static int mhi_ep_match(struct device *dev, struct device_driver *drv)
{
	struct mhi_ep_device *mhi_dev = to_mhi_ep_device(dev);
	struct mhi_ep_driver *mhi_drv = to_mhi_ep_driver(drv);
	const struct mhi_device_id *id;

	/*
	 * If the device is a controller type then there is no client driver
	 * associated with it
	 */
	if (mhi_dev->dev_type == MHI_DEVICE_CONTROLLER)
		return 0;

	for (id = mhi_drv->id_table; id->chan[0]; id++)
		if (!strcmp(mhi_dev->name, id->chan)) {
			mhi_dev->id = id;
			return 1;
		}

	return 0;
};

struct bus_type mhi_ep_bus_type = {
	.name = "mhi_ep",
	.dev_name = "mhi_ep",
	.match = mhi_ep_match,
};

static int __init mhi_ep_init(void)
{
	return bus_register(&mhi_ep_bus_type);
}

static void __exit mhi_ep_exit(void)
{
	bus_unregister(&mhi_ep_bus_type);
}

postcore_initcall(mhi_ep_init);
module_exit(mhi_ep_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MHI Bus Endpoint stack");
MODULE_AUTHOR("Manivannan Sadhasivam <manivannan.sadhasivam@linaro.org>");
