#ifndef _MHI_EP_INTERNAL_
#define _MHI_EP_INTERNAL_

#include <linux/bitfield.h>
#include <linux/mhi.h>

extern struct bus_type mhi_ep_bus_type;

/* MHI register definition */
#define MHI_CTRL_INT_STATUS_A7		0x4
#define MHI_CTRL_INT_STATUS_A7_MSK	BIT(0)
#define MHI_CTRL_INT_STATUS_CRDB_MSK	BIT(1)
#define MHI_CHDB_INT_STATUS_A7_n(n)	(0x28 + 0x4 * (n))
#define MHI_ERDB_INT_STATUS_A7_n(n)	(0x38 + 0x4 * (n))

#define MHI_CTRL_INT_CLEAR_A7		0x4c
#define MHI_CTRL_INT_MMIO_WR_CLEAR	BIT(2)
#define MHI_CTRL_INT_CRDB_CLEAR		BIT(1)
#define MHI_CTRL_INT_CRDB_MHICTRL_CLEAR	BIT(0)

#define MHI_CHDB_INT_CLEAR_A7_n(n)	(0x70 + 0x4 * (n))
#define MHI_CHDB_INT_CLEAR_A7_n_CLEAR_ALL GENMASK(31, 0)
#define MHI_ERDB_INT_CLEAR_A7_n(n)	(0x80 + 0x4 * (n))
#define MHI_ERDB_INT_CLEAR_A7_n_CLEAR_ALL GENMASK(31, 0)

#define MHI_CTRL_INT_MASK_A7		0x94
#define MHI_CTRL_INT_MASK_A7_MASK_MASK	GENMASK(1, 0)
#define MHI_CTRL_MHICTRL_MASK		BIT(0)
#define MHI_CTRL_MHICTRL_SHFT		0
#define MHI_CTRL_CRDB_MASK		BIT(1)
#define MHI_CTRL_CRDB_SHFT		1

#define MHI_CHDB_INT_MASK_A7_n(n)	(0xb8 + 0x4 * (n))
#define MHI_CHDB_INT_MASK_A7_n_EN_ALL	GENMASK(31, 0) 
#define MHI_ERDB_INT_MASK_A7_n(n)	(0xc8 + 0x4 * (n))
#define MHI_ERDB_INT_MASK_A7_n_EN_ALL	GENMASK(31, 0) 

#define MHIREGLEN 0x100
#define MHIVER 	0x108

#define MHICFG 	0x110
#define MHICFG_NHWER_MASK		GENMASK(31, 24)
#define MHICFG_NER_MASK GENMASK(23, 16)
#define MHICFG_RESERVED_BITS15_8_MASK	GENMASK(15, 8)
#define MHICFG_NCH_MASK GENMASK(7, 0)

#define CHDBOFF 	0x118
#define ERDBOFF 	0x120
#define BHIOFF 	0x128
#define DEBUGOFF 0x130

#define MHICTRL 	0x138
#define MHICTRL_MHISTATE_MASK		GENMASK(15, 8)
#define MHICTRL_RESET_MASK		BIT(1)
#define MHICTRL_RESET_SHIFT		1

#define MHISTATUS 0x148
#define MHISTATUS_MHISTATE_MASK		GENMASK(15, 8)
#define MHISTATUS_MHISTATE_SHIFT	8
#define MHISTATUS_SYSERR_MASK		BIT(2)
#define MHISTATUS_SYSERR_SHIFT		2
#define MHISTATUS_READY_MASK		BIT(0)
#define MHISTATUS_READY_SHIFT		0

#define CCABAP_LOWER 0x158
#define CCABAP_HIGHER 0x15C
#define ECABAP_LOWER 0x160
#define ECABAP_HIGHER 0x164
#define CRCBAP_LOWER 0x168
#define CRCBAP_HIGHER 0x16C
#define CRDB_LOWER 0x170
#define CRDB_HIGHER 0x174
#define MHICTRLBASE_LOWER		0x180
#define MHICTRLBASE_HIGHER		0x184
#define MHICTRLLIMIT_LOWER		0x188
#define MHICTRLLIMIT_HIGHER		0x18C
#define MHIDATABASE_LOWER		0x198
#define MHIDATABASE_HIGHER		0x19C
#define MHIDATALIMIT_LOWER		0x1A0
#define MHIDATALIMIT_HIGHER		0x1A4
#define CHDB_LOWER_n(n) (0x400 + 0x8 * (n))
#define CHDB_HIGHER_n(n)		(0x404 + 0x8 * (n))
#define ERDB_LOWER_n(n) (0x800 + 0x8 * (n))
#define ERDB_HIGHER_n(n)		(0x804 + 0x8 * (n))
#define BHI_INTVEC 0x220
#define BHI_EXECENV 0x228
#define BHI_IMGTXDB 0x218

