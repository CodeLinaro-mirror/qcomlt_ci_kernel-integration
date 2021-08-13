#ifndef _PCI_EPF_MHI_INTERNAL_
#define _PCI_EPF_MHI_INTERNAL_

#include <linux/types.h>
#include <linux/pci-epc.h>
#include <linux/pci-epf.h>

/* MHI register definition */
#define MHI_CTRL_INT_STATUS_A7				(0x0004)
#define MHI_CTRL_INT_STATUS_A7_STATUS_MASK		0xffffffff
#define MHI_CTRL_INT_STATUS_A7_STATUS_SHIFT		0x0

#define MHI_CHDB_INT_STATUS_A7_n(n)			(0x0028 + 0x4 * (n))
#define MHI_CHDB_INT_STATUS_A7_n_STATUS_MASK		0xffffffff
#define MHI_CHDB_INT_STATUS_A7_n_STATUS_SHIFT		0x0

#define MHI_ERDB_INT_STATUS_A7_n(n)			(0x0038 + 0x4 * (n))
#define MHI_ERDB_INT_STATUS_A7_n_STATUS_MASK		0xffffffff
#define MHI_ERDB_INT_STATUS_A7_n_STATUS_SHIFT		0x0

#define MHI_CTRL_INT_CLEAR_A7				(0x004C)
#define MHI_CTRL_INT_CLEAR_A7_CLEAR_MASK		0xffffffff
#define MHI_CTRL_INT_CLEAR_A7_CLEAR_SHIFT		0x0
#define MHI_CTRL_INT_MMIO_WR_CLEAR			BIT(2)
#define MHI_CTRL_INT_CRDB_CLEAR				BIT(1)
#define MHI_CTRL_INT_CRDB_MHICTRL_CLEAR			BIT(0)

#define MHI_CHDB_INT_CLEAR_A7_n(n)			(0x0070 + 0x4 * (n))
#define MHI_CHDB_INT_CLEAR_A7_n_CLEAR_MASK		0xffffffff
#define MHI_CHDB_INT_CLEAR_A7_n_CLEAR_SHIFT		0x0

#define MHI_ERDB_INT_CLEAR_A7_n(n)			(0x0080 + 0x4 * (n))
#define MHI_ERDB_INT_CLEAR_A7_n_CLEAR_MASK		0xffffffff
#define MHI_ERDB_INT_CLEAR_A7_n_CLEAR_SHIFT		0x0

#define MHI_CTRL_INT_MASK_A7				(0x0094)
#define MHI_CTRL_INT_MASK_A7_MASK_MASK			0x3
#define MHI_CTRL_INT_MASK_A7_MASK_SHIFT			0x0
#define MHI_CTRL_MHICTRL_MASK				BIT(0)
#define MHI_CTRL_MHICTRL_SHFT				0
#define MHI_CTRL_CRDB_MASK				BIT(1)
#define MHI_CTRL_CRDB_SHFT				1

#define MHI_CHDB_INT_MASK_A7_n(n)			(0x00B8 + 0x4 * (n))
#define MHI_CHDB_INT_MASK_A7_n_MASK_MASK		0xffffffff
#define MHI_CHDB_INT_MASK_A7_n_MASK_SHIFT		0x0

#define MHI_ERDB_INT_MASK_A7_n(n)			(0x00C8 + 0x4 * (n))
#define MHI_ERDB_INT_MASK_A7_n_MASK_MASK		0xffffffff
#define MHI_ERDB_INT_MASK_A7_n_MASK_SHIFT		0x0

#define MHIREGLEN					(0x0100)
#define MHIREGLEN_MHIREGLEN_MASK			0xffffffff
#define MHIREGLEN_MHIREGLEN_SHIFT			0x0

#define MHIVER						(0x0108)
#define MHIVER_MHIVER_MASK				0xffffffff
#define MHIVER_MHIVER_SHIFT				0x0

#define MHICFG						(0x0110)
#define MHICFG_NHWER_MASK			0xff000000
#define MHICFG_NHWER_SHIFT			0x18
#define MHICFG_NER_MASK					0xff0000
#define MHICFG_NER_SHIFT				0x10
#define MHICFG_RESERVED_BITS15_8_MASK			0xff00
#define MHICFG_RESERVED_BITS15_8_SHIFT			0x8
#define MHICFG_NCH_MASK					0xff
#define MHICFG_NCH_SHIFT				0x0

#define CHDBOFF						(0x0118)
#define CHDBOFF_CHDBOFF_MASK				0xffffffff
#define CHDBOFF_CHDBOFF_SHIFT				0x0

#define ERDBOFF						(0x0120)
#define ERDBOFF_ERDBOFF_MASK				0xffffffff
#define ERDBOFF_ERDBOFF_SHIFT				0x0

#define BHIOFF						(0x0128)
#define BHIOFF_BHIOFF_MASK				0xffffffff
#define BHIOFF_BHIOFF_SHIFT				0x0

#define DEBUGOFF					(0x0130)
#define DEBUGOFF_DEBUGOFF_MASK				0xffffffff
#define DEBUGOFF_DEBUGOFF_SHIFT				0x0

#define MHICTRL						(0x0138)
#define MHICTRL_MHISTATE_MASK				0x0000FF00
#define MHICTRL_MHISTATE_SHIFT				0x8
#define MHICTRL_RESET_MASK				0x2
#define MHICTRL_RESET_SHIFT				0x1

#define MHISTATUS					(0x0148)
#define MHISTATUS_MHISTATE_MASK				0x0000ff00
#define MHISTATUS_MHISTATE_SHIFT			0x8
#define MHISTATUS_SYSERR_MASK				0x4
#define MHISTATUS_SYSERR_SHIFT				0x2
#define MHISTATUS_READY_MASK				0x1
#define MHISTATUS_READY_SHIFT				0x0

#define CCABAP_LOWER					(0x0158)
#define CCABAP_LOWER_CCABAP_LOWER_MASK			0xffffffff
#define CCABAP_LOWER_CCABAP_LOWER_SHIFT			0x0

