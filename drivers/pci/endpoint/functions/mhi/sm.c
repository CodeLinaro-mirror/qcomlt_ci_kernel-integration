#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/pci-epf.h>

#include "internal.h"

static inline const char *mhi_sm_dev_event_str(enum pci_epf_mhi_event state)
{
	const char *str;

	switch (state) {
	case PCI_EPF_MHI_EVENT_CTRL_TRIG:
		str = "PCI_EPF_MHI_EVENT_CTRL_TRIG";
		break;
	case PCI_EPF_MHI_EVENT_M0_STATE:
		str = "PCI_EPF_MHI_EVENT_M0_STATE";
		break;
	case PCI_EPF_MHI_EVENT_M1_STATE:
		str = "PCI_EPF_MHI_EVENT_M1_STATE";
		break;
	case PCI_EPF_MHI_EVENT_M2_STATE:
		str = "PCI_EPF_MHI_EVENT_M2_STATE";
		break;
	case PCI_EPF_MHI_EVENT_M3_STATE:
		str = "PCI_EPF_MHI_EVENT_M3_STATE";
		break;
	case PCI_EPF_MHI_EVENT_HW_ACC_WAKEUP:
		str = "PCI_EPF_MHI_EVENT_HW_ACC_WAKEUP";
		break;
	case PCI_EPF_MHI_EVENT_CORE_WAKEUP:
		str = "PCI_EPF_MHI_EVENT_CORE_WAKEUP";
		break;
	default:
		str = "INVALID PCI_EPF_MHI_EVENT";
	}

	return str;
}

static inline const char *mhi_sm_mstate_str(enum pci_epf_mhi_state state)
{
	const char *str;

	switch (state) {
	case PCI_EPF_MHI_RESET_STATE:
		str = "RESET";
		break;
	case PCI_EPF_MHI_READY_STATE:
		str = "READY";
		break;
	case PCI_EPF_MHI_M0_STATE:
		str = "M0";
		break;
	case PCI_EPF_MHI_M1_STATE:
		str = "M1";
		break;
	case PCI_EPF_MHI_M2_STATE:
		str = "M2";
		break;
	case PCI_EPF_MHI_M3_STATE:
		str = "M3";
		break;
	case PCI_EPF_MHI_SYSERR_STATE:
		str = "SYSTEM ERROR";
		break;
	default:
		str = "INVALID";
		break;
	}

	return str;
}

static void pci_epf_mhi_sm_mmio_set_status(struct pci_epf_mhi *epf_mhi,
					   enum pci_epf_mhi_state state)
{
	struct pci_epf_mhi_sm *sm = epf_mhi->sm;

	switch (state) {
	case PCI_EPF_MHI_READY_STATE:
		pr_info("set MHISTATUS to READY mode\n");
		pci_epf_mhi_mmio_masked_write(epf_mhi, MHISTATUS,
				MHISTATUS_READY_MASK,
				MHISTATUS_READY_SHIFT, 1);

		pci_epf_mhi_mmio_masked_write(epf_mhi, MHISTATUS,
				MHISTATUS_MHISTATE_MASK,
				MHISTATUS_MHISTATE_SHIFT, state);
		break;
	case PCI_EPF_MHI_SYSERR_STATE:
		pr_info("set MHISTATUS to SYSTEM ERROR mode\n");
		pci_epf_mhi_mmio_masked_write(epf_mhi, MHISTATUS,
				MHISTATUS_SYSERR_MASK,
				MHISTATUS_SYSERR_SHIFT, 1);

		pci_epf_mhi_mmio_masked_write(epf_mhi, MHISTATUS,
				MHISTATUS_MHISTATE_MASK,
				MHISTATUS_MHISTATE_SHIFT, state);
		break;
	case PCI_EPF_MHI_M1_STATE:
	case PCI_EPF_MHI_M2_STATE:
		pr_err("Not supported state, can't set MHISTATUS to %s\n",
			mhi_sm_mstate_str(state));
		return;
	case PCI_EPF_MHI_M0_STATE:
	case PCI_EPF_MHI_M3_STATE:
		pr_info("set MHISTATUS.MHISTATE to %s state\n",
			mhi_sm_mstate_str(state));
		pci_epf_mhi_mmio_masked_write(epf_mhi, MHISTATUS,
				MHISTATUS_MHISTATE_MASK,
				MHISTATUS_MHISTATE_SHIFT, state);
		break;
	default:
		pr_err("Invalid mhi state: 0x%x state", state);
		return;
	}

