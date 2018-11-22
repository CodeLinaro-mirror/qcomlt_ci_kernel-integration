/*
 * Copyright (c) 2012-2018, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/dma-buf.h>
#include <linux/dma-contiguous.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/hash.h>
#include <linux/idr.h>
#include <linux/iommu.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/pagemap.h>
#include <linux/pm_qos.h>
#include <linux/rpmsg.h>
#include <linux/scatterlist.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/sort.h>
#include <linux/uaccess.h>
#include <uapi/linux/fastrpc.h>

#define ADSP_MMAP_HEAP_ADDR 4
#define ADSP_MMAP_REMOTE_HEAP_ADDR 8
#define ADSP_MMAP_ADD_PAGES 0x1000
#define FASTRPC_DMAHANDLE_NOMAP (16)

#define SENSORS_PDR_SERVICE_LOCATION_CLIENT_NAME   "sensors_pdr_adsprpc"
#define IS_CACHE_ALIGNED(x) (((x) & ((L1_CACHE_BYTES)-1)) == 0)

#define BALIGN		128
#define FASTRPC_MAX_SESSIONS	9	/*8 compute, 1 cpz*/
#define M_FDLIST	(16)
#define M_CRCLIST	(64)
#define FASTRPC_CTX_MAX (256)
#define FASTRPC_CTXID_MASK (0xFF0)

#define ADSP_DOMAIN_ID (0)
#define MDSP_DOMAIN_ID (1)
#define SDSP_DOMAIN_ID (2)
#define CDSP_DOMAIN_ID (3)
#define INIT_FILELEN_MAX (2*1024*1024)
#define INIT_MEMLEN_MAX  (8*1024*1024)

#define FASTRPC_DEVICE_NAME	"fastrpc"
#define FASTRPC_DEV_MAX		4 /* adsp, mdsp, slpi, cdsp*/

/* Retrives number of input buffers from the scalars parameter */
#define REMOTE_SCALARS_INBUFS(sc)        (((sc) >> 16) & 0x0ff)

/* Retrives number of output buffers from the scalars parameter */
#define REMOTE_SCALARS_OUTBUFS(sc)       (((sc) >> 8) & 0x0ff)

/* Retrives number of input handles from the scalars parameter */
#define REMOTE_SCALARS_INHANDLES(sc)     (((sc) >> 4) & 0x0f)

/* Retrives number of output handles from the scalars parameter */
#define REMOTE_SCALARS_OUTHANDLES(sc)    ((sc) & 0x0f)

#define REMOTE_SCALARS_LENGTH(sc)	(REMOTE_SCALARS_INBUFS(sc) +\
					REMOTE_SCALARS_OUTBUFS(sc) +\
					REMOTE_SCALARS_INHANDLES(sc) +\
					REMOTE_SCALARS_OUTHANDLES(sc))

#define FASTRPC_BUILD_SCALARS(attr, method, in, out, oin, oout) \
		((((uint32_t)   (attr) & 0x7) << 29) | \
		(((uint32_t) (method) & 0x1f) << 24) | \
		(((uint32_t)     (in) & 0xff) << 16) | \
		(((uint32_t)    (out) & 0xff) <<  8) | \
		(((uint32_t)    (oin) & 0x0f) <<  4) | \
		((uint32_t)   (oout) & 0x0f))

#define FASTRPC_SCALARS(method, in, out) \
		FASTRPC_BUILD_SCALARS(0, method, in, out, 0, 0)

/* Remote Method id table */
#define FASTRPC_RMID_INIT_ATTACH	0
#define FASTRPC_RMID_INIT_RELEASE	1
#define FASTRPC_RMID_MMAP_DSP		4
#define FASTRPC_RMID_UNMMAP_DSP		5
#define FASTRPC_RMID_INIT_CREATE	6
#define FASTRPC_RMID_INIT_CREATE_ATTR	7
#define FASTRPC_RMID_INIT_CREATE_STATIC	8
#define FASTRPC_RMID_UNMMAP_DSP_RH	9

#define cdev_to_cctx(d) container_of(d, struct fastrpc_channel_ctx, cdev)

static const char *domains[FASTRPC_DEV_MAX] = { "adsp", "mdsp",
						"sdsp", "cdsp"};
static dev_t fastrpc_major;
static struct class *fastrpc_class;

struct fastrpc_invoke_header {
	uint64_t ctx;		/* invoke caller context */
	uint32_t handle;	    /* handle to invoke */
	uint32_t sc;		    /* scalars structure describing the data */
};

struct fastrpc_phy_page {
	uint64_t addr;		/* physical address */
	uint64_t size;		/* size of contiguous region */
};

struct fastrpc_invoke_buf {
	int num;		/* number of contiguous regions */
	int pgidx;		/* index to start of contiguous region */
};

struct fastrpc_invoke {
	struct fastrpc_invoke_header header;
	struct fastrpc_phy_page page;   /* remote arg and list of pages address */
};

struct fastrpc_msg {
	uint32_t pid;           /* process group id */
	uint32_t tid;           /* thread id */
	struct fastrpc_invoke invoke;
};

struct fastrpc_invoke_rsp {
	uint64_t ctx;			/* invoke caller context */
	int retval;	             /* invoke return value */
};

struct fastrpc_buf {
	struct list_head node;
	struct fastrpc_user *fl;
	struct dma_buf *dmabuf;
	struct device *dev;
	void *virt;
	uint64_t phys;
	uint64_t dmabuf_phys;
	size_t size;
	unsigned long dma_attr;
	uintptr_t raddr;
	uint32_t flags;
	int remote;
	/* Lock for dma buf attachments */
	struct mutex lock;
	struct list_head attachments;
};

struct fastrpc_dma_buf_attachment {
	struct device *dev;
	struct sg_table sgt;
	struct list_head node;
};

