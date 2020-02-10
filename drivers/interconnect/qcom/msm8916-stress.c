// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019, Linaro Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/interconnect.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/random.h>
#include <linux/uaccess.h>

struct platform_device *msm8916_icc_pdev;

struct msm8916_icc_stress_data {
	struct icc_path *path;
	u32 src;
	u32 dst;
};

static struct msm8916_icc_stress_data data[] = {
	{ .src = 12, .dst = 53, }, // sdcc2
	{ .src = 9, .dst = 53, }, // mdp
	{ .src = 21, .dst = 53, }, // video
	{ .src = 7, .dst = 53, }, // gfx3d
};

static int msm8916_icc_stress_thread(void *data)
{
	struct msm8916_icc_stress_data *d = data;
	u32 avg_bw, peak_bw, tag = 0;
	int i = 0;

	do {
		avg_bw = get_random_u32() % 10000000 + 1000 ;
		peak_bw = get_random_u32() % 10000000 + 1000;
		tag = get_random_u32() % 100;

		if (IS_ERR_OR_NULL(d->path)) {
			d->path = icc_get(&msm8916_icc_pdev->dev, d->src, d->dst);
			if (IS_ERR(d->path) && PTR_ERR(d->path) != -EPROBE_DEFER)
			    pr_err("%s icc_get src=%u dst=%u error: %lu\n",
				   __func__, d->src, d->dst, PTR_ERR(d->path));
		}

		if (!IS_ERR(d->path)) {

			if (i % 7)
				icc_set_tag(d->path, tag);

			icc_set_bw(d->path, avg_bw, peak_bw);

			avg_bw--;
			peak_bw--;

			if (i++ % 5) {
				icc_put(d->path);
				d->path = NULL;
			}
		}

	} while (!kthread_should_stop());

	return 0;
}

static int __init msm8916_icc_stress_init(void)
{
	int cpu;

	msm8916_icc_pdev = platform_device_alloc("icc-msm8916-stress",
						PLATFORM_DEVID_AUTO);
	platform_device_add(msm8916_icc_pdev);

	for_each_online_cpu(cpu) {

		struct task_struct *k = kthread_create(msm8916_icc_stress_thread,
						       &data[cpu],
						       "icc_stress_%d", cpu);
		kthread_bind(k, cpu);
		wake_up_process(k);
		pr_info("Started %s on cpu%d\n", __func__, cpu);
	}
	/* TODO: stop threads and free resources */

	return 0;
}
late_initcall(msm8916_icc_stress_init);
MODULE_LICENSE("GPL v2");