#define CCABAP_HIGHER					(0x015C)
#define CCABAP_HIGHER_CCABAP_HIGHER_MASK		0xffffffff
#define CCABAP_HIGHER_CCABAP_HIGHER_SHIFT		0x0

#define ECABAP_LOWER					(0x0160)
#define ECABAP_LOWER_ECABAP_LOWER_MASK			0xffffffff
#define ECABAP_LOWER_ECABAP_LOWER_SHIFT			0x0

#define ECABAP_HIGHER					(0x0164)
#define ECABAP_HIGHER_ECABAP_HIGHER_MASK		0xffffffff
#define ECABAP_HIGHER_ECABAP_HIGHER_SHIFT		0x0

#define CRCBAP_LOWER					(0x0168)
#define CRCBAP_LOWER_CRCBAP_LOWER_MASK			0xffffffff
#define CRCBAP_LOWER_CRCBAP_LOWER_SHIFT			0x0

#define CRCBAP_HIGHER					(0x016C)
#define CRCBAP_HIGHER_CRCBAP_HIGHER_MASK		0xffffffff
#define CRCBAP_HIGHER_CRCBAP_HIGHER_SHIFT		0x0

#define CRDB_LOWER					(0x0170)
#define CRDB_LOWER_CRDB_LOWER_MASK			0xffffffff
#define CRDB_LOWER_CRDB_LOWER_SHIFT			0x0

#define CRDB_HIGHER					(0x0174)
#define CRDB_HIGHER_CRDB_HIGHER_MASK			0xffffffff
#define CRDB_HIGHER_CRDB_HIGHER_SHIFT			0x0

#define MHICTRLBASE_LOWER				(0x0180)
#define MHICTRLBASE_LOWER_MHICTRLBASE_LOWER_MASK	0xffffffff
#define MHICTRLBASE_LOWER_MHICTRLBASE_LOWER_SHIFT	0x0

#define MHICTRLBASE_HIGHER				(0x0184)
#define MHICTRLBASE_HIGHER_MHICTRLBASE_HIGHER_MASK	0xffffffff
#define MHICTRLBASE_HIGHER_MHICTRLBASE_HIGHER_SHIFT	0x0

#define MHICTRLLIMIT_LOWER				(0x0188)
#define MHICTRLLIMIT_LOWER_MHICTRLLIMIT_LOWER_MASK	0xffffffff
#define MHICTRLLIMIT_LOWER_MHICTRLLIMIT_LOWER_SHIFT	0x0

#define MHICTRLLIMIT_HIGHER				(0x018C)
#define MHICTRLLIMIT_HIGHER_MHICTRLLIMIT_HIGHER_MASK	0xffffffff
#define MHICTRLLIMIT_HIGHER_MHICTRLLIMIT_HIGHER_SHIFT	0x0

#define MHIDATABASE_LOWER				(0x0198)
#define MHIDATABASE_LOWER_MHIDATABASE_LOWER_MASK	0xffffffff
#define MHIDATABASE_LOWER_MHIDATABASE_LOWER_SHIFT	0x0

#define MHIDATABASE_HIGHER				(0x019C)
#define MHIDATABASE_HIGHER_MHIDATABASE_HIGHER_MASK	0xffffffff
#define MHIDATABASE_HIGHER_MHIDATABASE_HIGHER_SHIFT	0x0

#define MHIDATALIMIT_LOWER				(0x01A0)
#define MHIDATALIMIT_LOWER_MHIDATALIMIT_LOWER_MASK	0xffffffff
#define MHIDATALIMIT_LOWER_MHIDATALIMIT_LOWER_SHIFT	0x0

#define MHIDATALIMIT_HIGHER				(0x01A4)
#define MHIDATALIMIT_HIGHER_MHIDATALIMIT_HIGHER_MASK	0xffffffff
#define MHIDATALIMIT_HIGHER_MHIDATALIMIT_HIGHER_SHIFT	0x0

#define CHDB_LOWER_n(n)					(0x0400 + 0x8 * (n))
#define CHDB_LOWER_n_CHDB_LOWER_MASK			0xffffffff
#define CHDB_LOWER_n_CHDB_LOWER_SHIFT			0x0

#define CHDB_HIGHER_n(n)				(0x0404 + 0x8 * (n))
#define CHDB_HIGHER_n_CHDB_HIGHER_MASK			0xffffffff
#define CHDB_HIGHER_n_CHDB_HIGHER_SHIFT			0x0

#define ERDB_LOWER_n(n)					(0x0800 + 0x8 * (n))
#define ERDB_LOWER_n_ERDB_LOWER_MASK			0xffffffff
#define ERDB_LOWER_n_ERDB_LOWER_SHIFT			0x0

#define ERDB_HIGHER_n(n)				(0x0804 + 0x8 * (n))
#define ERDB_HIGHER_n_ERDB_HIGHER_MASK			0xffffffff
#define ERDB_HIGHER_n_ERDB_HIGHER_SHIFT			0x0

#define BHI_INTVEC					(0x220)
#define BHI_INTVEC_MASK					0xFFFFFFFF
#define BHI_INTVEC_SHIFT				0

#define BHI_EXECENV					(0x228)
#define BHI_EXECENV_MASK				0xFFFFFFFF
#define BHI_EXECENV_SHIFT				0

#define BHI_IMGTXDB					(0x218)

#define NUM_CHANNELS			128
#define HW_CHANNEL_BASE			100
#define NUM_HW_CHANNELS			15
#define HW_CHANNEL_END			110
#define MHI_ENV_VALUE			2
#define MHI_MASK_ROWS_CH_EV_DB		4
#define TRB_MAX_DATA_SIZE		8192
#define MHI_CTRL_STATE			100

enum pci_epf_mhi_ctrl_info {
	PCI_EPF_MHI_STATE_CONFIGURED,
	PCI_EPF_MHI_STATE_CONNECTED,
	PCI_EPF_MHI_STATE_DISCONNECTED,
	PCI_EPF_MHI_STATE_INVAL,
};