#define NR_OF_CMD_RINGS 1
#define NUM_EVENT_RINGS 128
#define NUM_HW_EVENT_RINGS		2
#define NUM_CHANNELS 128
#define HW_CHANNEL_BASE 100
#define NUM_HW_CHANNELS 15
#define HW_CHANNEL_END 110
#define MHI_ENV_VALUE 2
#define MHI_MASK_ROWS_CH_EV_DB		4
#define TRB_MAX_DATA_SIZE		8192
#define MHI_CTRL_STATE 100

/* Channel state */
enum mhi_ch_state {
	MHI_CH_STATE_DISABLED,
	MHI_CH_STATE_ENABLED,
	MHI_CH_STATE_RUNNING,
	MHI_CH_STATE_SUSPENDED,
	MHI_CH_STATE_STOP,
	MHI_CH_STATE_ERROR,
};

#define CHAN_CTX_CHSTATE_MASK GENMASK(7, 0)
#define CHAN_CTX_CHSTATE_SHIFT 0
#define CHAN_CTX_BRSTMODE_MASK GENMASK(9, 8)
#define CHAN_CTX_BRSTMODE_SHIFT 8
#define CHAN_CTX_POLLCFG_MASK GENMASK(15, 10)
#define CHAN_CTX_POLLCFG_SHIFT 10
#define CHAN_CTX_RESERVED_MASK GENMASK(31, 16)
struct mhi_chan_ctx {
	__u32 chcfg;
	__u32 chtype;
	__u32 erindex;

	__u64 rbase __packed __aligned(4);
	__u64 rlen __packed __aligned(4);
	__u64 rp __packed __aligned(4);
	__u64 wp __packed __aligned(4);
};

/* Event ring context type */
#define EV_CTX_RESERVED_MASK GENMASK(7, 0)
#define EV_CTX_INTMODC_MASK GENMASK(15, 8)
#define EV_CTX_INTMODC_SHIFT 8
#define EV_CTX_INTMODT_MASK GENMASK(31, 16)
#define EV_CTX_INTMODT_SHIFT 16
struct mhi_event_ctx {
	__u32 intmod;
	__u32 ertype;
	__u32 msivec;

	__u64 rbase __packed __aligned(4);
	__u64 rlen __packed __aligned(4);
	__u64 rp __packed __aligned(4);
	__u64 wp __packed __aligned(4);
};

/* Command context */
struct mhi_cmd_ctx {
	__u32 reserved0;
	__u32 reserved1;
	__u32 reserved2;

	__u64 rbase __packed __aligned(4);
	__u64 rlen __packed __aligned(4);
	__u64 rp __packed __aligned(4);
	__u64 wp __packed __aligned(4);
};

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

enum mhi_cmd_type {
	MHI_CMD_NOP = 1,
	MHI_CMD_RESET_CHAN = 16,
	MHI_CMD_STOP_CHAN = 17,
	MHI_CMD_START_CHAN = 18,
};

/* Command Ring Element macros */

/* No operation command */
#define MHI_EP_CRE_NOOP_PTR (0)
#define MHI_EP_CRE_NOOP_DWORD0 (0)
#define MHI_EP_CRE_NOOP_DWORD1 (MHI_CMD_NOP << 16)

/* Channel reset command */
#define MHI_EP_CRE_RESET_PTR (0)
#define MHI_EP_CRE_RESET_DWORD0 (0)
#define MHI_EP_CRE_RESET_DWORD1(chid) ((chid << 24) | \
 		(MHI_CMD_RESET_CHAN << 16))

/* Channel stop command */
#define MHI_EP_CRE_STOP_PTR (0)
#define MHI_EP_CRE_STOP_DWORD0 (0)
#define MHI_EP_CRE_STOP_DWORD1(chid) ((chid << 24) | \
 	       (MHI_CMD_STOP_CHAN << 16))

/* Channel start command */
#define MHI_EP_CRE_START_PTR (0)
#define MHI_EP_CRE_START_DWORD0 (0)
#define MHI_EP_CRE_START_DWORD1(chid) ((chid << 24) | \
 		(MHI_CMD_START_CHAN << 16))