	sm->state = state;
}

/**
 * pci_epf_mhi_sm_set_ready() -Set MHI state to ready.
 *
 * Set MHISTATUS register in mmio to READY.
 * Synchronic function.
 *
 * Return:	0: success
 *		EINVAL: mhi state manager is not initialized
 *		EPERM: Operation not permitted as EP PCIE link is desable.
 *		EFAULT: MHI state is not RESET
 *		negative: other failure
 */
int pci_epf_mhi_sm_set_ready(struct pci_epf_mhi *epf_mhi)
{
	struct pci_epf_mhi_sm *sm = epf_mhi->sm;
	struct pci_epf *epf = epf_mhi->epf;
	struct device *dev = &epf->dev;
	enum pci_epf_mhi_state state;
	int is_ready;
	int ret = 0;

	mutex_lock(&sm->lock);

	/* verify that MHISTATUS is configured to RESET*/
	pci_epf_mhi_mmio_masked_read(epf_mhi,
		MHISTATUS, MHISTATUS_MHISTATE_MASK,
		MHISTATUS_MHISTATE_SHIFT, &state);

	pci_epf_mhi_mmio_masked_read(epf_mhi, MHISTATUS,
		MHISTATUS_READY_MASK,
		MHISTATUS_READY_SHIFT, &is_ready);

	if (state != PCI_EPF_MHI_RESET_STATE || is_ready) {
		dev_err(dev, "Cannot switch to READY, MHI is not in RESET state");
		dev_err(dev, "-MHISTATE: %s, READY bit: 0x%x\n",
			mhi_sm_mstate_str(state), is_ready);
		ret = -EFAULT;
		goto unlock_and_exit;
	}
	pci_epf_mhi_sm_mmio_set_status(epf_mhi, PCI_EPF_MHI_READY_STATE);

unlock_and_exit:
	mutex_unlock(&sm->lock);
	return ret;
}
EXPORT_SYMBOL(pci_epf_mhi_sm_set_ready);

/**
 * pci_epf_mhi_sm_init() - Initialize MHI state machine.
 * @epf_mhi: pointer to mhi device instance
 *
 * Assuming MHISTATUS register is in RESET state.
 *
 * Return:	0 success
 *		-EINVAL: invalid param
 *		-ENOMEM: allocating memory error
 */
int pci_epf_mhi_sm_init(struct pci_epf_mhi *epf_mhi)
{
	struct pci_epf *epf = epf_mhi->epf;
	struct device *dev = &epf->dev;
	struct pci_epf_mhi_sm *sm;

	sm = devm_kzalloc(dev, sizeof(*epf_mhi->sm), GFP_KERNEL);
	if (!sm)
		return -ENOMEM;

	sm->wq = alloc_workqueue("pci_epf_sm_wq", WQ_HIGHPRI | WQ_UNBOUND, 1);
	if (!sm->wq) {
		dev_err(dev, "Failed to create SM workqueue\n");
		return -ENOMEM;
	}

	mutex_init(&sm->lock);
	sm->state = PCI_EPF_MHI_RESET_STATE;
	sm->d_state = PCI_EPF_MHI_SM_D0_STATE;
	atomic_set(&sm->pending_device_events, 0);
	atomic_set(&sm->pending_pcie_events, 0);
	epf_mhi->sm = sm;

	return 0;
}
EXPORT_SYMBOL(pci_epf_mhi_sm_init);