/* Channel context state */
enum pci_epf_mhi_ch_ctx_state {
	PCI_EPF_MHI_CH_STATE_DISABLED,
	PCI_EPF_MHI_CH_STATE_ENABLED,
	PCI_EPF_MHI_CH_STATE_RUNNING,
	PCI_EPF_MHI_CH_STATE_SUSPENDED,
	PCI_EPF_MHI_CH_STATE_STOP,
	PCI_EPF_MHI_CH_STATE_ERROR,
	PCI_EPF_MHI_CH_STATE_RESERVED,
	PCI_EPF_MHI_CH_STATE_32BIT = 0x7FFFFFFF
};

/* Channel type */
enum pci_epf_mhi_ch_ctx_type {
	PCI_EPF_MHI_CH_TYPE_NONE,
	PCI_EPF_MHI_CH_TYPE_OUTBOUND_CHANNEL,
	PCI_EPF_MHI_CH_TYPE_INBOUND_CHANNEL,
	PCI_EPF_MHI_CH_RESERVED
};

/* Channel context type */
struct pci_epf_mhi_ch_ctx {
	enum pci_epf_mhi_ch_ctx_state	ch_state;
	enum pci_epf_mhi_ch_ctx_type	ch_type;
	u32			err_indx;
	uint64_t			rbase;
	uint64_t			rlen;
	uint64_t			rp;
	uint64_t			wp;
} __packed;

enum pci_epf_mhi_ring_element_type_id {
	PCI_EPF_MHI_RING_EL_INVALID = 0,
	PCI_EPF_MHI_RING_EL_NOOP = 1,
	PCI_EPF_MHI_RING_EL_TRANSFER = 2,
	PCI_EPF_MHI_RING_EL_RESET = 16,
	PCI_EPF_MHI_RING_EL_STOP = 17,
	PCI_EPF_MHI_RING_EL_START = 18,
	PCI_EPF_MHI_RING_EL_MHI_STATE_CHG = 32,
	PCI_EPF_MHI_RING_EL_CMD_COMPLETION_EVT = 33,
	PCI_EPF_MHI_RING_EL_TRANSFER_COMPLETION_EVENT = 34,
	PCI_EPF_MHI_RING_EL_EE_STATE_CHANGE_NOTIFY = 64,
	PCI_EPF_MHI_RING_EL_UNDEF
};

enum pci_epf_mhi_ring_state {
	RING_STATE_UINT = 0,
	RING_STATE_IDLE,
	RING_STATE_PENDING,
};

enum pci_epf_mhi_ring_type {
	RING_TYPE_CMD = 0,
	RING_TYPE_ER,
	RING_TYPE_CH,
	RING_TYPE_INVAL
};

/* Event context interrupt moderation */
enum pci_epf_mhi_evt_ctx_int_mod_timer {
	PCI_EPF_MHI_EVT_INT_MODERATION_DISABLED
};

/* Event ring type */
enum pci_epf_mhi_evt_ctx_event_ring_type {
	PCI_EPF_MHI_EVT_TYPE_DEFAULT,
	PCI_EPF_MHI_EVT_TYPE_VALID,
	PCI_EPF_MHI_EVT_RESERVED
};

/* Event ring context type */
struct pci_epf_mhi_ev_ctx {
	u32				res1:16;
	enum pci_epf_mhi_evt_ctx_int_mod_timer	intmodt:16;
	enum pci_epf_mhi_evt_ctx_event_ring_type	ertype;
	u32				msivec;
	uint64_t				rbase;
	uint64_t				rlen;
	uint64_t				rp;
	uint64_t				wp;
} __packed;

/* Command context */
struct pci_epf_mhi_cmd_ctx {
	u32				res1;
	u32				res2;
	u32				res3;
	uint64_t				rbase;
	uint64_t				rlen;
	uint64_t				rp;
	uint64_t				wp;
} __packed;

/* generic context */
struct pci_epf_mhi_gen_ctx {
	u32				res1;
	u32				res2;
	u32				res3;
	uint64_t				rbase;
	uint64_t				rlen;
	uint64_t				rp;
	uint64_t				wp;
} __packed;

/* Transfer ring element */
struct pci_epf_mhi_transfer_ring_element {
	uint64_t				data_buf_ptr;
	u32				len:16;
	u32				res1:16;
	u32				chain:1;
	u32				res2:7;
	u32				ieob:1;
	u32				ieot:1;
	u32				bei:1;
	u32				res3:5;
	enum pci_epf_mhi_ring_element_type_id	type:8;
	u32				res4:8;
} __packed;

/* Command ring element */
/* Command ring No op command */
struct pci_epf_mhi_cmd_ring_op {
	uint64_t				res1;
	u32				res2;
	u32				res3:16;
	enum pci_epf_mhi_ring_element_type_id	type:8;
	u32				chid:8;
} __packed;

/* Command ring reset channel command */
struct pci_epf_mhi_cmd_ring_reset_channel_cmd {
	uint64_t				res1;
	u32				res2;
	u32				res3:16;
	enum pci_epf_mhi_ring_element_type_id	type:8;
	u32				chid:8;
} __packed;

/* Command ring stop channel command */
struct pci_epf_mhi_cmd_ring_stop_channel_cmd {
	uint64_t				res1;
	u32				res2;
	u32				res3:16;
	enum pci_epf_mhi_ring_element_type_id	type:8;
	u32				chid:8;
} __packed;

/* Command ring start channel command */
struct pci_epf_mhi_cmd_ring_start_channel_cmd {
	uint64_t				res1;
	u32				seqnum;
	u32				reliable:1;
	u32				res2:15;
	enum pci_epf_mhi_ring_element_type_id	type:8;
	u32				chid:8;
} __packed;