#define MHI_EP_CRE_GET_CHID(cre) (((cre)->dword[1] >> 24) & 0xff)
#define MHI_EP_CRE_GET_TYPE(cre) (((cre)->dword[1] >> 16) & 0xff)

/* Event Ring Element macros */

/* Transfer completion event */
#define MHI_EP_ERE_TR_PTR 0
#define MHI_EP_ERE_TR_DWORD0(code, len) ((code << 24) | len)
#define MHI_EP_ERE_TR_DWORD1(chid, type) ((chid << 24) | (type << 16))

/* State change event */
#define MHI_EP_ERE_SC_PTR 0
#define MHI_EP_ERE_SC_DWORD0(state) (state << 24)
#define MHI_EP_ERE_SC_DWORD1(type) (type << 16)

/* EE event */
#define MHI_EP_ERE_EE_PTR 0
#define MHI_EP_ERE_EE_DWORD0(ee) (ee << 24)
#define MHI_EP_ERE_EE_DWORD1(type) (type << 16)

/* Command Completion event */
#define MHI_EP_ERE_CC_PTR(ptr) (ptr)
#define MHI_EP_ERE_CC_DWORD0(code) (code << 24)
#define MHI_EP_ERE_CC_DWORD1(type) (type << 16)

/* Transfer Ring Element macros */
#define MHI_EP_TRE_PTR(ptr) (ptr)
#define MHI_EP_TRE_DWORD0(len) (len & MHI_MAX_MTU)
#define MHI_EP_TRE_DWORD1(bei, ieot, ieob, chain) ((2 << 16) | (bei << 10) \
	| (ieot << 9) | (ieob << 8) | chain)
#define MHI_EP_TRE_GET_PTR(tre) ((tre)->ptr)
#define MHI_EP_TRE_GET_LEN(tre) ((tre)->dword[0] & 0xffff)
#define MHI_EP_TRE_GET_CHAIN(tre) FIELD_GET(BIT(0), (tre)->dword[1])
#define MHI_EP_TRE_GET_IEOB(tre) FIELD_GET(BIT(8), (tre)->dword[1])
#define MHI_EP_TRE_GET_IEOT(tre) FIELD_GET(BIT(9), (tre)->dword[1])
#define MHI_EP_TRE_GET_BEI(tre) FIELD_GET(BIT(10), (tre)->dword[1])

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

enum mhi_ep_execenv {
	MHI_EP_SBL_EE = 1,
	MHI_EP_AMSS_EE = 2,
	MHI_EP_UNRESERVED
};

struct mhi_ep_ring_element {
	u64 ptr;
	u32 dword[2];
};

/* Transfer ring element type */
union mhi_ep_ring_ctx {
	struct mhi_cmd_ctx cmd;
	struct mhi_event_ctx ev;
	struct mhi_chan_ctx ch;
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

extern const char * const mhi_ep_state_str[MHI_STATE_MAX];
#define TO_MHI_STATE_STR(state) ((state >= MHI_STATE_MAX || \
				  !mhi_ep_state_str[state]) ? \
				"INVALID_STATE" : mhi_ep_state_str[state])

extern const char * const mhi_ep_link_state_str[LINK_STATE_MAX];
#define TO_LINK_STATE_STR(state) ((state >= LINK_STATE_MAX || \
				  !mhi_ep_link_state_str[state]) ? \
				"INVALID_STATE" : mhi_ep_link_state_str[state])