struct fastrpc_mmap {
	struct list_head hn;
	struct fastrpc_user *fl;
	int fd;
	uint32_t flags;
	struct dma_buf *buf;
	struct sg_table *table;
	struct dma_buf_attachment *attach;
	uint64_t phys;
	size_t size;
	uintptr_t va;
	size_t len;
	int refs;
	uintptr_t raddr;
	int uncached;
	int secure;
	uintptr_t attr;
};

struct fastrpc_invoke_ctx {
	struct fastrpc_user *fl;
	struct list_head hn; /* list of ctxs */	
	struct completion work;	
	int retval;
	int pid;
	int tgid;
	uint32_t sc;
	struct fastrpc_msg msg;
	uint64_t ctxid;
	size_t used_sz;
	
	remote_arg_t *lpra;
	unsigned int *attrs;
	int *fds;
	uint32_t *crc;

	remote_arg64_t *rpra;
	struct fastrpc_mmap **maps;
	struct fastrpc_buf *buf;
};

struct fastrpc_session_ctx {
	struct device *dev;
	int sid;
	bool used;
	bool coherent;
	bool secure;
	bool smmu_enabled;
};

struct fastrpc_channel_ctx {
	int domain_id;
	int sesscount;
	/* Indicates, if channel is restricted to secure node only */
	bool secure;
	struct rpmsg_device *rpdev;
	struct fastrpc_session_ctx session[FASTRPC_MAX_SESSIONS];
	uint32_t staticpd_flags;
	spinlock_t lock;
	struct idr ctx_idr;
	struct list_head users;
	//Heap map list...
	struct list_head maps;
	struct cdev cdev;
	struct device dev;
};

struct fastrpc_user {
	struct list_head user;
	struct list_head maps;
	struct list_head bufs;
	struct list_head pending;

	struct fastrpc_channel_ctx *channel_ctx;
	struct fastrpc_session_ctx *sctx;
	struct fastrpc_session_ctx *secsctx;

	struct fastrpc_buf *init_mem;
	uint32_t profile;
	int tgid;
	int cid;
	int pd;
	char *spdname;
	int file_close;

	/* Lock for lists */
	spinlock_t lock;
	/* lock for allocations */
	struct mutex mutex;

	/* Identifies the device (MINOR_NUM_DEV / MINOR_NUM_SECURE_DEV) */
	int dev_minor;
	struct device *dev;
};

static void fastrpc_buf_free(struct fastrpc_buf *buf, int cache)
{
	struct fastrpc_user *fl = buf == NULL ? NULL : buf->fl;

	if (!fl)
		return;

	if (buf->remote) {
		spin_lock(&fl->lock);
		list_del(&buf->node);
		spin_unlock(&fl->lock);
		buf->remote = 0;
		buf->raddr = 0;
	}

	if (!IS_ERR_OR_NULL(buf->virt)) {
		if (fl->sctx->sid && fl->cid != SDSP_DOMAIN_ID)
			buf->phys &= ~((uint64_t)fl->sctx->sid << 32);

		dma_free_coherent(fl->sctx->dev, buf->size, buf->virt, buf->phys);
	}
	kfree(buf);
}

static void fastrpc_remote_buf_list_free(struct fastrpc_user *fl)
{
	struct fastrpc_buf *buf, *free, *n;

	do {
		free = NULL;
		spin_lock(&fl->lock);
		list_for_each_entry_safe(buf, n, &fl->bufs, node) {
			free = buf;
			break;
		}
		spin_unlock(&fl->lock);
		if (free)
			fastrpc_buf_free(free, 0);
	} while (free);
}

static void fastrpc_mmap_add(struct fastrpc_mmap *map)
{
	if (map->flags == ADSP_MMAP_HEAP_ADDR ||
				map->flags == ADSP_MMAP_REMOTE_HEAP_ADDR) {
		struct fastrpc_channel_ctx *cctx = map->fl->channel_ctx;

		list_add_tail(&map->hn, &cctx->maps);
	} else {
		struct fastrpc_user *fl = map->fl;

		list_add_tail(&map->hn, &fl->maps);
	}
}

static int fastrpc_mmap_find(struct fastrpc_user *fl, int fd,
		uintptr_t va, size_t len, int mflags, int refs,
		struct fastrpc_mmap **ppmap)
{
	struct fastrpc_mmap *match = NULL, *map = NULL, *n;
	struct fastrpc_channel_ctx *cctx = fl->channel_ctx;

	if ((va + len) < va)
		return -EOVERFLOW;
	if (mflags == ADSP_MMAP_HEAP_ADDR ||
				 mflags == ADSP_MMAP_REMOTE_HEAP_ADDR) {
		list_for_each_entry_safe(map, n, &cctx->maps, hn) {
			if (va >= map->va &&
				va + len <= map->va + map->len &&
				map->fd == fd) {
				if (refs)
					map->refs++;
				match = map;
				break;
			}
		}
	} else {
		list_for_each_entry_safe(map, n, &fl->maps, hn) {
			if (va >= map->va &&
				va + len <= map->va + map->len &&
				map->fd == fd) {
				if (refs)
					map->refs++;
				match = map;
				break;
			}
		}
	}
	if (match) {
		*ppmap = match;
		return 0;
	}
	return -ENOTTY;
}

static void fastrpc_mmap_free(struct fastrpc_mmap *map, uint32_t flags)
{
	struct fastrpc_user *fl;

	if (!map)
		return;
	fl = map->fl;
	if (map->flags == ADSP_MMAP_HEAP_ADDR ||
				map->flags == ADSP_MMAP_REMOTE_HEAP_ADDR) {
		map->refs--;
		if (!map->refs)
			list_del(&map->hn);
		if (map->refs > 0)
			return;
	} else {
		map->refs--;
		if (!map->refs)
			list_del(&map->hn);
		if (map->refs > 0 && !flags)
			return;
	}
	if (map->flags == ADSP_MMAP_HEAP_ADDR ||
		map->flags == ADSP_MMAP_REMOTE_HEAP_ADDR) {
		if (map->phys)
			dma_free_coherent(fl->sctx->dev, map->size, &map->va,
					map->phys & ~((uint64_t)fl->sctx->sid << 32));
	} else {
		if (!IS_ERR_OR_NULL(map->table))
			dma_buf_unmap_attachment(map->attach, map->table,
					DMA_BIDIRECTIONAL);
		if (!IS_ERR_OR_NULL(map->attach))
			dma_buf_detach(map->buf, map->attach);
		if (!IS_ERR_OR_NULL(map->buf))
			dma_buf_put(map->buf);
	}

	kfree(map);
}