enum pci_epf_mhi_cmd_completion_code {
	MHI_CMD_COMPL_CODE_INVALID = 0,
	MHI_CMD_COMPL_CODE_SUCCESS = 1,
	MHI_CMD_COMPL_CODE_EOT = 2,
	MHI_CMD_COMPL_CODE_OVERFLOW = 3,
	MHI_CMD_COMPL_CODE_EOB = 4,
	MHI_CMD_COMPL_CODE_UNDEFINED = 16,
	MHI_CMD_COMPL_CODE_RING_EL = 17,
	MHI_CMD_COMPL_CODE_RES
};

/* Event ring elements */
/* Transfer completion event */
struct pci_epf_mhi_event_ring_transfer_completion {
	uint64_t				ptr;
	u32				len:16;
	u32				res1:8;
	enum pci_epf_mhi_cmd_completion_code	code:8;
	u32				res2:16;
	enum pci_epf_mhi_ring_element_type_id	type:8;
	u32				chid:8;
} __packed;

/* Command completion event */
struct pci_epf_mhi_event_ring_cmd_completion {
	uint64_t				ptr;
	u32				res1:24;
	enum pci_epf_mhi_cmd_completion_code	code:8;
	u32				res2:16;
	enum pci_epf_mhi_ring_element_type_id	type:8;
	u32				res3:8;
} __packed;

/**
 * enum pci_epf_mhi_event - MHI state change events
 * @PCI_EPF_MHI_EVENT_CTRL_TRIG: CTRL register change event.
 *				Not supported,for future use
 * @PCI_EPF_MHI_EVENT_M0_STATE: M0 state change event
 * @PCI_EPF_MHI_EVENT_M1_STATE: M1 state change event. Not supported, for future use
 * @PCI_EPF_MHI_EVENT_M2_STATE: M2 state change event. Not supported, for future use
 * @PCI_EPF_MHI_EVENT_M3_STATE: M0 state change event
 * @PCI_EPF_MHI_EVENT_HW_ACC_WAKEUP: pendding data on IPA, initiate Host wakeup
 * @PCI_EPF_MHI_EVENT_CORE_WAKEUP: MHI core initiate Host wakup
 */
enum pci_epf_mhi_event {
	PCI_EPF_MHI_EVENT_CTRL_TRIG,
	PCI_EPF_MHI_EVENT_M0_STATE,
	PCI_EPF_MHI_EVENT_M1_STATE,
	PCI_EPF_MHI_EVENT_M2_STATE,
	PCI_EPF_MHI_EVENT_M3_STATE,
	PCI_EPF_MHI_EVENT_HW_ACC_WAKEUP,
	PCI_EPF_MHI_EVENT_CORE_WAKEUP,
	PCI_EPF_MHI_EVENT_MAX
};

enum pci_epf_mhi_state {
	PCI_EPF_MHI_RESET_STATE = 0,
	PCI_EPF_MHI_READY_STATE,
	PCI_EPF_MHI_M0_STATE,
	PCI_EPF_MHI_M1_STATE,
	PCI_EPF_MHI_M2_STATE,
	PCI_EPF_MHI_M3_STATE,
	PCI_EPF_MHI_MAX_STATE,
	PCI_EPF_MHI_SYSERR_STATE = 0xff
};

/* MHI state change event */
struct pci_epf_mhi_event_ring_state_change {
	uint64_t				ptr;
	u32				res1:24;
	enum pci_epf_mhi_state			mhistate:8;
	u32				res2:16;
	enum pci_epf_mhi_ring_element_type_id	type:8;
	u32				res3:8;
} __packed;

enum pci_epf_mhi_execenv {
	PCI_EPF_MHI_SBL_EE = 1,
	PCI_EPF_MHI_AMSS_EE = 2,
	PCI_EPF_MHI_UNRESERVED
};

/* EE state change event */
struct pci_epf_mhi_event_ring_ee_state_change {
	uint64_t				ptr;
	u32				res1:24;
	enum pci_epf_mhi_execenv			execenv:8;
	u32				res2:16;
	enum pci_epf_mhi_ring_element_type_id	type:8;
	u32				res3:8;
} __packed;

/* Generic cmd to parse common details like type and channel id */
struct pci_epf_mhi_ring_generic {
	uint64_t				ptr;
	u32				res1:24;
	enum pci_epf_mhi_state			mhistate:8;
	u32				res2:16;
	enum pci_epf_mhi_ring_element_type_id	type:8;
	u32				chid:8;
} __packed;

struct pci_epf_mhi_config {
	u32	mhi_reg_len;
	u32	version;
	u32	event_rings;
	u32	hw_event_rings;
	u32	channels;
	u32	chdb_offset;
	u32	erdb_offset;
};

/* Possible ring element types */
union pci_epf_mhi_ring_element_type {
	struct pci_epf_mhi_cmd_ring_op			cmd_no_op;
	struct pci_epf_mhi_cmd_ring_reset_channel_cmd	cmd_reset;
	struct pci_epf_mhi_cmd_ring_stop_channel_cmd	cmd_stop;
	struct pci_epf_mhi_cmd_ring_start_channel_cmd	cmd_start;
	struct pci_epf_mhi_transfer_ring_element		tre;
	struct pci_epf_mhi_event_ring_transfer_completion	evt_tr_comp;
	struct pci_epf_mhi_event_ring_cmd_completion	evt_cmd_comp;
	struct pci_epf_mhi_event_ring_state_change		evt_state_change;
	struct pci_epf_mhi_event_ring_ee_state_change	evt_ee_state;
	struct pci_epf_mhi_ring_generic			generic;
};

/* Transfer ring element type */
union pci_epf_mhi_ring_ctx {
	struct pci_epf_mhi_cmd_ctx		cmd;
	struct pci_epf_mhi_ev_ctx		ev;
	struct pci_epf_mhi_ch_ctx		ch;
	struct pci_epf_mhi_gen_ctx		generic;
};

/* MHI host Control and data address region */
struct pci_epf_mhi_host_addr {
	u32	ctrl_base_lsb;
	u32	ctrl_base_msb;
	u32	ctrl_limit_lsb;
	u32	ctrl_limit_msb;
	u32	data_base_lsb;
	u32	data_base_msb;
	u32	data_limit_lsb;
	u32	data_limit_msb;
};

