/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021, Linaro Ltd.
 *
 */

#ifndef _MHI_COMMON_H
#define _MHI_COMMON_H

#include <linux/mhi.h>

/* MHI registers */
#define REG_MHIREGLEN					0x00
#define REG_MHIVER					0x08
#define REG_MHICFG					0x10
#define REG_CHDBOFF					0x18
#define REG_ERDBOFF					0x20
#define REG_BHIOFF					0x28
#define REG_BHIEOFF					0x2c
#define REG_DEBUGOFF					0x30
#define REG_MHICTRL					0x38
#define REG_MHISTATUS					0x48
#define REG_CCABAP_LOWER				0x58
#define REG_CCABAP_HIGHER				0x5c
#define REG_ECABAP_LOWER				0x60
#define REG_ECABAP_HIGHER				0x64
#define REG_CRCBAP_LOWER				0x68
#define REG_CRCBAP_HIGHER				0x6c
#define REG_CRDB_LOWER					0x70
#define REG_CRDB_HIGHER					0x74
#define REG_MHICTRLBASE_LOWER				0x80
#define REG_MHICTRLBASE_HIGHER				0x84
#define REG_MHICTRLLIMIT_LOWER				0x88
#define REG_MHICTRLLIMIT_HIGHER				0x8c
#define REG_MHIDATABASE_LOWER				0x98
#define REG_MHIDATABASE_HIGHER				0x9c
#define REG_MHIDATALIMIT_LOWER				0xa0
#define REG_MHIDATALIMIT_HIGHER				0xa4

/* MHI BHI registers */
#define REG_BHI_BHIVERSION_MINOR			0x00
#define REG_BHI_BHIVERSION_MAJOR			0x04
#define REG_BHI_IMGADDR_LOW				0x08
#define REG_BHI_IMGADDR_HIGH				0x0c
#define REG_BHI_IMGSIZE					0x10
#define REG_BHI_RSVD1					0x14
#define REG_BHI_IMGTXDB					0x18
#define REG_BHI_RSVD2					0x1c
#define REG_BHI_INTVEC					0x20
#define REG_BHI_RSVD3					0x24
#define REG_BHI_EXECENV					0x28
#define REG_BHI_STATUS					0x2c
#define REG_BHI_ERRCODE					0x30
#define REG_BHI_ERRDBG1					0x34
#define REG_BHI_ERRDBG2					0x38
#define REG_BHI_ERRDBG3					0x3c
#define REG_BHI_SERIALNU				0x40
#define REG_BHI_SBLANTIROLLVER				0x44
#define REG_BHI_NUMSEG					0x48
#define REG_BHI_MSMHWID(n)				(0x4c + (0x4 * (n)))
#define REG_BHI_OEMPKHASH(n)				(0x64 + (0x4 * (n)))
#define REG_BHI_RSVD5					0xc4

/* BHI register bits */
#define BHI_TXDB_SEQNUM_BMSK				GENMASK(29, 0)
#define BHI_STATUS_MASK					GENMASK(31, 30)
#define BHI_STATUS_ERROR				0x03
#define BHI_STATUS_SUCCESS				0x02
#define BHI_STATUS_RESET				0x00

/* MHI BHIE registers */
#define REG_BHIE_MSMSOCID_OFFS				0x00
#define REG_BHIE_TXVECADDR_LOW_OFFS			0x2c
#define REG_BHIE_TXVECADDR_HIGH_OFFS			0x30
#define REG_BHIE_TXVECSIZE_OFFS				0x34
#define REG_BHIE_TXVECDB_OFFS				0x3c
#define REG_BHIE_TXVECSTATUS_OFFS			0x44
#define REG_BHIE_RXVECADDR_LOW_OFFS			0x60
#define REG_BHIE_RXVECADDR_HIGH_OFFS			0x64
#define REG_BHIE_RXVECSIZE_OFFS				0x68
#define REG_BHIE_RXVECDB_OFFS				0x70
#define REG_BHIE_RXVECSTATUS_OFFS			0x78

/* BHIE register bits */
#define BHIE_TXVECDB_SEQNUM_BMSK			GENMASK(29, 0)
#define BHIE_TXVECSTATUS_SEQNUM_BMSK			GENMASK(29, 0)
#define BHIE_TXVECSTATUS_STATUS_BMSK			GENMASK(31, 30)
#define BHIE_TXVECSTATUS_STATUS_RESET			0x00
#define BHIE_TXVECSTATUS_STATUS_XFER_COMPL		0x02
#define BHIE_TXVECSTATUS_STATUS_ERROR			0x03
#define BHIE_RXVECDB_SEQNUM_BMSK			GENMASK(29, 0)
#define BHIE_RXVECSTATUS_SEQNUM_BMSK			GENMASK(29, 0)
#define BHIE_RXVECSTATUS_STATUS_BMSK			GENMASK(31, 30)
#define BHIE_RXVECSTATUS_STATUS_RESET			0x00
#define BHIE_RXVECSTATUS_STATUS_XFER_COMPL		0x02
#define BHIE_RXVECSTATUS_STATUS_ERROR			0x03

