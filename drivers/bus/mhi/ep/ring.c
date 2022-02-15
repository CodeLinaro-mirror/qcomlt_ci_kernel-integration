// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Linaro Ltd.
 * Author: Manivannan Sadhasivam <manivannan.sadhasivam@linaro.org>
 */

#include <linux/mhi_ep.h>
#include "internal.h"

size_t mhi_ep_ring_addr2offset(struct mhi_ep_ring *ring, u64 ptr)
{
	u64 rbase;

	rbase = le64_to_cpu(ring->ring_ctx->generic.rbase);

	return (ptr - rbase) / sizeof(struct mhi_ep_ring_element);
}

static u32 mhi_ep_ring_num_elems(struct mhi_ep_ring *ring)
{
	return le64_to_cpu(ring->ring_ctx->generic.rlen) / sizeof(struct mhi_ep_ring_element);
}

void mhi_ep_ring_inc_index(struct mhi_ep_ring *ring)
{
	ring->rd_offset++;
	if (ring->rd_offset == ring->ring_size)
		ring->rd_offset = 0;
}

static int __mhi_ep_cache_ring(struct mhi_ep_ring *ring, size_t end)
{
	struct mhi_ep_cntrl *mhi_cntrl = ring->mhi_cntrl;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	size_t start, copy_size;
	int ret;

	/* No need to cache event rings */
	if (ring->type == RING_TYPE_ER)
		return 0;

	/* No need to cache the ring if write pointer is unmodified */
	if (ring->wr_offset == end)
		return 0;

	start = ring->wr_offset;
	if (start < end) {
		copy_size = (end - start) * sizeof(struct mhi_ep_ring_element);
		ret = mhi_cntrl->read_from_host(mhi_cntrl,
						(le64_to_cpu(ring->ring_ctx->generic.rbase) +
						(start * sizeof(struct mhi_ep_ring_element))),
						&ring->ring_cache[start], copy_size);
		if (ret < 0)
			return ret;
	} else {
		copy_size = (ring->ring_size - start) * sizeof(struct mhi_ep_ring_element);
		ret = mhi_cntrl->read_from_host(mhi_cntrl,
						(le64_to_cpu(ring->ring_ctx->generic.rbase) +
						(start * sizeof(struct mhi_ep_ring_element))),
						&ring->ring_cache[start], copy_size);
		if (ret < 0)
			return ret;

		if (end) {
			ret = mhi_cntrl->read_from_host(mhi_cntrl,
							le64_to_cpu(ring->ring_ctx->generic.rbase),
							&ring->ring_cache[0],
							end * sizeof(struct mhi_ep_ring_element));
			if (ret < 0)
				return ret;
		}
	}

	dev_dbg(dev, "Cached ring: start %zu end %zu size %zu\n", start, end, copy_size);

	return 0;
}

static int mhi_ep_cache_ring(struct mhi_ep_ring *ring, u64 wr_ptr)
{
	size_t wr_offset;
	int ret;

	wr_offset = mhi_ep_ring_addr2offset(ring, wr_ptr);

	/* Cache the host ring till write offset */
	ret = __mhi_ep_cache_ring(ring, wr_offset);
	if (ret)
		return ret;

	ring->wr_offset = wr_offset;

	return 0;
}

int mhi_ep_update_wr_offset(struct mhi_ep_ring *ring)
{
	u64 wr_ptr;

	wr_ptr = mhi_ep_mmio_get_db(ring);

	return mhi_ep_cache_ring(ring, wr_ptr);
}

static int mhi_ep_process_ring_element(struct mhi_ep_ring *ring, size_t offset)
{
	struct mhi_ep_ring_element *el;

	/* Get the element and invoke the respective callback */
	el = &ring->ring_cache[offset];

	return ring->ring_cb(ring, el);
}

int mhi_ep_process_ring(struct mhi_ep_ring *ring)
{
	struct mhi_ep_cntrl *mhi_cntrl = ring->mhi_cntrl;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	int ret = 0;

	/* Event rings should not be processed */
	if (ring->type == RING_TYPE_ER)
		return -EINVAL;

	dev_dbg(dev, "Processing ring of type: %d\n", ring->type);

	/* Update the write offset for the ring */
	ret = mhi_ep_update_wr_offset(ring);
	if (ret) {
		dev_err(dev, "Error updating write offset for ring\n");
		return ret;
	}

	/* Sanity check to make sure there are elements in the ring */
	if (ring->rd_offset == ring->wr_offset)
		return 0;

	/* Process channel ring first */
	if (ring->type == RING_TYPE_CH) {
		ret = mhi_ep_process_ring_element(ring, ring->rd_offset);
		if (ret)
			dev_err(dev, "Error processing ch ring element: %zu\n", ring->rd_offset);

		return ret;
	}

	/* Process command ring now */
	while (ring->rd_offset != ring->wr_offset) {
		ret = mhi_ep_process_ring_element(ring, ring->rd_offset);
		if (ret) {
			dev_err(dev, "Error processing cmd ring element: %zu\n", ring->rd_offset);
			return ret;
		}

		mhi_ep_ring_inc_index(ring);
	}

	return 0;
}

