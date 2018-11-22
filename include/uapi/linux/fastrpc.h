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
#ifndef __QCOM_FASTRPC_H__
#define __QCOM_FASTRPC_H__

#include <linux/types.h>

#define FASTRPC_IOCTL_INVOKE	_IOWR('R', 1, struct fastrpc_ioctl_invoke)
#define FASTRPC_IOCTL_INVOKE_FD	_IOWR('R', 4, struct fastrpc_ioctl_invoke_fd)
#define FASTRPC_IOCTL_INVOKE_ATTRS \
				_IOWR('R', 7, struct fastrpc_ioctl_invoke_attrs)
#define FASTRPC_IOCTL_INVOKE_CRC _IOWR('R', 11, struct fastrpc_ioctl_invoke_crc)

#define FASTRPC_IOCTL_INIT	_IOWR('R', 6, struct fastrpc_ioctl_init)
#define FASTRPC_IOCTL_INIT_ATTRS _IOWR('R', 10, struct fastrpc_ioctl_init_attrs)
#define FASTRPC_IOCTL_ALLOC_DMA_BUFF _IOWR('R', 16, struct fastrpc_ioctl_alloc_dma_buf)
#define FASTRPC_IOCTL_FREE_DMA_BUFF _IOWR('R', 17, uint32_t)

/* Set for buffers that have no virtual mapping in userspace */
#define FASTRPC_ATTR_NOVA 0x1

/* Set for buffers that are NOT dma coherent */
#define FASTRPC_ATTR_NON_COHERENT 0x2

/* Set for buffers that are dma coherent */
#define FASTRPC_ATTR_COHERENT 0x4

/* Fastrpc attribute for keeping the map persistent */
#define FASTRPC_ATTR_KEEP_MAP	0x8

/* Fastrpc attribute for no mapping of fd  */
#define FASTRPC_ATTR_NOMAP (16)

/* INIT a new process or attach to guestos */
#define FASTRPC_INIT_ATTACH      0
#define FASTRPC_INIT_CREATE      1
#define FASTRPC_INIT_CREATE_STATIC  2
#define FASTRPC_INIT_ATTACH_SENSORS 3

#define remote_arg64_t    union remote_arg64

struct remote_buf64 {
	uint64_t pv;
	uint64_t len;
};

struct remote_dma_handle64 {
	int fd;
	uint32_t offset;
	uint32_t len;
};

union remote_arg64 {
	struct remote_buf64	buf;
	struct remote_dma_handle64 dma;
	uint32_t h;
};

#define remote_arg_t    union remote_arg

struct remote_buf {
	void *pv;		/* buffer pointer */
	size_t len;		/* length of buffer */
};

struct remote_dma_handle {
	int fd;
	uint32_t offset;
};

union remote_arg {
	struct remote_buf buf;	/* buffer info */
	struct remote_dma_handle dma;
	uint32_t h;		/* remote handle */
};

struct fastrpc_ioctl_invoke {
	uint32_t handle;	/* remote handle */
	uint32_t sc;		/* scalars describing the data */
	remote_arg_t *pra;	/* remote arguments list */
};

struct fastrpc_ioctl_invoke_fd {
	struct fastrpc_ioctl_invoke inv;
	int *fds;		/* fd list */
};

struct fastrpc_ioctl_invoke_attrs {
	struct fastrpc_ioctl_invoke inv;
	int *fds;		/* fd list */
	unsigned int *attrs;	/* attribute list */
};

struct fastrpc_ioctl_invoke_crc {
	struct fastrpc_ioctl_invoke inv;
	int *fds;		/* fd list */
	unsigned int *attrs;	/* attribute list */
	unsigned int *crc;
};

struct fastrpc_ioctl_init {
	uint32_t flags;		/* one of FASTRPC_INIT_* macros */
	uintptr_t file;		/* pointer to elf file */
	uint32_t filelen;	/* elf file length */
	int32_t filefd;		/* ION fd for the file */
	uintptr_t mem;		/* mem for the PD */
	uint32_t memlen;	/* mem length */
	int32_t memfd;		/* ION fd for the mem */
};

struct fastrpc_ioctl_init_attrs {
		struct fastrpc_ioctl_init init;
		int attrs;
		unsigned int siglen;
};

struct fastrpc_ioctl_alloc_dma_buf {
	int     fd;	/* fd */
	ssize_t size;	/* size */
	uint32_t flags;	/* flags to map with */
};

#endif /* __QCOM_FASTRPC_H__ */
