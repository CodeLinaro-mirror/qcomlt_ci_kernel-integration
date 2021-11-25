/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021, Linaro Ltd.
 *
 */

#ifndef _MHI_COMMON_H
#define _MHI_COMMON_H

#include <linux/mhi.h>

/* Command Ring Element macros */
/* No operation command */
#define MHI_TRE_CMD_NOOP_PTR (0)
#define MHI_TRE_CMD_NOOP_DWORD0 (0)
#define MHI_TRE_CMD_NOOP_DWORD1 (cpu_to_le32(MHI_CMD_NOP << 16))

/* Channel reset command */
#define MHI_TRE_CMD_RESET_PTR (0)
#define MHI_TRE_CMD_RESET_DWORD0 (0)
#define MHI_TRE_CMD_RESET_DWORD1(chid) (cpu_to_le32((chid << 24) | \
					(MHI_CMD_RESET_CHAN << 16)))

/* Channel stop command */
#define MHI_TRE_CMD_STOP_PTR (0)
#define MHI_TRE_CMD_STOP_DWORD0 (0)
#define MHI_TRE_CMD_STOP_DWORD1(chid) (cpu_to_le32((chid << 24) | \
				       (MHI_CMD_STOP_CHAN << 16)))

/* Channel start command */
#define MHI_TRE_CMD_START_PTR (0)
#define MHI_TRE_CMD_START_DWORD0 (0)
#define MHI_TRE_CMD_START_DWORD1(chid) (cpu_to_le32((chid << 24) | \
					(MHI_CMD_START_CHAN << 16)))

#define MHI_TRE_GET_DWORD(tre, word) (le32_to_cpu((tre)->dword[(word)]))
#define MHI_TRE_GET_CMD_CHID(tre) ((MHI_TRE_GET_DWORD(tre, 1) >> 24) & 0xFF)
#define MHI_TRE_GET_CMD_TYPE(tre) ((MHI_TRE_GET_DWORD(tre, 1) >> 16) & 0xFF)

/* Event descriptor macros */
#define MHI_TRE_EV_PTR(ptr) (cpu_to_le64(ptr))
#define MHI_TRE_EV_DWORD0(code, len) (cpu_to_le32((code << 24) | len))
#define MHI_TRE_EV_DWORD1(chid, type) (cpu_to_le32((chid << 24) | (type << 16)))
#define MHI_TRE_GET_EV_PTR(tre) (le64_to_cpu((tre)->ptr))
#define MHI_TRE_GET_EV_CODE(tre) ((MHI_TRE_GET_DWORD(tre, 0) >> 24) & 0xFF)
#define MHI_TRE_GET_EV_LEN(tre) (MHI_TRE_GET_DWORD(tre, 0) & 0xFFFF)
#define MHI_TRE_GET_EV_CHID(tre) ((MHI_TRE_GET_DWORD(tre, 1) >> 24) & 0xFF)
#define MHI_TRE_GET_EV_TYPE(tre) ((MHI_TRE_GET_DWORD(tre, 1) >> 16) & 0xFF)
#define MHI_TRE_GET_EV_STATE(tre) ((MHI_TRE_GET_DWORD(tre, 0) >> 24) & 0xFF)
#define MHI_TRE_GET_EV_EXECENV(tre) ((MHI_TRE_GET_DWORD(tre, 0) >> 24) & 0xFF)
#define MHI_TRE_GET_EV_SEQ(tre) MHI_TRE_GET_DWORD(tre, 0)
#define MHI_TRE_GET_EV_TIME(tre) (MHI_TRE_GET_EV_PTR(tre))
#define MHI_TRE_GET_EV_COOKIE(tre) lower_32_bits(MHI_TRE_GET_EV_PTR(tre))
#define MHI_TRE_GET_EV_VEID(tre) ((MHI_TRE_GET_DWORD(tre, 0) >> 16) & 0xFF)
#define MHI_TRE_GET_EV_LINKSPEED(tre) ((MHI_TRE_GET_DWORD(tre, 1) >> 24) & 0xFF)
#define MHI_TRE_GET_EV_LINKWIDTH(tre) (MHI_TRE_GET_DWORD(tre, 0) & 0xFF)

/* Transfer descriptor macros */
#define MHI_TRE_DATA_PTR(ptr) (cpu_to_le64(ptr))
#define MHI_TRE_DATA_DWORD0(len) (cpu_to_le32(len & MHI_MAX_MTU))
#define MHI_TRE_DATA_DWORD1(bei, ieot, ieob, chain) (cpu_to_le32((2 << 16) | (bei << 10) \
	| (ieot << 9) | (ieob << 8) | chain))