/* TODO: Support for adding multiple ring elements to the ring */
int mhi_ep_ring_add_element(struct mhi_ep_ring *ring, struct mhi_ep_ring_element *el)
{
	struct mhi_ep_cntrl *mhi_cntrl = ring->mhi_cntrl;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	__le64 rbase = ring->ring_ctx->generic.rbase;
	size_t old_offset = 0;
	u32 num_free_elem;
	int ret;

	ret = mhi_ep_update_wr_offset(ring);
	if (ret) {
		dev_err(dev, "Error updating write pointer\n");
		return ret;
	}

	if (ring->rd_offset < ring->wr_offset)
		num_free_elem = (ring->wr_offset - ring->rd_offset) - 1;
	else
		num_free_elem = ((ring->ring_size - ring->rd_offset) + ring->wr_offset) - 1;

	/* Check if there is space in ring for adding at least an element */
	if (!num_free_elem) {
		dev_err(dev, "No space left in the ring\n");
		return -ENOSPC;
	}

	old_offset = ring->rd_offset;
	mhi_ep_ring_inc_index(ring);

	dev_dbg(dev, "Adding an element to ring at offset (%zu)\n", ring->rd_offset);

	/* Update rp in ring context */
	ring->ring_ctx->generic.rp = cpu_to_le64((ring->rd_offset * sizeof(*el))) + rbase;

	/* Ensure that the ring pointer gets updated before writing the element to ring */
	smp_wmb();

	ret = mhi_cntrl->write_to_host(mhi_cntrl, el, (le64_to_cpu(rbase) +
				       (old_offset * sizeof(*el))), sizeof(*el));
	if (ret < 0)
		return ret;

	return 0;
}

void mhi_ep_ring_init(struct mhi_ep_ring *ring, enum mhi_ep_ring_type type, u32 id)
{
	ring->type = type;
	if (ring->type == RING_TYPE_CMD) {
		ring->ring_cb = mhi_ep_process_cmd_ring;
		ring->db_offset_h = CRDB_HIGHER;
		ring->db_offset_l = CRDB_LOWER;
	} else if (ring->type == RING_TYPE_CH) {
		ring->ring_cb = mhi_ep_process_tre_ring;
		ring->db_offset_h = CHDB_HIGHER_n(id);
		ring->db_offset_l = CHDB_LOWER_n(id);
		ring->ch_id = id;
	} else {
		ring->db_offset_h = ERDB_HIGHER_n(id);
		ring->db_offset_l = ERDB_LOWER_n(id);
	}
}

int mhi_ep_ring_start(struct mhi_ep_cntrl *mhi_cntrl, struct mhi_ep_ring *ring,
			union mhi_ep_ring_ctx *ctx)
{
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	int ret;

	ring->mhi_cntrl = mhi_cntrl;
	ring->ring_ctx = ctx;
	ring->ring_size = mhi_ep_ring_num_elems(ring);

	if (ring->type == RING_TYPE_CH)
		ring->er_index = le32_to_cpu(ring->ring_ctx->ch.erindex);

	if (ring->type == RING_TYPE_ER)
		ring->irq_vector = le32_to_cpu(ring->ring_ctx->ev.msivec);

	/* During ring init, both rp and wp are equal */
	ring->rd_offset = mhi_ep_ring_addr2offset(ring, le64_to_cpu(ring->ring_ctx->generic.rp));
	ring->wr_offset = mhi_ep_ring_addr2offset(ring, le64_to_cpu(ring->ring_ctx->generic.rp));

	/* Allocate ring cache memory for holding the copy of host ring */
	ring->ring_cache = kcalloc(ring->ring_size, sizeof(struct mhi_ep_ring_element),
				   GFP_KERNEL);
	if (!ring->ring_cache)
		return -ENOMEM;

	ret = mhi_ep_cache_ring(ring, le64_to_cpu(ring->ring_ctx->generic.wp));
	if (ret) {
		dev_err(dev, "Failed to cache ring\n");
		kfree(ring->ring_cache);
		return ret;
	}

	ring->started = true;

	return 0;
}

void mhi_ep_ring_reset(struct mhi_ep_cntrl *mhi_cntrl, struct mhi_ep_ring *ring)
{
	ring->started = false;
	kfree(ring->ring_cache);
}