/* MHI physical and virtual address region */
struct mhi_meminfo {
	struct device	*dev;
	uintptr_t	pa_aligned;
	uintptr_t	pa_unaligned;
	uintptr_t	va_aligned;
	uintptr_t	va_unaligned;
	uintptr_t	size;
};

struct pci_epf_mhi_addr {
	uint64_t	host_pa;
	size_t	device_pa;
	size_t	device_va;
	size_t		size;
	dma_addr_t	phy_addr;
	void		*virt_addr;
	bool		use_ipa_dma;
};

struct mhi_interrupt_state {
	u32	mask;
	u32	status;
};

enum pci_epf_mhi_channel_state {
	PCI_EPF_MHI_CH_UNINT,
	PCI_EPF_MHI_CH_STARTED,
	PCI_EPF_MHI_CH_PENDING_START,
	PCI_EPF_MHI_CH_PENDING_STOP,
	PCI_EPF_MHI_CH_STOPPED,
	PCI_EPF_MHI_CH_CLOSED,
};

enum pci_epf_mhi_ch_operation {
	PCI_EPF_MHI_OPEN_CH,
	PCI_EPF_MHI_CLOSE_CH,
	PCI_EPF_MHI_READ_CH,
	PCI_EPF_MHI_READ_WR,
	PCI_EPF_MHI_POLL,
};

enum pci_epf_mhi_tr_compl_evt_type {
	SEND_EVENT_BUFFER,
	SEND_EVENT_RD_OFFSET,
	SEND_MSI
};

enum pci_epf_mhi_transfer_type {
	PCI_EPF_MHI_DMA_SYNC,
	PCI_EPF_MHI_DMA_ASYNC,
};

struct pci_epf_mhi_channel;

struct pci_epf_mhi_ring {
	struct list_head			list;
	struct pci_epf_mhi				*epf_mhi;

	u32				id;
	size_t				rd_offset;
	size_t				wr_offset;
	size_t				ring_size;

	enum pci_epf_mhi_ring_type			type;
	enum pci_epf_mhi_ring_state			state;
	/*
	 * Lock to prevent race in updating event ring
	 * which is shared by multiple channels
	 */
	struct mutex	event_lock;
	/* device virtual address location of the cached host ring ctx data */
	union pci_epf_mhi_ring_element_type		*ring_cache;
	/* Physical address of the cached ring copy on the device side */
	dma_addr_t				ring_cache_dma_handle;
	/* Device VA of read pointer array (used only for event rings) */
	uint64_t			*evt_rp_cache;
	/* PA of the read pointer array (used only for event rings) */
	dma_addr_t				evt_rp_cache_dma_handle;
	/* Device VA of msi buffer (used only for event rings)  */
	u32			*msi_buf;
	/* PA of msi buf (used only for event rings) */
	dma_addr_t				msi_buf_dma_handle;
	/* Physical address of the host where we will write/read to/from */
	struct pci_epf_mhi_addr				ring_shadow;
	/* Ring type - cmd, event, transfer ring and its rp/wp... */
	union pci_epf_mhi_ring_ctx			*ring_ctx;
	/* ring_ctx_shadow -> tracking ring_ctx in the host */
	union pci_epf_mhi_ring_ctx			*ring_ctx_shadow;
	void (*ring_cb)(struct pci_epf_mhi *dev,
			union pci_epf_mhi_ring_element_type *el,
			void *ctx);
};

/* trace information planned to use for read/write */
#define TRACE_DATA_MAX				128
#define PCI_EPF_MHI_DATA_MAX			512

#define PCI_EPF_MHI_MMIO_RANGE			0xb80
#define PCI_EPF_MHI_MMIO_OFFSET			0x100

struct ring_cache_req {
	struct completion	*done;
	void			*context;
};

struct event_req {
	union pci_epf_mhi_ring_element_type *tr_events;
	/*
	 * Start index of the completion event buffer segment
	 * to be flushed to host
	 */
	u32			start;
	u32			num_events;
	dma_addr_t		dma;
	u32			dma_len;
	dma_addr_t		event_rd_dma;
	void			*context;
	enum pci_epf_mhi_tr_compl_evt_type event_type;
	u32			event_ring;
	void			(*client_cb)(void *req);
	void			(*rd_offset_cb)(void *req);
	void			(*msi_cb)(void *req);
	struct list_head	list;
	u32			flush_num;
};

struct pci_epf_mhi_channel {
	struct list_head		list;
	struct list_head		clients;
	/* synchronization for changing channel state,
	 * adding/removing clients, pci_epf_mhi callbacks, etc
	 */
	struct pci_epf_mhi_ring		*ring;

	enum pci_epf_mhi_channel_state	state;
	u32			ch_id;
	enum pci_epf_mhi_ch_ctx_type	ch_type;
	struct mutex			ch_lock;
	/* Pointer to completion event buffer */
	union pci_epf_mhi_ring_element_type *tr_events;
	/* Indices for completion event buffer */
	u32			evt_buf_rp;
	u32			evt_buf_wp;
	u32			evt_buf_size;
	/*
	 * Pointer to a block of event request structs used to temporarily
	 * store completion events and meta data before sending them to host
	 */
	struct event_req		*ereqs;
	/* Linked list head for event request structs */
	struct list_head		event_req_buffers;
	u32				evt_req_size;
	/* Linked list head for event request structs to be flushed */
	struct list_head		flush_event_req_buffers;
	/* Pointer to the currently used event request struct */
	struct event_req		*curr_ereq;
	/* current TRE being processed */
	uint64_t			tre_loc;
	/* current TRE size */
	u32			tre_size;
	/* tre bytes left to read/write */
	u32			tre_bytes_left;
	/* td size being read/written from/to so far */
	u32			td_size;
	u32			pend_wr_count;
	u32			msi_cnt;
	u32			flush_req_cnt;
	bool				skip_td;
};