static int fastrpc_mmap_create(struct fastrpc_user *fl, int fd,
	unsigned int attr, uintptr_t va, size_t len, int mflags,
	struct fastrpc_mmap **ppmap)
{
	struct fastrpc_session_ctx *sess = fl->sctx;
	struct fastrpc_mmap *map = NULL;
	int err = 0;

	if (!fastrpc_mmap_find(fl, fd, va, len, mflags, 1, ppmap))
		return 0;
	map = kzalloc(sizeof(*map), GFP_KERNEL);
	if (!map)
		return -ENOMEM;
	INIT_LIST_HEAD(&map->hn);
	map->flags = mflags;
	map->refs = 1;
	map->fl = fl;
	map->fd = fd;
	map->attr = attr;
	if (mflags == ADSP_MMAP_HEAP_ADDR ||
				mflags == ADSP_MMAP_REMOTE_HEAP_ADDR) {
		dma_set_mask(fl->sctx->dev, DMA_BIT_MASK(32));
		map->va = (uintptr_t) dma_alloc_coherent(fl->sctx->dev, len, (void *)&map->phys, GFP_KERNEL);
		if (!map->va)
			pr_err("ADSPRPC: Failed to allocate remote heap memory\n");

		map->phys += ((uint64_t)sess->sid << 32);
		map->size = len;
	} else {
		struct fastrpc_buf *buf;

		if (map->attr && (map->attr & FASTRPC_ATTR_KEEP_MAP)) {
			pr_info("adsprpc: buffer mapped with persist attr %x\n",
				(unsigned int)map->attr);
			map->refs = 2;
		}
		map->buf = dma_buf_get(fd);
		if (!map->buf)
			goto bail;

		map->attach = dma_buf_attach(map->buf, sess->dev);
		if (IS_ERR(map->attach)) {
			dev_err(sess->dev, "Faile to attach dmabuf\n");
			goto bail;
		}

		map->table = dma_buf_map_attachment(map->attach, DMA_BIDIRECTIONAL);
		if (IS_ERR(map->table))
			goto bail;

		buf = map->buf->priv;
		map->phys = buf->phys;
		map->phys = sg_dma_address(map->table->sgl);
		map->phys += ((uint64_t)fl->sctx->sid << 32);
		map->size = len;
		map->va = (uintptr_t) buf->virt;
	}
	map->len = len;

	fastrpc_mmap_add(map);
	*ppmap = map;

bail:
	if (err && map)
		fastrpc_mmap_free(map, 0);
	return err;
}

static int fastrpc_buf_alloc(struct fastrpc_user *fl, struct device *dev, size_t size,
			unsigned long dma_attr, uint32_t rflags,
			int remote, struct fastrpc_buf **obuf)
{
	int err = 0;
	struct fastrpc_buf *buf = NULL;//, *fr = NULL;

	buf = kzalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	INIT_LIST_HEAD(&buf->attachments);
	mutex_init(&buf->lock);

	buf->fl = fl;
	buf->virt = NULL;
	buf->phys = 0;
	buf->size = size;
	buf->dma_attr = dma_attr;
	buf->flags = rflags;
	buf->raddr = 0;
	buf->remote = 0;
	buf->dev = dev;

	buf->virt = dma_alloc_coherent(dev, buf->size,
						(dma_addr_t *)&buf->phys, GFP_KERNEL);
	if (!buf->virt) {
		err = -ENOMEM;
		goto bail;
	}

	buf->dmabuf_phys = buf->phys;

	if (fl->sctx && fl->sctx->sid && fl->cid != SDSP_DOMAIN_ID)
		buf->phys += ((uint64_t)fl->sctx->sid << 32);

	if (remote) {
		INIT_LIST_HEAD(&buf->node);
		spin_lock(&fl->lock);
		list_add_tail(&buf->node, &fl->bufs);
		spin_unlock(&fl->lock);
		buf->remote = remote;
	}
	*obuf = buf;
 bail:
	if (err && buf)
		fastrpc_buf_free(buf, 0);
	return err;
}

static void fastrpc_context_free(struct fastrpc_invoke_ctx *ctx)
{
	struct fastrpc_channel_ctx *cctx = ctx->fl->channel_ctx;
	int nbufs = REMOTE_SCALARS_INBUFS(ctx->sc) +
		    REMOTE_SCALARS_OUTBUFS(ctx->sc);
	int i;

	spin_lock(&ctx->fl->lock);
	list_del(&ctx->hn);
	spin_unlock(&ctx->fl->lock);

	mutex_lock(&ctx->fl->mutex);
	for (i = 0; i < nbufs; ++i)
		fastrpc_mmap_free(ctx->maps[i], 0);
	mutex_unlock(&ctx->fl->mutex);

	fastrpc_buf_free(ctx->buf, 1);

	spin_lock(&cctx->lock);
	idr_remove(&cctx->ctx_idr, ctx->ctxid >> 4);
	spin_unlock(&cctx->lock);

	kfree(ctx);
}

static struct fastrpc_invoke_ctx * fastrpc_context_alloc(
				struct fastrpc_user *fl,
				uint32_t kernel,
				struct fastrpc_ioctl_invoke_crc *invokefd)
{
	struct fastrpc_channel_ctx *cctx = fl->channel_ctx;
	struct fastrpc_ioctl_invoke *invoke = &invokefd->inv;
	struct fastrpc_invoke_ctx *ctx = NULL;
	int err = 0, bufs, size = 0, ret;