/* MHI register bits */
#define MHIREGLEN_MHIREGLEN_MASK			GENMASK(31, 0)
#define MHIVER_MHIVER_MASK				GENMASK(31, 0)
#define MHICFG_NHWER_MASK				GENMASK(31, 24)
#define MHICFG_NER_MASK					GENMASK(23, 16)
#define MHICFG_NHWCH_MASK				GENMASK(15, 8)
#define MHICFG_NCH_MASK					GENMASK(7, 0)
#define CHDBOFF_CHDBOFF_MASK				GENMASK(31, 0)
#define ERDBOFF_ERDBOFF_MASK				GENMASK(31, 0)
#define BHIOFF_BHIOFF_MASK				GENMASK(31, 0)
#define BHIEOFF_BHIEOFF_MASK				GENMASK(31, 0)
#define DEBUGOFF_DEBUGOFF_MASK				GENMASK(31, 0)
#define MHICTRL_MHISTATE_MASK				GENMASK(15, 8)
#define MHICTRL_RESET_MASK				BIT(1)
#define MHISTATUS_MHISTATE_MASK				GENMASK(15, 8)
#define MHISTATUS_SYSERR_MASK				BIT(2)
#define MHISTATUS_READY_MASK				BIT(0)
#define CCABAP_LOWER_CCABAP_LOWER_MASK			GENMASK(31, 0)
#define CCABAP_HIGHER_CCABAP_HIGHER_MASK		GENMASK(31, 0)
#define ECABAP_LOWER_ECABAP_LOWER_MASK			GENMASK(31, 0)
#define ECABAP_HIGHER_ECABAP_HIGHER_MASK		GENMASK(31, 0)
#define CRCBAP_LOWER_CRCBAP_LOWER_MASK			GENMASK(31, 0)
#define CRCBAP_HIGHER_CRCBAP_HIGHER_MASK		GENMASK(31, 0)
#define CRDB_LOWER_CRDB_LOWER_MASK			GENMASK(31, 0)
#define CRDB_HIGHER_CRDB_HIGHER_MASK			GENMASK(31, 0)
#define MHICTRLBASE_LOWER_MHICTRLBASE_LOWER_MASK	GENMASK(31, 0)
#define MHICTRLBASE_HIGHER_MHICTRLBASE_HIGHER_MASK	GENMASK(31, 0)
#define MHICTRLLIMIT_LOWER_MHICTRLLIMIT_LOWER_MASK	GENMASK(31, 0)
#define MHICTRLLIMIT_HIGHER_MHICTRLLIMIT_HIGHER_MASK	GENMASK(31, 0)
#define MHIDATABASE_LOWER_MHIDATABASE_LOWER_MASK	GENMASK(31, 0)
#define MHIDATABASE_HIGHER_MHIDATABASE_HIGHER_MASK	GENMASK(31, 0)
#define MHIDATALIMIT_LOWER_MHIDATALIMIT_LOWER_MASK	GENMASK(31, 0)
#define MHIDATALIMIT_HIGHER_MHIDATALIMIT_HIGHER_MASK	GENMASK(31, 0)

/* Command Ring Element macros */
/* No operation command */
#define MHI_TRE_CMD_NOOP_PTR				0
#define MHI_TRE_CMD_NOOP_DWORD0				0
#define MHI_TRE_CMD_NOOP_DWORD1				cpu_to_le32(MHI_CMD_NOP << 16)

/* Channel reset command */
#define MHI_TRE_CMD_RESET_PTR				0
#define MHI_TRE_CMD_RESET_DWORD0			0
#define MHI_TRE_CMD_RESET_DWORD1(chid)			(cpu_to_le32((chid << 24) | \
							(MHI_CMD_RESET_CHAN << 16)))

/* Channel stop command */
#define MHI_TRE_CMD_STOP_PTR				0
#define MHI_TRE_CMD_STOP_DWORD0				0
#define MHI_TRE_CMD_STOP_DWORD1(chid)			(cpu_to_le32((chid << 24) | \
							(MHI_CMD_STOP_CHAN << 16)))

/* Channel start command */
#define MHI_TRE_CMD_START_PTR				0
#define MHI_TRE_CMD_START_DWORD0			0
#define MHI_TRE_CMD_START_DWORD1(chid)			(cpu_to_le32((chid << 24) | \
							(MHI_CMD_START_CHAN << 16)))

#define MHI_TRE_GET_DWORD(tre, word)			le32_to_cpu((tre)->dword[(word)])
#define MHI_TRE_GET_CMD_CHID(tre)			((MHI_TRE_GET_DWORD(tre, 1) >> 24) & 0xFF)
#define MHI_TRE_GET_CMD_TYPE(tre)			((MHI_TRE_GET_DWORD(tre, 1) >> 16) & 0xFF)