enum pci_epf_mhi_sm_pcie_state {
	PCI_EPF_MHI_SM_LINK_DISABLE,
	PCI_EPF_MHI_SM_D0_STATE,
	PCI_EPF_MHI_SM_D3_HOT_STATE,
	PCI_EPF_MHI_SM_D3_COLD_STATE,
};

/**
 * struct pci_epf_mhi_sm - MHI state manager context information
 * @state: MHI M state of the MHI device
 * @d_state: EP-PCIe D state of the MHI device
 * @lock: mutex for mhi_state
 * @syserr_occurred:flag to indicate if a syserr condition has occurred.
 * @wq: workqueue for state change events
 * @pending_device_events: number of pending mhi state change events in sm_wq
 * @pending_pcie_events: number of pending mhi state change events in sm_wq
 */
struct pci_epf_mhi_sm {
	enum pci_epf_mhi_state state;
	enum pci_epf_mhi_sm_pcie_state d_state;
	struct mutex lock;
	bool syserr_occurred;
	struct workqueue_struct *wq;
	atomic_t pending_device_events;
	atomic_t pending_pcie_events;
};

struct pci_epf_mhi {
	struct pci_epf *epf;
	struct pci_epf_mhi_config cfg;
	struct pci_epf_mhi_ring *ring;
	struct pci_epf_mhi_channel *ch;
	struct pci_epf_mhi_sm *sm;

	/* Host control base information */
	struct pci_epf_mhi_host_addr		host_addr;
	struct pci_epf_mhi_addr			ctrl_base;
	struct pci_epf_mhi_addr			data_base;
	struct pci_epf_mhi_addr			ch_ctx_shadow;
	struct pci_epf_mhi_ch_ctx		*ch_ctx_cache;
	phys_addr_t			ch_ctx_cache_dma_handle;
	struct pci_epf_mhi_addr			ev_ctx_shadow;
	struct pci_epf_mhi_ev_ctx		*ev_ctx_cache;
	phys_addr_t			ev_ctx_cache_dma_handle;

	struct pci_epf_mhi_addr			cmd_ctx_shadow;
	struct pci_epf_mhi_cmd_ctx		*cmd_ctx_cache;
	phys_addr_t			cmd_ctx_cache_dma_handle;

	struct workqueue_struct *init_wq;
	struct work_struct init_work;
	void *dma_cache;
	void *read_handle;
	void *write_handle;
	/* Physical scratch buffer for writing control data to the host */
	dma_addr_t cache_dma_handle;
	/*
	 * Physical scratch buffer address used when picking host data
	 * from the host used in mhi_read()
	 */
	dma_addr_t read_dma_handle;
	/*
	 * Physical scratch buffer address used when writing to the host
	 * region from device used in mhi_write()
	 */
	dma_addr_t write_dma_handle;

	enum pci_epf_mhi_ctrl_info ctrl_info;
	phys_addr_t mmio_base_pa_addr;
	void *mmio_base_addr;
	void __iomem *mmio;
	resource_size_t mmio_phys;
	u32 mmio_size;
	int irq;
	bool mmio_initialized;

	spinlock_t lock;
	u32 *mmio_backup;
        int ctrl_int;
        int cmd_int;

	size_t cmd_ring_idx;
	size_t ev_ring_start;
	size_t ch_ring_start;

        /* CHDB and EVDB device interrupt state */
        struct mhi_interrupt_state      chdb[4];
        struct mhi_interrupt_state      evdb[4];
};

/* MHI Ring related functions */

/**
 * pci_epf_mhi_ring_init() - Initializes the Ring id to the default un-initialized
 *		state. Once a start command is received, the respective ring
 *		is then prepared by fetching the context and updating the
 *		offset.
 * @ring:	Ring for the respective context - Channel/Event/Command.
 * @type:	Command/Event or Channel transfer ring.
 * @id:		Index to the ring id. For command its usually 1, Event rings
 *		may vary from 1 to 128. Channels vary from 1 to 256.
 */
void pci_epf_mhi_ring_init(struct pci_epf_mhi_ring *ring,
			enum pci_epf_mhi_ring_type type, int id);

/**
 * pci_epf_mhi_ring_start() - Fetches the respective transfer ring's context from
 *		the host and updates the write offset.
 * @ring:	Ring for the respective context - Channel/Event/Command.
 * @ctx:	Transfer ring of type pci_epf_mhi_ring_ctx.
 * @dev:	MHI device structure.
 */
int pci_epf_mhi_ring_start(struct pci_epf_mhi_ring *ring,
			union pci_epf_mhi_ring_ctx *ctx, struct pci_epf_mhi *mhi);

/**
 * pci_epf_mhi_cache_ring() - Cache the data for the corresponding ring locally.
 * @ring:	Ring for the respective context - Channel/Event/Command.
 * @wr_offset:	Cache the TRE's upto the write offset value.
 */
int pci_epf_mhi_cache_ring(struct pci_epf_mhi_ring *ring, size_t wr_offset);

/**
 * pci_epf_mhi_update_wr_offset() - Check for any updates in the write offset.
 * @ring:	Ring for the respective context - Channel/Event/Command.
 */
int pci_epf_mhi_update_wr_offset(struct pci_epf_mhi_ring *ring);

/**
 * pci_epf_mhi_process_ring() - Update the Write pointer, fetch the ring elements
 *			    and invoke the clients callback.
 * @ring:	Ring for the respective context - Channel/Event/Command.
 */
int pci_epf_mhi_process_ring(struct pci_epf_mhi_ring *ring);

/**
 * pci_epf_mhi_process_ring_element() - Fetch the ring elements and invoke the
 *			    clients callback.
 * @ring:	Ring for the respective context - Channel/Event/Command.
 * @offset:	Offset index into the respective ring's cache element.
 */
int pci_epf_mhi_process_ring_element(struct pci_epf_mhi_ring *ring, size_t offset);