	bufs = REMOTE_SCALARS_LENGTH(invoke->sc);
	size = 	(sizeof(*ctx->lpra) + sizeof(*ctx->maps) +
		sizeof(*ctx->fds)  + sizeof(*ctx->attrs)) * bufs;

	ctx = kzalloc(sizeof(*ctx) + size, GFP_KERNEL);
	if (!ctx)
		return ERR_PTR(-ENOMEM);

	INIT_LIST_HEAD(&ctx->hn);
	ctx->fl = fl;
	ctx->maps = (struct fastrpc_mmap **)(&ctx[1]);
	ctx->lpra = (remote_arg_t *)(&ctx->maps[bufs]);
	ctx->fds = (int *)(&ctx->lpra[bufs]);
	ctx->attrs = (unsigned int *)(&ctx->fds[bufs]);

	if (!kernel) {
		err = copy_from_user(ctx->lpra, (void const __user *)invoke->pra,
						bufs * sizeof(*ctx->lpra));
		if (err)
			goto bail;
	
		if (invokefd->fds) {
			err = copy_from_user(ctx->fds, (void const __user *)invokefd->fds,
							bufs * sizeof(*ctx->fds));
			if (err)
				goto bail;
		}
		if (invokefd->attrs) {
			err = copy_from_user(ctx->attrs, (void const __user *)invokefd->attrs,
							bufs * sizeof(*ctx->attrs));
			if (err)
				goto bail;
		}
	} else {
		memcpy(ctx->lpra, invoke->pra, bufs * sizeof(*ctx->lpra));
		if (invokefd->fds)
			memcpy(ctx->fds, invokefd->fds, bufs * sizeof(*ctx->fds));
		if (invokefd->attrs)
			memcpy(ctx->attrs, invokefd->attrs,
						bufs * sizeof(*ctx->attrs));
	}

	ctx->crc = (uint32_t *)invokefd->crc;
	ctx->sc = invoke->sc;
	ctx->retval = -1;
	ctx->pid = current->pid;
	ctx->tgid = fl->tgid;
	init_completion(&ctx->work);

	spin_lock(&fl->lock);
	list_add_tail(&ctx->hn, &fl->pending);
	spin_unlock(&fl->lock);

	spin_lock(&cctx->lock);
	ret = idr_alloc_cyclic(&cctx->ctx_idr, ctx, 1,
				FASTRPC_CTX_MAX, GFP_ATOMIC);
	if (ret < 0) {
		spin_unlock(&cctx->lock);
		err = ret;
		goto bail;
	}
	ctx->ctxid = ret << 4;
	spin_unlock(&cctx->lock);

	return ctx;
bail:
	if (ctx && err)
		fastrpc_context_free(ctx);

	return ERR_PTR(err);
}

static void context_notify_user(struct fastrpc_invoke_ctx *ctx, int retval)
{
	ctx->retval = retval;
	complete(&ctx->work);
}

static void fastrpc_notify_users(struct fastrpc_user *fl)
{
	struct fastrpc_invoke_ctx *ictx, *n;

	spin_lock(&fl->lock);
	list_for_each_entry_safe(ictx, n, &fl->pending, hn) {
		complete(&ictx->work);
	}
	spin_unlock(&fl->lock);

}

static void fastrpc_notify_drivers(struct fastrpc_channel_ctx *cctx)
{
	struct fastrpc_user *fl, *n;

	spin_lock(&cctx->lock);
	list_for_each_entry_safe(fl, n, &cctx->users, user) {
		fastrpc_notify_users(fl);
	}
	spin_unlock(&cctx->lock);

}

static void fastrpc_context_list_dtor(struct fastrpc_user *fl)
{
	struct fastrpc_invoke_ctx *ictx = NULL, *ctxfree, *n;

	do {
		ctxfree = NULL;
		spin_lock(&fl->lock);
		list_for_each_entry_safe(ictx, n, &fl->pending, hn) {
			list_del(&ictx->hn);
			ctxfree = ictx;
			break;
		}
		spin_unlock(&fl->lock);
		if (ctxfree)
			fastrpc_context_free(ctxfree);
	} while (ctxfree);
}

static inline struct fastrpc_invoke_buf *fastrpc_invoke_buf_start(remote_arg64_t *pra,
							uint32_t sc)
{
	return (struct fastrpc_invoke_buf *)(&pra[REMOTE_SCALARS_LENGTH(sc)]);
}

static inline struct fastrpc_phy_page *fastrpc_phy_page_start(uint32_t sc,
						struct fastrpc_invoke_buf *buf)
{
	return (struct fastrpc_phy_page *)(&buf[REMOTE_SCALARS_LENGTH(sc)]);
}

