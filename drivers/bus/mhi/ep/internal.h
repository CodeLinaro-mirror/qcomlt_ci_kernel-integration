/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021, Linaro Ltd.
 *
 */

#ifndef _MHI_EP_INTERNAL_
#define _MHI_EP_INTERNAL_

#include <linux/bitfield.h>

#include "../common.h"

extern struct bus_type mhi_ep_bus_type;

#define MHI_REG_OFFSET				0x100
#define BHI_REG_OFFSET				0x200

/* MHI registers */
#define MHIREGLEN				(MHI_REG_OFFSET + REG_MHIREGLEN)
#define MHIVER					(MHI_REG_OFFSET + REG_MHIVER)
#define MHICFG					(MHI_REG_OFFSET + REG_MHICFG)
#define CHDBOFF					(MHI_REG_OFFSET + REG_CHDBOFF)
#define ERDBOFF					(MHI_REG_OFFSET + REG_ERDBOFF)
#define BHIOFF					(MHI_REG_OFFSET + REG_BHIOFF)
#define BHIEOFF					(MHI_REG_OFFSET + REG_BHIEOFF)
#define DEBUGOFF				(MHI_REG_OFFSET + REG_DEBUGOFF)
#define MHICTRL					(MHI_REG_OFFSET + REG_MHICTRL)
#define MHISTATUS				(MHI_REG_OFFSET + REG_MHISTATUS)
#define CCABAP_LOWER				(MHI_REG_OFFSET + REG_CCABAP_LOWER)
#define CCABAP_HIGHER				(MHI_REG_OFFSET + REG_CCABAP_HIGHER)
#define ECABAP_LOWER				(MHI_REG_OFFSET + REG_ECABAP_LOWER)
#define ECABAP_HIGHER				(MHI_REG_OFFSET + REG_ECABAP_HIGHER)
#define CRCBAP_LOWER				(MHI_REG_OFFSET + REG_CRCBAP_LOWER)
#define CRCBAP_HIGHER				(MHI_REG_OFFSET + REG_CRCBAP_HIGHER)
#define CRDB_LOWER				(MHI_REG_OFFSET + REG_CRDB_LOWER)
#define CRDB_HIGHER				(MHI_REG_OFFSET + REG_CRDB_HIGHER)
#define MHICTRLBASE_LOWER			(MHI_REG_OFFSET + REG_MHICTRLBASE_LOWER)
#define MHICTRLBASE_HIGHER			(MHI_REG_OFFSET + REG_MHICTRLBASE_HIGHER)
#define MHICTRLLIMIT_LOWER			(MHI_REG_OFFSET + REG_MHICTRLLIMIT_LOWER)
#define MHICTRLLIMIT_HIGHER			(MHI_REG_OFFSET + REG_MHICTRLLIMIT_HIGHER)
#define MHIDATABASE_LOWER			(MHI_REG_OFFSET + REG_MHIDATABASE_LOWER)
#define MHIDATABASE_HIGHER			(MHI_REG_OFFSET + REG_MHIDATABASE_HIGHER)
#define MHIDATALIMIT_LOWER			(MHI_REG_OFFSET + REG_MHIDATALIMIT_LOWER)
#define MHIDATALIMIT_HIGHER			(MHI_REG_OFFSET + REG_MHIDATALIMIT_HIGHER)

/* MHI BHI registers */
#define BHI_IMGTXDB				(BHI_REG_OFFSET + REG_BHI_IMGTXDB)
#define BHI_EXECENV				(BHI_REG_OFFSET + REG_BHI_EXECENV)
#define BHI_INTVEC				(BHI_REG_OFFSET + REG_BHI_INTVEC)

/* MHI Doorbell registers */
#define CHDB_LOWER_n(n)				(0x400 + 0x8 * (n))
#define CHDB_HIGHER_n(n)			(0x404 + 0x8 * (n))
#define ERDB_LOWER_n(n)				(0x800 + 0x8 * (n))
#define ERDB_HIGHER_n(n)			(0x804 + 0x8 * (n))