/* RSC transfer descriptor macros */
#define MHI_RSCTRE_DATA_PTR(ptr, len) (cpu_to_le64(((u64)len << 48) | ptr))
#define MHI_RSCTRE_DATA_DWORD0(cookie) (cpu_to_le32(cookie))
#define MHI_RSCTRE_DATA_DWORD1 (cpu_to_le32(MHI_PKT_TYPE_COALESCING << 16))

enum mhi_pkt_type {
	MHI_PKT_TYPE_INVALID = 0x0,
	MHI_PKT_TYPE_NOOP_CMD = 0x1,
	MHI_PKT_TYPE_TRANSFER = 0x2,
	MHI_PKT_TYPE_COALESCING = 0x8,
	MHI_PKT_TYPE_RESET_CHAN_CMD = 0x10,
	MHI_PKT_TYPE_STOP_CHAN_CMD = 0x11,
	MHI_PKT_TYPE_START_CHAN_CMD = 0x12,
	MHI_PKT_TYPE_STATE_CHANGE_EVENT = 0x20,
	MHI_PKT_TYPE_CMD_COMPLETION_EVENT = 0x21,
	MHI_PKT_TYPE_TX_EVENT = 0x22,
	MHI_PKT_TYPE_RSC_TX_EVENT = 0x28,
	MHI_PKT_TYPE_EE_EVENT = 0x40,
	MHI_PKT_TYPE_TSYNC_EVENT = 0x48,
	MHI_PKT_TYPE_BW_REQ_EVENT = 0x50,
	MHI_PKT_TYPE_STALE_EVENT, /* internal event */
};

/* MHI transfer completion events */
enum mhi_ev_ccs {
	MHI_EV_CC_INVALID = 0x0,
	MHI_EV_CC_SUCCESS = 0x1,
	MHI_EV_CC_EOT = 0x2, /* End of transfer event */
	MHI_EV_CC_OVERFLOW = 0x3,
	MHI_EV_CC_EOB = 0x4, /* End of block event */
	MHI_EV_CC_OOB = 0x5, /* Out of block event */
	MHI_EV_CC_DB_MODE = 0x6,
	MHI_EV_CC_UNDEFINED_ERR = 0x10,
	MHI_EV_CC_BAD_TRE = 0x11,
};

/* Channel state */
enum mhi_ch_state {
	MHI_CH_STATE_DISABLED,
	MHI_CH_STATE_ENABLED,
	MHI_CH_STATE_RUNNING,
	MHI_CH_STATE_SUSPENDED,
	MHI_CH_STATE_STOP,
	MHI_CH_STATE_ERROR,
};

enum mhi_cmd_type {
	MHI_CMD_NOP = 1,
	MHI_CMD_RESET_CHAN = 16,
	MHI_CMD_STOP_CHAN = 17,
	MHI_CMD_START_CHAN = 18,
};

#define EV_CTX_RESERVED_MASK GENMASK(7, 0)
#define EV_CTX_INTMODC_MASK GENMASK(15, 8)
#define EV_CTX_INTMODC_SHIFT 8
#define EV_CTX_INTMODT_MASK GENMASK(31, 16)
#define EV_CTX_INTMODT_SHIFT 16
struct mhi_event_ctxt {
	__le32 intmod;
	__le32 ertype;
	__le32 msivec;

	__le64 rbase __packed __aligned(4);
	__le64 rlen __packed __aligned(4);
	__le64 rp __packed __aligned(4);
	__le64 wp __packed __aligned(4);
};

#define CHAN_CTX_CHSTATE_MASK GENMASK(7, 0)
#define CHAN_CTX_CHSTATE_SHIFT 0
#define CHAN_CTX_BRSTMODE_MASK GENMASK(9, 8)
#define CHAN_CTX_BRSTMODE_SHIFT 8
#define CHAN_CTX_POLLCFG_MASK GENMASK(15, 10)
#define CHAN_CTX_POLLCFG_SHIFT 10
#define CHAN_CTX_RESERVED_MASK GENMASK(31, 16)
struct mhi_chan_ctxt {
	__le32 chcfg;
	__le32 chtype;
	__le32 erindex;

	__le64 rbase __packed __aligned(4);
	__le64 rlen __packed __aligned(4);
	__le64 rp __packed __aligned(4);
	__le64 wp __packed __aligned(4);
};

struct mhi_cmd_ctxt {
	__le32 reserved0;
	__le32 reserved1;
	__le32 reserved2;

	__le64 rbase __packed __aligned(4);
	__le64 rlen __packed __aligned(4);
	__le64 rp __packed __aligned(4);
	__le64 wp __packed __aligned(4);
};

extern const char * const mhi_state_str[MHI_STATE_MAX];
#define TO_MHI_STATE_STR(state) ((state >= MHI_STATE_MAX || \
				  !mhi_state_str[state]) ? \
				"INVALID_STATE" : mhi_state_str[state])

#endif /* _MHI_COMMON_H */
