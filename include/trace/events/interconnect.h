/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019, Linaro Ltd.
 * Author: Georgi Djakov <georgi.djakov@linaro.org>
 */

#if !defined(_TRACE_INTERCONNECT_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_INTERCONNECT_H

#include <linux/tracepoint.h>

#undef TRACE_SYSTEM
#define TRACE_SYSTEM interconnect

struct icc_node;

TRACE_EVENT(icc_set_bw,

	TP_PROTO(struct icc_node *n, const char *cdev, u32 avg_bw, u32 peak_bw),

	TP_ARGS(n, cdev, avg_bw, peak_bw),

	TP_STRUCT__entry(
		__string(node_name, n->name)
		__field(u32, node_avg_bw)
		__field(u32, node_peak_bw)
		__string(cdev, cdev)
		__field(u32, avg_bw)
		__field(u32, peak_bw)
	),

	TP_fast_assign(
		__assign_str(node_name, n->name);
		__entry->node_avg_bw = n->avg_bw;
		__entry->node_peak_bw = n->peak_bw;
		__assign_str(cdev, cdev);
		__entry->avg_bw = avg_bw;
		__entry->peak_bw = peak_bw;
	),

	TP_printk("%s avg_bw=%u peak_bw=%u cdev=%s avg_bw=%u peak_bw=%u",
		__get_str(node_name),
		__entry->node_avg_bw,
		__entry->node_peak_bw,
		__get_str(cdev),
		__entry->avg_bw,
		__entry->peak_bw)
);
#endif /* _TRACE_INTERCONNECT_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
