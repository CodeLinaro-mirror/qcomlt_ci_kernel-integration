/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause) */
/*
 * Copyright (c) 2023, Linaro Limited
 */

#ifndef __DT_BINDINGS_INTERCONNECT_QCOM_RPM_ICC_H
#define __DT_BINDINGS_INTERCONNECT_QCOM_RPM_ICC_H

#define RPM_ACTIVE_TAG		BIT(0)
#define RPM_SLEEP_TAG		BIT(1)
#define RPM_ALWAYS_TAG		(RPM_ACTIVE_TAG | RPM_SLEEP_TAG)

#endif