#define MHI_CTRL_INT_STATUS_A7			0x4
#define MHI_CTRL_INT_STATUS_A7_MSK		BIT(0)
#define MHI_CTRL_INT_STATUS_CRDB_MSK		BIT(1)
#define MHI_CHDB_INT_STATUS_A7_n(n)		(0x28 + 0x4 * (n))
#define MHI_ERDB_INT_STATUS_A7_n(n)		(0x38 + 0x4 * (n))

#define MHI_CTRL_INT_CLEAR_A7			0x4c
#define MHI_CTRL_INT_MMIO_WR_CLEAR		BIT(2)
#define MHI_CTRL_INT_CRDB_CLEAR			BIT(1)
#define MHI_CTRL_INT_CRDB_MHICTRL_CLEAR		BIT(0)

#define MHI_CHDB_INT_CLEAR_A7_n(n)		(0x70 + 0x4 * (n))
#define MHI_CHDB_INT_CLEAR_A7_n_CLEAR_ALL	GENMASK(31, 0)
#define MHI_ERDB_INT_CLEAR_A7_n(n)		(0x80 + 0x4 * (n))
#define MHI_ERDB_INT_CLEAR_A7_n_CLEAR_ALL	GENMASK(31, 0)

/*
 * Unlike the usual "masking" convention, writing "1" to a bit in this register
 * enables the interrupt and writing "0" will disable it..
 */
#define MHI_CTRL_INT_MASK_A7			0x94
#define MHI_CTRL_INT_MASK_A7_MASK		GENMASK(1, 0)
#define MHI_CTRL_MHICTRL_MASK			BIT(0)
#define MHI_CTRL_CRDB_MASK			BIT(1)

#define MHI_CHDB_INT_MASK_A7_n(n)		(0xb8 + 0x4 * (n))
#define MHI_CHDB_INT_MASK_A7_n_EN_ALL		GENMASK(31, 0)
#define MHI_ERDB_INT_MASK_A7_n(n)		(0xc8 + 0x4 * (n))
#define MHI_ERDB_INT_MASK_A7_n_EN_ALL		GENMASK(31, 0)

#define NR_OF_CMD_RINGS				1
#define MHI_MASK_ROWS_CH_EV_DB			4
#define MHI_MASK_CH_EV_LEN			32

/* Generic context */
struct mhi_generic_ctx {
	__u32 reserved0;
	__u32 reserved1;
	__u32 reserved2;

	__u64 rbase __packed __aligned(4);
	__u64 rlen __packed __aligned(4);
	__u64 rp __packed __aligned(4);
	__u64 wp __packed __aligned(4);
};

enum mhi_ep_ring_type {
	RING_TYPE_CMD = 0,
	RING_TYPE_ER,
	RING_TYPE_CH,
};

struct mhi_ep_ring_element {
	u64 ptr;
	u32 dword[2];
};

/* Ring element */
union mhi_ep_ring_ctx {
	struct mhi_cmd_ctxt cmd;
	struct mhi_event_ctxt ev;
	struct mhi_chan_ctxt ch;
	struct mhi_generic_ctx generic;
};

struct mhi_ep_ring {
	struct mhi_ep_cntrl *mhi_cntrl;
	int (*ring_cb)(struct mhi_ep_ring *ring, struct mhi_ep_ring_element *el);
	union mhi_ep_ring_ctx *ring_ctx;
	struct mhi_ep_ring_element *ring_cache;
	enum mhi_ep_ring_type type;
	size_t rd_offset;
	size_t wr_offset;
	size_t ring_size;
	u32 db_offset_h;
	u32 db_offset_l;
	u32 ch_id;
};

struct mhi_ep_cmd {
	struct mhi_ep_ring ring;
};

struct mhi_ep_event {
	struct mhi_ep_ring ring;
};

struct mhi_ep_chan {
	char *name;
	struct mhi_ep_device *mhi_dev;
	struct mhi_ep_ring ring;
	struct mutex lock;
	void (*xfer_cb)(struct mhi_ep_device *mhi_dev, struct mhi_result *result);
	enum mhi_ch_state state;
	enum dma_data_direction dir;
	u64 tre_loc;
	u32 tre_size;
	u32 tre_bytes_left;
	u32 chan;
	bool skip_td;
};

#endif