static int fastrpc_get_args(uint32_t kernel, struct fastrpc_invoke_ctx *ctx)
{
	remote_arg64_t *rpra;
	remote_arg_t *lpra = ctx->lpra;
	struct fastrpc_invoke_buf *list;
	struct fastrpc_phy_page *pages;
	uint32_t sc = ctx->sc;
	uintptr_t args;
	size_t rlen = 0, copylen = 0, metalen = 0;
	int inbufs, handles, bufs, i, err = 0, mflags = 0;
	uint64_t *fdlist;
	uint32_t *crclist;

	inbufs = REMOTE_SCALARS_INBUFS(sc);
	bufs = inbufs + REMOTE_SCALARS_OUTBUFS(sc);
	handles = REMOTE_SCALARS_INHANDLES(sc) + REMOTE_SCALARS_OUTHANDLES(sc);
	metalen = (bufs + handles) * (sizeof(remote_arg64_t) +
			sizeof(struct fastrpc_invoke_buf) +
			sizeof(struct fastrpc_phy_page)) +
			sizeof(uint64_t) * M_FDLIST +
			sizeof(uint32_t) * M_CRCLIST;

	copylen = metalen;
	
	for (i = 0; i < bufs + handles; ++i) {
		uintptr_t buf = (uintptr_t)lpra[i].buf.pv;
		size_t len = lpra[i].buf.len;

		if (i < bufs) {
			mutex_lock(&ctx->fl->mutex);
			if (ctx->fds[i] && (ctx->fds[i] != -1))
				fastrpc_mmap_create(ctx->fl, ctx->fds[i], ctx->attrs[i], buf, len, mflags, &ctx->maps[i]);
			mutex_unlock(&ctx->fl->mutex);
	
			if (!len)
				continue;

			if (ctx->maps[i])
				continue;

			copylen = ALIGN(copylen, BALIGN);
			copylen += len;
		} else {
			int dmaflags = 0;
	
			if (ctx->attrs && (ctx->attrs[i] & FASTRPC_ATTR_NOMAP))
				dmaflags = FASTRPC_DMAHANDLE_NOMAP;
	
			mutex_lock(&ctx->fl->mutex);
			err = fastrpc_mmap_create(ctx->fl, ctx->fds[i], FASTRPC_ATTR_NOVA, 0, 0, dmaflags, &ctx->maps[i]);
			mutex_unlock(&ctx->fl->mutex);
			if (err)
				goto bail;
		}
	}
	ctx->used_sz = copylen;

	/* allocate new buffer */
	if (copylen) {
		err = fastrpc_buf_alloc(ctx->fl, ctx->fl->sctx->dev, copylen, 0, 0, 0, &ctx->buf);
		if (err)
			goto bail;
	}

	/* copy metadata */
	rpra = ctx->buf->virt;
	ctx->rpra = rpra;
	list = fastrpc_invoke_buf_start(rpra, sc);
	pages = fastrpc_phy_page_start(sc, list);
	args = (uintptr_t)ctx->buf->virt + metalen;
	fdlist = (uint64_t *)&pages[bufs + handles];
	memset(fdlist, 0, sizeof(uint32_t)*M_FDLIST);
	crclist = (uint32_t *)&fdlist[M_FDLIST];
	memset(crclist, 0, sizeof(uint32_t)*M_CRCLIST);
	rlen = copylen - metalen;

	for (i = 0; i < bufs + handles; ++i) {
		struct fastrpc_mmap *map = ctx->maps[i];
		size_t len = lpra[i].buf.len;
		size_t mlen;

		if (len)
			list[i].num = 1;
		else
			list[i].num = 0;

		list[i].pgidx = i;

		if (i < bufs) { /* buffers */
			rpra[i].buf.pv = 0;
			rpra[i].buf.len = len;
			if (!len)
				continue;
			if (map) {
				uintptr_t offset = 0;
				uint64_t num = roundup(len, PAGE_SIZE) / PAGE_SIZE;
				int idx = list[i].pgidx;

				pages[idx].addr = map->phys + offset;
				pages[idx].size = num << PAGE_SHIFT;
			} else {
				rlen -= ALIGN(args, BALIGN) - args;
				args = ALIGN(args, BALIGN);
				mlen = len;
				if (rlen < mlen)
					goto bail;
		
				rpra[i].buf.pv = (args);		
				pages[list[i].pgidx].addr = ctx->buf->phys + (copylen - rlen);
				pages[list[i].pgidx].addr = pages[list[i].pgidx].addr & PAGE_MASK;
		
				pages[list[i].pgidx].size = roundup(len, PAGE_SIZE) * PAGE_SIZE;
		
				if (i < inbufs) {
					if (!kernel) {
						err = copy_from_user((void *)rpra[i].buf.pv, (void const __user *)lpra[i].buf.pv, len);
						if (err)
							goto bail;
					} else {
						memcpy((void *)rpra[i].buf.pv, lpra[i].buf.pv, len);
					}
				}
				args = args + mlen;
				rlen -= mlen;
			}

		} else { /* handles */
			pages[i].addr = map->phys;
			pages[i].size = map->size;
			rpra[i].dma.fd = ctx->fds[i];
			rpra[i].dma.len = len;
			rpra[i].dma.offset = (uint32_t)(uintptr_t)lpra[i].buf.pv;
		}
	}
 bail:
	return err;
}

static int fastrpc_put_args(struct fastrpc_invoke_ctx *ctx,
			    uint32_t kernel, remote_arg_t *upra)
{
	remote_arg64_t *rpra = ctx->rpra;
	int i, inbufs, outbufs, handles;
	struct fastrpc_invoke_buf *list;
	struct fastrpc_phy_page *pages;
	struct fastrpc_mmap *mmap;
	uint32_t sc = ctx->sc;
	uint64_t *fdlist;
	uint32_t *crclist;
	int err = 0;

	inbufs = REMOTE_SCALARS_INBUFS(sc);
	outbufs = REMOTE_SCALARS_OUTBUFS(sc);
	handles = REMOTE_SCALARS_INHANDLES(sc) + REMOTE_SCALARS_OUTHANDLES(sc);
	list = fastrpc_invoke_buf_start(ctx->rpra, sc);
	pages = fastrpc_phy_page_start(sc, list);
	fdlist = (uint64_t *)(pages + inbufs + outbufs + handles);
	crclist = (uint32_t *)(fdlist + M_FDLIST);

	for (i = inbufs; i < inbufs + outbufs; ++i) {
		if (!ctx->maps[i]) {
			if (!kernel)
				err = copy_to_user((void __user *)ctx->lpra[i].buf.pv,	(void * )rpra[i].buf.pv,rpra[i].buf.len);
			else 
				memcpy(ctx->lpra[i].buf.pv, (void *)rpra[i].buf.pv,rpra[i].buf.len);

			if (err)
				goto bail;
		} else {
			mutex_lock(&ctx->fl->mutex);
			fastrpc_mmap_free(ctx->maps[i], 0);
			mutex_unlock(&ctx->fl->mutex);
			ctx->maps[i] = NULL;
		}
	}
	mutex_lock(&ctx->fl->mutex);
	if (inbufs + outbufs + handles) {
		for (i = 0; i < M_FDLIST; i++) {
			if (!fdlist[i])
				break;
			if (!fastrpc_mmap_find(ctx->fl, (int)fdlist[i], 0, 0,
						0, 0, &mmap))
				fastrpc_mmap_free(mmap, 0);
		}
	}
	mutex_unlock(&ctx->fl->mutex);
	if (ctx->crc && crclist) {
		if (!kernel)
			err = copy_to_user((void __user *)ctx->crc, crclist, M_CRCLIST*sizeof(uint32_t));
		else
			memcpy(ctx->crc, crclist, M_CRCLIST*sizeof(uint32_t));
	}

 bail:
	return err;
}

