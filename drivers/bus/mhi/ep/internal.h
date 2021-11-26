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

/* MHI register definitions */
#define MHIREGLEN				0x100
#define MHIVER					0x108
#define MHICFG					0x110
#define CHDBOFF					0x118
#define ERDBOFF					0x120
#define BHIOFF					0x128
#define DEBUGOFF				0x130
#define MHICTRL					0x138
#define MHISTATUS				0x148
#define CCABAP_LOWER				0x158
#define CCABAP_HIGHER				0x15c
#define ECABAP_LOWER				0x160
#define ECABAP_HIGHER				0x164
#define CRCBAP_LOWER				0x168
#define CRCBAP_HIGHER				0x16c
#define CRDB_LOWER				0x170
#define CRDB_HIGHER				0x174
#define MHICTRLBASE_LOWER			0x180
#define MHICTRLBASE_HIGHER			0x184
#define MHICTRLLIMIT_LOWER			0x188
#define MHICTRLLIMIT_HIGHER			0x18c
#define MHIDATABASE_LOWER			0x198
#define MHIDATABASE_HIGHER			0x19c
#define MHIDATALIMIT_LOWER			0x1a0
#define MHIDATALIMIT_HIGHER			0x1a4
#define CHDB_LOWER_n(n)				(0x400 + 0x8 * (n))
#define CHDB_HIGHER_n(n)			(0x404 + 0x8 * (n))
#define ERDB_LOWER_n(n)				(0x800 + 0x8 * (n))
#define ERDB_HIGHER_n(n)			(0x804 + 0x8 * (n))
#define BHI_INTVEC				0x220
#define BHI_EXECENV				0x228
#define BHI_IMGTXDB				0x218

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

#define MHI_CTRL_INT_MASK_A7			0x94
#define MHI_CTRL_INT_MASK_A7_MASK_MASK		GENMASK(1, 0)
#define MHI_CTRL_MHICTRL_MASK			BIT(0)
#define MHI_CTRL_MHICTRL_SHFT			0
#define MHI_CTRL_CRDB_MASK			BIT(1)
#define MHI_CTRL_CRDB_SHFT			1

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

enum mhi_ep_ring_state {
	RING_STATE_UINT = 0,
	RING_STATE_IDLE,
};

enum mhi_ep_ring_type {
	RING_TYPE_CMD = 0,
	RING_TYPE_ER,
	RING_TYPE_CH,
	RING_TYPE_INVALID,
};

struct mhi_ep_ring_element {
	u64 ptr;
	u32 dword[2];
};

/* Transfer ring element type */
union mhi_ep_ring_ctx {
	struct mhi_cmd_ctxt cmd;
	struct mhi_event_ctxt ev;
	struct mhi_chan_ctxt ch;
	struct mhi_generic_ctx generic;
};

struct mhi_ep_ring {
	struct list_head list;
	struct mhi_ep_cntrl *mhi_cntrl;
	int (*ring_cb)(struct mhi_ep_ring *ring, struct mhi_ep_ring_element *el);
	union mhi_ep_ring_ctx *ring_ctx;
	struct mhi_ep_ring_element *ring_cache;
	enum mhi_ep_ring_type type;
	enum mhi_ep_ring_state state;
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