/* Event descriptor macros */
/* Transfer completion event */
#define MHI_TRE_EV_PTR(ptr)				cpu_to_le64(ptr)
#define MHI_TRE_EV_DWORD0(code, len)			cpu_to_le32((code << 24) | len)
#define MHI_TRE_EV_DWORD1(chid, type)			cpu_to_le32((chid << 24) | (type << 16))
#define MHI_TRE_GET_EV_PTR(tre)				le64_to_cpu((tre)->ptr)
#define MHI_TRE_GET_EV_CODE(tre)			((MHI_TRE_GET_DWORD(tre, 0) >> 24) & 0xFF)
#define MHI_TRE_GET_EV_LEN(tre)				(MHI_TRE_GET_DWORD(tre, 0) & 0xFFFF)
#define MHI_TRE_GET_EV_CHID(tre)			((MHI_TRE_GET_DWORD(tre, 1) >> 24) & 0xFF)
#define MHI_TRE_GET_EV_TYPE(tre)			((MHI_TRE_GET_DWORD(tre, 1) >> 16) & 0xFF)
#define MHI_TRE_GET_EV_STATE(tre)			((MHI_TRE_GET_DWORD(tre, 0) >> 24) & 0xFF)
#define MHI_TRE_GET_EV_EXECENV(tre)			((MHI_TRE_GET_DWORD(tre, 0) >> 24) & 0xFF)
#define MHI_TRE_GET_EV_SEQ(tre)				MHI_TRE_GET_DWORD(tre, 0)
#define MHI_TRE_GET_EV_TIME(tre)			MHI_TRE_GET_EV_PTR(tre)
#define MHI_TRE_GET_EV_COOKIE(tre)			lower_32_bits(MHI_TRE_GET_EV_PTR(tre))
#define MHI_TRE_GET_EV_VEID(tre)			((MHI_TRE_GET_DWORD(tre, 0) >> 16) & 0xFF)
#define MHI_TRE_GET_EV_LINKSPEED(tre)			((MHI_TRE_GET_DWORD(tre, 1) >> 24) & 0xFF)
#define MHI_TRE_GET_EV_LINKWIDTH(tre)			(MHI_TRE_GET_DWORD(tre, 0) & 0xFF)

/* State change event */
#define MHI_SC_EV_PTR					0
#define MHI_SC_EV_DWORD0(state)				cpu_to_le32(state << 24)
#define MHI_SC_EV_DWORD1(type)				cpu_to_le32(type << 16)

/* EE event */
#define MHI_EE_EV_PTR					0
#define MHI_EE_EV_DWORD0(ee)				cpu_to_le32(ee << 24)
#define MHI_EE_EV_DWORD1(type)				cpu_to_le32(type << 16)

/* Command Completion event */
#define MHI_CC_EV_PTR(ptr)				cpu_to_le64(ptr)
#define MHI_CC_EV_DWORD0(code)				cpu_to_le32(code << 24)
#define MHI_CC_EV_DWORD1(type)				cpu_to_le32(type << 16)

/* Transfer descriptor macros */
#define MHI_TRE_DATA_PTR(ptr)				cpu_to_le64(ptr)
#define MHI_TRE_DATA_DWORD0(len)			cpu_to_le32(len & MHI_MAX_MTU)
#define MHI_TRE_DATA_DWORD1(bei, ieot, ieob, chain)	(cpu_to_le32((2 << 16) | (bei << 10) \
							| (ieot << 9) | (ieob << 8) | chain))

/* RSC transfer descriptor macros */
#define MHI_RSCTRE_DATA_PTR(ptr, len)			cpu_to_le64(((u64)len << 48) | ptr)
#define MHI_RSCTRE_DATA_DWORD0(cookie)			cpu_to_le32(cookie)
#define MHI_RSCTRE_DATA_DWORD1				cpu_to_le32(MHI_PKT_TYPE_COALESCING << 16)

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
#define EV_CTX_INTMODT_MASK GENMASK(31, 16)
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
#define CHAN_CTX_BRSTMODE_MASK GENMASK(9, 8)
#define CHAN_CTX_POLLCFG_MASK GENMASK(15, 10)
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

static inline const char * const mhi_state_str(enum mhi_state state)
{
	switch(state) {
	case MHI_STATE_RESET:
		return "RESET";
	case MHI_STATE_READY:
		return "READY";
	case MHI_STATE_M0:
		return "M0";
	case MHI_STATE_M1:
		return "M1";
	case MHI_STATE_M2:
		return"M2";
	case MHI_STATE_M3:
		return"M3";
	case MHI_STATE_M3_FAST:
		return"M3 FAST";
	case MHI_STATE_BHI:
		return"BHI";
	case MHI_STATE_SYS_ERR:
		return "SYS ERROR";
	default:
		return "Unknown state";
	}
};

#endif /* _MHI_COMMON_H */