static void inv_args_pre(struct fastrpc_invoke_ctx *ctx)
{
	int i, inbufs, outbufs;
	uint32_t sc = ctx->sc;
	remote_arg64_t *rpra = ctx->rpra;
	uintptr_t end;

	inbufs = REMOTE_SCALARS_INBUFS(sc);
	outbufs = REMOTE_SCALARS_OUTBUFS(sc);
	for (i = inbufs; i < inbufs + outbufs; ++i) {
		struct fastrpc_mmap *map = ctx->maps[i];

		if (map && map->uncached)
			continue;
		if (!rpra[i].buf.len)
			continue;
		if (ctx->fl->sctx->coherent &&
			!(map && (map->attr & FASTRPC_ATTR_NON_COHERENT)))
			continue;
		if (map && (map->attr & FASTRPC_ATTR_COHERENT))
			continue;

		if (((uint64_t)rpra & PAGE_MASK) == (rpra[i].buf.pv & PAGE_MASK))
			continue;

		if (!IS_CACHE_ALIGNED((uintptr_t)(rpra[i].buf.pv))) {
			if (map && map->attach) {
				dma_buf_begin_cpu_access(map->buf,
					DMA_BIDIRECTIONAL);
				dma_buf_end_cpu_access(map->buf,
					DMA_BIDIRECTIONAL);
			}
		}

		end = (uintptr_t)(rpra[i].buf.pv +
							rpra[i].buf.len);
		if (!IS_CACHE_ALIGNED(end)) {
			if (map && map->attach) {
				dma_buf_begin_cpu_access(map->buf,
					DMA_BIDIRECTIONAL);
				dma_buf_end_cpu_access(map->buf,
					DMA_BIDIRECTIONAL);
			}
		}
	}
}

static void inv_args(struct fastrpc_invoke_ctx *ctx)
{
	int i, inbufs, outbufs;
	uint32_t sc = ctx->sc;
	remote_arg64_t *rpra = ctx->rpra;

	inbufs = REMOTE_SCALARS_INBUFS(sc);
	outbufs = REMOTE_SCALARS_OUTBUFS(sc);
	for (i = inbufs; i < inbufs + outbufs; ++i) {
		struct fastrpc_mmap *map = ctx->maps[i];

		if (map && map->uncached)
			continue;
		if (!rpra[i].buf.len)
			continue;
		if (ctx->fl->sctx->coherent &&
			!(map && (map->attr & FASTRPC_ATTR_NON_COHERENT)))
			continue;
		if (map && (map->attr & FASTRPC_ATTR_COHERENT))
			continue;

		if (((uint64_t)rpra & PAGE_MASK) == (rpra[i].buf.pv & PAGE_MASK))
			continue;

		if (map && map->buf) {
			dma_buf_begin_cpu_access(map->buf,
				DMA_BIDIRECTIONAL);
			dma_buf_end_cpu_access(map->buf,
				DMA_BIDIRECTIONAL);
		}
	}

}

static int fastrpc_invoke_send(struct fastrpc_session_ctx *sctx,
			       struct fastrpc_invoke_ctx *ctx,
			       uint32_t kernel, uint32_t handle)
{
	struct fastrpc_channel_ctx *channel_ctx;
	struct fastrpc_user *fl = ctx->fl;
	struct fastrpc_msg *msg = &ctx->msg;

	channel_ctx = fl->channel_ctx;
	msg->pid = fl->tgid;
	msg->tid = current->pid;

	if (kernel)
		msg->pid = 0;

	msg->invoke.header.ctx = ctx->ctxid | fl->pd;
	msg->invoke.header.handle = handle;
	msg->invoke.header.sc = ctx->sc;
	msg->invoke.page.addr = ctx->buf ? ctx->buf->phys : 0;
	//msg->invoke.page.size = buf_page_size(ctx->used_sz);
	msg->invoke.page.size = roundup(ctx->used_sz, PAGE_SIZE);

	return rpmsg_send(channel_ctx->rpdev->ept, (void *)msg, sizeof(*msg));
}

static int fastrpc_internal_invoke(struct fastrpc_user *fl,
				   uint32_t kernel,
				   struct fastrpc_ioctl_invoke_crc *inv)
{
	struct fastrpc_ioctl_invoke *invoke = &inv->inv;
	struct fastrpc_invoke_ctx *ctx = NULL;
	int err = 0;

	if (!fl->sctx)
		return -EINVAL;

	ctx = fastrpc_context_alloc(fl, kernel, inv);
	if (err)
		goto bail;

	if (REMOTE_SCALARS_LENGTH(ctx->sc)) {
		err = fastrpc_get_args(kernel, ctx);
		if (err)
			goto bail;
	}

	if (!fl->sctx->coherent)
		inv_args_pre(ctx);

	err = fastrpc_invoke_send(fl->sctx, ctx, kernel, invoke->handle);
	if (err)
		goto bail;

	err = wait_for_completion_interruptible(&ctx->work);
	if (err)
		goto bail;

	if (!fl->sctx->coherent)
		inv_args(ctx);

	err = ctx->retval;
	if (err)
		goto bail;