/**
 * pci_epf_mhi_add_element() - Copy the element to the respective transfer rings
 *			read pointer and increment the index.
 * @ring:	Ring for the respective context - Channel/Event/Command.
 * @element:	Transfer ring element to be copied to the host memory.
 */
int pci_epf_mhi_add_element(struct pci_epf_mhi_ring *ring,
				union pci_epf_mhi_ring_element_type *element,
				struct event_req *ereq, int evt_offset);

/*
 * pci_epf_mhi_ring_set_cb () - Call back function of the ring.
 *
 * @ring:	Ring for the respective context - Channel/Event/Command.
 * @ring_cb:	callback function.
 */
void pci_epf_mhi_ring_set_cb(struct pci_epf_mhi_ring *ring,
			void (*ring_cb)(struct pci_epf_mhi *dev,
			union pci_epf_mhi_ring_element_type *el, void *ctx));

/**
 * pci_epf_mhi_ring_set_state() - Sets internal state of the ring for tracking whether
 *		a ring is being processed, idle or uninitialized.
 * @ring:	Ring for the respective context - Channel/Event/Command.
 * @state:	state of type pci_epf_mhi_ring_state.
 */
void pci_epf_mhi_ring_set_state(struct pci_epf_mhi_ring *ring,
			enum pci_epf_mhi_ring_state state);

/**
 * pci_epf_mhi_ring_get_state() - Obtains the internal state of the ring.
 * @ring:	Ring for the respective context - Channel/Event/Command.
 */
enum pci_epf_mhi_ring_state pci_epf_mhi_ring_get_state(struct pci_epf_mhi_ring *ring);

/* MMIO related functions */

/**
 * pci_epf_mhi_mmio_read() - Generic MHI MMIO register read API.
 * @epf_mhi:	MHI device structure.
 * @offset:	MHI address offset from base.
 * @reg_val:	Pointer the register value is stored to.
 */
int pci_epf_mhi_mmio_read(struct pci_epf_mhi *epf_mhi, u32 offset,
			u32 *reg_value);

/**
 * pci_epf_mhi_mmio_read() - Generic MHI MMIO register write API.
 * @epf_mhi:	MHI device structure.
 * @offset:	MHI address offset from base.
 * @val:	Value to be written to the register offset.
 */
int pci_epf_mhi_mmio_write(struct pci_epf_mhi *epf_mhi, u32 offset,
				u32 val);

/**
 * pci_epf_mhi_mmio_masked_write() - Generic MHI MMIO register write masked API.
 * @epf_mhi:	MHI device structure.
 * @offset:	MHI address offset from base.
 * @mask:	Register field mask.
 * @shift:	Register field mask shift value.
 * @val:	Value to be written to the register offset.
 */
int pci_epf_mhi_mmio_masked_write(struct pci_epf_mhi *epf_mhi, u32 offset,
						u32 mask, u32 shift,
						u32 val);
/**
 * pci_epf_mhi_mmio_masked_read() - Generic MHI MMIO register read masked API.
 * @epf_mhi:	MHI device structure.
 * @offset:	MHI address offset from base.
 * @mask:	Register field mask.
 * @shift:	Register field mask shift value.
 * @reg_val:	Pointer the register value is stored to.
 */
int pci_epf_mhi_mmio_masked_read(struct pci_epf_mhi *epf_mhi, u32 offset,
						u32 mask, u32 shift,
						u32 *reg_val);

/**
 * pci_epf_mhi_mmio_enable_ctrl_interrupt() - Enable Control interrupt.
 * @epf_mhi:	MHI device structure.
 */