struct mhi_ep_state_transition {
	struct list_head node;
	enum mhi_state state;
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

/* MHI Ring related functions */
int mhi_ep_process_cmd_ring(struct mhi_ep_ring *ring, struct mhi_ep_ring_element *el);
int mhi_ep_process_tre_ring(struct mhi_ep_ring *ring, struct mhi_ep_ring_element *el);
void mhi_ep_ring_init(struct mhi_ep_ring *ring, enum mhi_ep_ring_type type, u32 id);
void mhi_ep_ring_stop(struct mhi_ep_cntrl *mhi_cntrl, struct mhi_ep_ring *ring);
size_t mhi_ep_ring_addr2offset(struct mhi_ep_ring *ring, u64 ptr);
int mhi_ep_ring_start(struct mhi_ep_cntrl *mhi_cntrl, struct mhi_ep_ring *ring, union mhi_ep_ring_ctx *ctx);
int mhi_ep_process_ring(struct mhi_ep_ring *ring);
int mhi_ep_ring_add_element(struct mhi_ep_ring *ring, struct mhi_ep_ring_element *element,
			    int evt_offset);
void mhi_ep_ring_inc_index(struct mhi_ep_ring *ring);

/* MMIO related functions */
void mhi_ep_mmio_read(struct mhi_ep_cntrl *mhi_cntrl, u32 offset, u32 *regval);
void mhi_ep_mmio_write(struct mhi_ep_cntrl *mhi_cntrl, u32 offset, u32 val);
void mhi_ep_mmio_masked_write(struct mhi_ep_cntrl *mhi_cntrl, u32 offset,
        u32 mask, u32 shift, u32 val);
int mhi_ep_mmio_masked_read(struct mhi_ep_cntrl *dev, u32 offset,
      u32 mask, u32 shift, u32 *regval);
void mhi_ep_mmio_enable_ctrl_interrupt(struct mhi_ep_cntrl *mhi_cntrl);
void mhi_ep_mmio_disable_ctrl_interrupt(struct mhi_ep_cntrl *mhi_cntrl);
void mhi_ep_mmio_enable_cmdb_interrupt(struct mhi_ep_cntrl *mhi_cntrl);
void mhi_ep_mmio_disable_cmdb_interrupt(struct mhi_ep_cntrl *mhi_cntrl);
void mhi_ep_mmio_enable_chdb_a7(struct mhi_ep_cntrl *mhi_cntrl, u32 chdb_id);
void mhi_ep_mmio_disable_chdb_a7(struct mhi_ep_cntrl *mhi_cntrl, u32 chdb_id);
void mhi_ep_mmio_enable_chdb_interrupts(struct mhi_ep_cntrl *mhi_cntrl);
void mhi_ep_mmio_read_chdb_status_interrupts(struct mhi_ep_cntrl *mhi_cntrl);
void mhi_ep_mmio_mask_interrupts(struct mhi_ep_cntrl *mhi_cntrl);
void mhi_ep_mmio_get_chc_base(struct mhi_ep_cntrl *mhi_cntrl);
void mhi_ep_mmio_get_erc_base(struct mhi_ep_cntrl *mhi_cntrl);
void mhi_ep_mmio_get_crc_base(struct mhi_ep_cntrl *mhi_cntrl);
void mhi_ep_mmio_get_ch_db(struct mhi_ep_ring *ring, u64 *wr_offset);
void mhi_ep_mmio_get_er_db(struct mhi_ep_ring *ring, u64 *wr_offset);
void mhi_ep_mmio_get_cmd_db(struct mhi_ep_ring *ring, u64 *wr_offset);
void mhi_ep_mmio_set_env(struct mhi_ep_cntrl *mhi_cntrl, u32 value);
void mhi_ep_mmio_clear_reset(struct mhi_ep_cntrl *mhi_cntrl);
void mhi_ep_mmio_reset(struct mhi_ep_cntrl *mhi_cntrl);
void mhi_ep_mmio_get_mhi_state(struct mhi_ep_cntrl *mhi_cntrl, enum mhi_state *state,
			       bool *mhi_reset);
void mhi_ep_mmio_init(struct mhi_ep_cntrl *mhi_cntrl);
void mhi_ep_mmio_update_ner(struct mhi_ep_cntrl *mhi_cntrl);

/* MHI EP core functions */
int mhi_ep_send_state_change_event(struct mhi_ep_cntrl *mhi_cntrl,
 		enum mhi_state state);
int mhi_ep_send_ee_event(struct mhi_ep_cntrl *mhi_cntrl, enum mhi_ep_execenv exec_env);
bool mhi_ep_check_mhi_state(struct mhi_ep_cntrl *mhi_cntrl, enum mhi_state cur_mhi_state,
			    enum mhi_state mhi_state);
int mhi_ep_set_mhi_state(struct mhi_ep_cntrl *mhi_cntrl, enum mhi_state mhi_state);
int mhi_ep_set_m0_state(struct mhi_ep_cntrl *mhi_cntrl);
int mhi_ep_set_m3_state(struct mhi_ep_cntrl *mhi_cntrl);
int mhi_ep_set_ready_state(struct mhi_ep_cntrl *mhi_cntrl);
void mhi_ep_resume_channels(struct mhi_ep_cntrl *mhi_cntrl);
void mhi_ep_suspend_channels(struct mhi_ep_cntrl *mhi_cntrl);
void mhi_ep_handle_syserr(struct mhi_ep_cntrl *mhi_cntrl);
int mhi_ep_suspend(struct mhi_ep_cntrl *mhi_cntrl);
int mhi_ep_resume(struct mhi_ep_cntrl *mhi_cntrl);

#endif