	err = fastrpc_put_args(ctx, kernel, invoke->pra);
	if (err)
		goto bail;
 bail:
	if (ctx)
		fastrpc_context_free(ctx);

	return err;
}

static int fastrpc_release_current_dsp_process(struct fastrpc_user *fl)
{
	struct fastrpc_ioctl_invoke_crc ioctl;
	remote_arg_t ra[1];
	int tgid = 0;

	tgid = fl->tgid;
	ra[0].buf.pv = (void *)&tgid;
	ra[0].buf.len = sizeof(tgid);
	ioctl.inv.handle = 1;
	ioctl.inv.sc = FASTRPC_SCALARS(FASTRPC_RMID_INIT_RELEASE, 1, 0);
	ioctl.inv.pra = ra;
	ioctl.fds = NULL;
	ioctl.attrs = NULL;
	ioctl.crc = NULL;

	return fastrpc_internal_invoke(fl, 1, &ioctl);
}

static const struct of_device_id fastrpc_match_table[] = {
	{ .compatible = "qcom,fastrpc-compute-cb", },
	{}
};

static void fastrpc_session_free(struct fastrpc_channel_ctx *chan, struct fastrpc_session_ctx *session)
{
	spin_lock(&chan->lock);
	session->used = false;
	spin_unlock(&chan->lock);
}

static int fastrpc_user_free(struct fastrpc_user *fl)
{
	struct fastrpc_mmap *map = NULL, *lmap = NULL, *n;
	struct fastrpc_channel_ctx *cctx = fl->channel_ctx;

	fastrpc_release_current_dsp_process(fl);

	spin_lock(&cctx->lock);

	list_del(&fl->user);

	spin_unlock(&cctx->lock);

	if (!fl->sctx) {
		kfree(fl);
		return 0;
	}
	spin_lock(&fl->lock);
	fl->file_close = 1;
	spin_unlock(&fl->lock);
	if (!IS_ERR_OR_NULL(fl->init_mem))
		fastrpc_buf_free(fl->init_mem, 0);
	fastrpc_context_list_dtor(fl);

	mutex_lock(&fl->mutex);
	do {
		lmap = NULL;
		list_for_each_entry_safe(map, n, &fl->maps, hn) {
			list_del(&map->hn);
			lmap = map;
			break;
		}
		fastrpc_mmap_free(lmap, 1);
	} while (lmap);
	mutex_unlock(&fl->mutex);

	if (fl->sctx)
		fastrpc_session_free(fl->channel_ctx, fl->sctx);
	if (fl->secsctx)
		fastrpc_session_free(fl->channel_ctx, fl->secsctx);

	fastrpc_remote_buf_list_free(fl);
	mutex_destroy(&fl->mutex);
	kfree(fl);
	return 0;
}

static int fastrpc_device_release(struct inode *inode, struct file *file)
{
	struct fastrpc_user *fl = (struct fastrpc_user *)file->private_data;

	if (fl) {
		fastrpc_user_free(fl);
		file->private_data = NULL;
	}
	return 0;
}

static int fastrpc_device_open(struct inode *inode, struct file *filp)
{
	struct fastrpc_channel_ctx *cctx = cdev_to_cctx(inode->i_cdev);
	struct fastrpc_user *fl = NULL;

	fl = kzalloc(sizeof(*fl), GFP_KERNEL);
	if (!fl)
		return -ENOMEM;

	filp->private_data = fl;

	spin_lock_init(&fl->lock);
	mutex_init(&fl->mutex);
	INIT_LIST_HEAD(&fl->pending);
	INIT_LIST_HEAD(&fl->maps);
	INIT_LIST_HEAD(&fl->bufs);
	INIT_LIST_HEAD(&fl->user);

	fl->tgid = current->tgid;
	fl->channel_ctx = cctx;
	fl->dev = &cctx->rpdev->dev;

	spin_lock(&cctx->lock);
	list_add_tail(&fl->user, &cctx->users);
	spin_unlock(&cctx->lock);

	return 0;
}

static const struct file_operations fastrpc_fops = {
	.open = fastrpc_device_open,
	.release = fastrpc_device_release,
};

static int fastrpc_cb_probe(struct platform_device *pdev)
{
	struct fastrpc_channel_ctx *chan;
	struct fastrpc_session_ctx *sess;
	struct of_phandle_args iommuspec;
	struct device *dev = &pdev->dev;
	int j, sharedcb_count, err = 0;

	chan = dev_get_drvdata(dev->parent);
	if (!chan)
		return -EINVAL;

	sess = &chan->session[chan->sesscount];
	if (!of_parse_phandle_with_args(dev->of_node, "iommus",
				"#iommu-cells", 0, &iommuspec)) {
		sess->sid = iommuspec.args[0] & 0xf;
		dma_set_mask(dev, DMA_BIT_MASK(32));
		sess->smmu_enabled = true;
	}

	sess->used = false;
	sess->coherent = of_property_read_bool(dev->of_node, "dma-coherent");
	sess->secure = of_property_read_bool(dev->of_node, "qcom,secure-context-bank");
	sess->dev = dev;
	dma_set_max_seg_size(sess->dev, DMA_BIT_MASK(32));
	dma_set_seg_boundary(sess->dev, (unsigned long)DMA_BIT_MASK(64));

	if (of_get_property(dev->of_node, "shared-cb", NULL) != NULL) {
		if (of_property_read_u32(dev->of_node, "shared-cb",
				&sharedcb_count))
			goto bail;

		if (sharedcb_count > 0) {
			struct fastrpc_session_ctx *dup_sess;

			for (j = 1; j < sharedcb_count &&
					chan->sesscount < FASTRPC_MAX_SESSIONS; j++) {
				chan->sesscount++;
				dup_sess = &chan->session[chan->sesscount];
				memcpy(dup_sess, sess, sizeof(*dup_sess));
			}
		}
	}

	chan->sesscount++;
bail:
	return err;
}