int pci_epf_mhi_mmio_enable_ctrl_interrupt(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_mmio_disable_ctrl_interrupt() - Disable Control interrupt.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_disable_ctrl_interrupt(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_mmio_read_ctrl_status_interrupt() - Read Control interrupt status.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_read_ctrl_status_interrupt(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_mmio_enable_cmdb_interrupt() - Enable Command doorbell interrupt.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_enable_cmdb_interrupt(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_mmio_disable_cmdb_interrupt() - Disable Command doorbell interrupt.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_disable_cmdb_interrupt(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_mmio_read_cmdb_interrupt() - Read Command doorbell status.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_read_cmdb_status_interrupt(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_mmio_enable_chdb_a7() - Enable Channel doorbell for a given
 *		channel id.
 * @epf_mhi:	MHI device structure.
 * @chdb_id:	Channel id number.
 */
int pci_epf_mhi_mmio_enable_chdb_a7(struct pci_epf_mhi *epf_mhi, u32 chdb_id);
/**
 * pci_epf_mhi_mmio_disable_chdb_a7() - Disable Channel doorbell for a given
 *		channel id.
 * @epf_mhi:	MHI device structure.
 * @chdb_id:	Channel id number.
 */
int pci_epf_mhi_mmio_disable_chdb_a7(struct pci_epf_mhi *epf_mhi, u32 chdb_id);

/**
 * pci_epf_mhi_mmio_enable_erdb_a7() - Enable Event ring doorbell for a given
 *		event ring id.
 * @epf_mhi:	MHI device structure.
 * @erdb_id:	Event ring id number.
 */
int pci_epf_mhi_mmio_enable_erdb_a7(struct pci_epf_mhi *epf_mhi, u32 erdb_id);

/**
 * pci_epf_mhi_mmio_disable_erdb_a7() - Disable Event ring doorbell for a given
 *		event ring id.
 * @epf_mhi:	MHI device structure.
 * @erdb_id:	Event ring id number.
 */
int pci_epf_mhi_mmio_disable_erdb_a7(struct pci_epf_mhi *epf_mhi, u32 erdb_id);

/**
 * pci_epf_mhi_mmio_enable_chdb_interrupts() - Enable all Channel doorbell
 *		interrupts.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_enable_chdb_interrupts(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_mmio_mask_chdb_interrupts() - Mask all Channel doorbell
 *		interrupts.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_mask_chdb_interrupts(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_mmio_read_chdb_interrupts() - Read all Channel doorbell
 *		interrupts.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_read_chdb_status_interrupts(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_mmio_enable_erdb_interrupts() - Enable all Event doorbell
 *		interrupts.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_enable_erdb_interrupts(struct pci_epf_mhi *epf_mhi);

/**
 *pci_epf_mhi_mmio_mask_erdb_interrupts() - Mask all Event doorbell
 *		interrupts.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_mask_erdb_interrupts(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_mmio_read_erdb_interrupts() - Read all Event doorbell
 *		interrupts.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_read_erdb_status_interrupts(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_mmio_mask_interrupts() - Mask all MHI interrupts.
 * @epf_mhi:	MHI device structure.
 */
void pci_epf_mhi_mmio_mask_interrupts(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_mmio_clear_interrupts() - Clear all doorbell interrupts.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_clear_interrupts(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_mmio_get_chc_base() - Fetch the Channel ring context base address.
 @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_get_chc_base(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_mmio_get_erc_base() - Fetch the Event ring context base address.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_get_erc_base(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_get_crc_base() - Fetch the Command ring context base address.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_get_crc_base(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_mmio_get_ch_db() - Fetch the Write offset of the Channel ring ID.
 * @epf_mhi:	MHI device structure.
 * @wr_offset:	Pointer of the write offset to be written to.
 */
int pci_epf_mhi_mmio_get_ch_db(struct pci_epf_mhi_ring *ring, uint64_t *wr_offset);

/**
 * pci_epf_mhi_get_erc_base() - Fetch the Write offset of the Event ring ID.
 * @epf_mhi:	MHI device structure.
 * @wr_offset:	Pointer of the write offset to be written to.
 */
int pci_epf_mhi_mmio_get_erc_db(struct pci_epf_mhi_ring *ring, uint64_t *wr_offset);

/**
 * pci_epf_mhi_get_cmd_base() - Fetch the Write offset of the Command ring ID.
 * @epf_mhi:	MHI device structure.
 * @wr_offset:	Pointer of the write offset to be written to.
 */
int pci_epf_mhi_mmio_get_cmd_db(struct pci_epf_mhi_ring *ring, uint64_t *wr_offset);

/**
 * pci_epf_mhi_mmio_set_env() - Write the Execution Enviornment.
 * @epf_mhi:	MHI device structure.
 * @value:	Value of the EXEC EVN.
 */
int pci_epf_mhi_mmio_set_env(struct pci_epf_mhi *epf_mhi, u32 value);

/**
 * pci_epf_mhi_mmio_clear_reset() - Clear the reset bit
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_clear_reset(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_mmio_reset() - Reset the MMIO done as part of initialization.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_reset(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_get_mhi_addr() - Fetches the Data and Control region from the Host.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_get_mhi_addr(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_get_mhi_state() - Fetches the MHI state such as M0/M1/M2/M3.
 * @epf_mhi:	MHI device structure.
 * @state:	Pointer of type pci_epf_mhi_state
 * @mhi_reset:	MHI device reset from host.
 */
int pci_epf_mhi_mmio_get_mhi_state(struct pci_epf_mhi *epf_mhi, enum pci_epf_mhi_state *state,
						u32 *epf_mhi_reset);

/**
 * pci_epf_mhi_mmio_init() - Initializes the MMIO and reads the Number of event
 *		rings, support number of channels, and offsets to the Channel
 *		and Event doorbell from the host.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_init(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_mmio_get_cfg() - Reads the Number of event
 *		rings, support number of channels, and offsets to the Channel
 *		and Event doorbell from the host.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_mmio_get_cfg(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_update_ner() - Update the number of event rings (NER) programmed by
 *		the host.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_update_ner(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_restore_mmio() - Restores the MMIO when MHI device comes out of M3.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_restore_mmio(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_backup_mmio() - Backup MMIO before a MHI transition to M3.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_backup_mmio(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_dump_mmio() - Memory dump of the MMIO region for debug.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_dump_mmio(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_config_outbound_iatu() - Configure Outbound Address translation
 *		unit between device and host to map the Data and Control
 *		information.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_config_outbound_iatu(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_send_state_change_event() - Send state change event to the host
 *		such as M0/M1/M2/M3.
 * @epf_mhi:	MHI device structure.
 * @state:	MHI state of type pci_epf_mhi_state
 */
int pci_epf_mhi_send_state_change_event(struct pci_epf_mhi *epf_mhi,
					enum pci_epf_mhi_state state);
/**
 * pci_epf_mhi_send_ee_event() - Send Execution enviornment state change
 *		event to the host.
 * @epf_mhi:	MHI device structure.
 * @state:	MHI state of type pci_epf_mhi_execenv
 */
int pci_epf_mhi_send_ee_event(struct pci_epf_mhi *epf_mhi,
					enum pci_epf_mhi_execenv exec_env);
/**
 * pci_epf_mhi_syserr() - System error when unexpected events are received.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_syserr(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_suspend() - MHI device suspend to stop channel processing at the
 *		Transfer ring boundary, update the channel state to suspended.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_suspend(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_resume() - MHI device resume to update the channel state to running.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_resume(struct pci_epf_mhi *epf_mhi);

/**
 * pci_epf_mhi_trigger_hw_acc_wakeup() - Notify State machine there is HW
 *		accelerated data to be send and prevent MHI suspend.
 * @epf_mhi:	MHI device structure.
 */
int pci_epf_mhi_trigger_hw_acc_wakeup(struct pci_epf_mhi *epf_mhi);

int pci_epf_mhi_sm_init(struct pci_epf_mhi *epf_mhi);
int pci_epf_mhi_sm_exit(struct pci_epf_mhi *epf_mhi);
int pci_epf_mhi_sm_set_ready(struct pci_epf_mhi *epf_mhi);
int pci_epf_mhi_notify_sm_event(enum pci_epf_mhi_event event);
int pci_epf_mhi_sm_get_mhi_state(enum pci_epf_mhi_state *state);

#endif