static int fastrpc_cb_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver fastrpc_cb_driver = {
	.probe = fastrpc_cb_probe,
	.remove = fastrpc_cb_remove,
	.driver = {
		.name = "fastrpc",
		.owner = THIS_MODULE,
		.of_match_table = fastrpc_match_table,
		.suppress_bind_attrs = true,
	},
};

static void fastrpc_cdev_release_device(struct device *dev)
{
	struct fastrpc_channel_ctx *data = dev_get_drvdata(dev->parent);

	cdev_del(&data->cdev);
}

static int fastrpc_rpmsg_probe(struct rpmsg_device *rpdev)
{
	struct device *rdev = &rpdev->dev;
	struct fastrpc_channel_ctx *data;
	struct device *dev;
	int err, domain_id;

	data = devm_kzalloc(rdev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	err = of_property_read_u32(rdev->of_node, "reg", &domain_id);
	if (err) {
		dev_err(rdev, "FastRPC Domain ID not specified in DT\n");
		return err;
	}

	if (of_property_read_bool(rdev->of_node, "secured"))
		data->secure = true;

	dev = &data->dev;
	device_initialize(dev);
	dev->parent = &rpdev->dev;
	dev->class = fastrpc_class;

	cdev_init(&data->cdev, &fastrpc_fops);
	data->cdev.owner = THIS_MODULE;
	dev->devt = MKDEV(MAJOR(fastrpc_major), domain_id);
	dev->id = domain_id;
	dev_set_name(&data->dev, "fastrpc-%s", domains[domain_id]);
	dev->release = fastrpc_cdev_release_device;

	err = cdev_device_add(&data->cdev, &data->dev);
	if (err)
		goto cdev_err;

	dev_set_drvdata(&rpdev->dev, data);
	dma_set_mask_and_coherent(rdev, DMA_BIT_MASK(32));
	INIT_LIST_HEAD(&data->users);
	INIT_LIST_HEAD(&data->maps);
	spin_lock_init(&data->lock);
	idr_init(&data->ctx_idr);
	data->domain_id = domain_id;
	data->rpdev = rpdev;

	return of_platform_populate(rdev->of_node, NULL, NULL, rdev);

cdev_err:
	put_device(dev);
	return err;
}

static void fastrpc_rpmsg_remove(struct rpmsg_device *rpdev)
{	
	struct fastrpc_channel_ctx *data = dev_get_drvdata(&rpdev->dev);

	fastrpc_notify_drivers(data);
	device_del(&data->dev);
	put_device(&data->dev);
	of_platform_depopulate(&rpdev->dev);
	kfree(data);
}

static int fastrpc_rpmsg_callback(struct rpmsg_device *rpdev, void *data,
				  int len, void *priv, u32 addr)
{
	struct fastrpc_channel_ctx *cctx = dev_get_drvdata(&rpdev->dev);
	struct fastrpc_invoke_rsp *rsp = (struct fastrpc_invoke_rsp *)data;
	struct fastrpc_invoke_ctx *ctx;
	unsigned long flags;
	int ctxid;

	if (rsp && len < sizeof(*rsp)) {
		dev_err(&rpdev->dev, "invalid response or context\n");
		return -EINVAL;
	}

	ctxid = (uint32_t)((rsp->ctx & FASTRPC_CTXID_MASK) >> 4);

	spin_lock_irqsave(&cctx->lock, flags);
	ctx = idr_find(&cctx->ctx_idr, ctxid);
	spin_unlock_irqrestore(&cctx->lock, flags);

	if (!ctx) {
		dev_err(&rpdev->dev, "No context ID matches response\n");
		return -ENOENT;
	}

	context_notify_user(ctx, rsp->retval);

	return 0;
}

static const struct of_device_id fastrpc_rpmsg_of_match[] = {
	{ .compatible = "qcom,fastrpc" },
	{ },
};
MODULE_DEVICE_TABLE(of, fastrpc_rpmsg_of_match);

static struct rpmsg_driver fastrpc_driver = {
	.probe = fastrpc_rpmsg_probe,
	.remove = fastrpc_rpmsg_remove,
	.callback = fastrpc_rpmsg_callback,
	.drv = {
		.name = "qcom,msm_fastrpc_rpmsg",
		.of_match_table = fastrpc_rpmsg_of_match,
	},
};

static int fastrpc_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&fastrpc_major, 0, FASTRPC_DEV_MAX,
				  FASTRPC_DEVICE_NAME);
	if (ret < 0) {
		pr_err("fastrpc: failed to allocate char dev region\n");
		return ret;
	}

	fastrpc_class = class_create(THIS_MODULE, "fastrpc");
	if (IS_ERR(fastrpc_class)) {
		pr_err("failed to create rpmsg class\n");
		unregister_chrdev_region(fastrpc_major, FASTRPC_DEV_MAX);
		return PTR_ERR(fastrpc_class);
	}

	ret = platform_driver_register(&fastrpc_cb_driver);
	if (ret < 0) {
		pr_err("fastrpc: failed to register cb driver\n");
		class_destroy(fastrpc_class);
		unregister_chrdev_region(fastrpc_major, FASTRPC_DEV_MAX);
	}

	ret = register_rpmsg_driver(&fastrpc_driver);
	if (ret < 0) {
		pr_err("fastrpc: failed to register rpmsg driver\n");
		class_destroy(fastrpc_class);
		unregister_chrdev_region(fastrpc_major, FASTRPC_DEV_MAX);
		platform_driver_unregister(&fastrpc_cb_driver);
	}

	return ret;
}
module_init(fastrpc_init);

static void fastrpc_exit(void)
{
	platform_driver_unregister(&fastrpc_cb_driver);
	unregister_rpmsg_driver(&fastrpc_driver);
	class_destroy(fastrpc_class);
	unregister_chrdev_region(fastrpc_major, FASTRPC_DEV_MAX);
}
module_exit(fastrpc_exit);

MODULE_ALIAS("fastrpc:fastrpc");
MODULE_LICENSE("GPL v2");
