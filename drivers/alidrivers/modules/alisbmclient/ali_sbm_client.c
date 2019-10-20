/*
 * Share Buffer Manager driver
 * Copyright(C) 2015 ALi Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/compat.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vt.h>
#include <linux/init.h>
#include <linux/linux_logo.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/console.h>
#include <linux/kmod.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/efi.h>
#include <linux/fb.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/version.h>
#include <ali_sbm_types.h>
#include <linux/ali_rpc.h>
#include <rpc_hld/ali_rpc_sbm.h>

#define SBM_PRF(fmt, args...) \
	    do { \
			pr_err("[ali sbm]"); \
			pr_err(fmt, ##args); \
		} while (0)

struct sbm_config_uni {
	u8 *buf_start;
	u8 *buf_end;
};

struct sbm_rw_desc_uni {
	u8 *read;
	u8 *write;
};

struct sbm_desc_uni {
	struct sbm_config_uni sbm_cfg;
	struct sbm_rw_desc_uni *sbm_rw;
	u8 *local_read;
	struct semaphore sema;
};

static u32 desc_sbm_create[] =
{   //desc of pointer para
	1, DESC_OUTPUT_STRU(0, sizeof(struct see_sbm)),
	1, DESC_P_PARA(0, 0, 0),
	//desc of pointer ret
	0,
	0,
};

static u32 desc_sbm_destroy[] =
{   //desc of pointer para
	1, DESC_STATIC_STRU(0, sizeof(struct see_sbm)),
	1, DESC_P_PARA(0, 0, 0),
	//desc of pointer ret
	0,
	0,
};

static int sbm_create(struct see_sbm *sbm)
{
	register int ret asm("$2");
    u32 *desc = desc_sbm_create;

	jump_to_func(NULL, ali_rpc_call, sbm, (LLD_SBM_MODULE<<24)|(1<<16)|FUNC_SBM_CREATE, desc);

	return ret;
}

static int sbm_destroy(struct see_sbm *sbm)
{
	register int ret asm("$2");
    u32 *desc = desc_sbm_destroy;

	jump_to_func(NULL, ali_rpc_call, sbm, (LLD_SBM_MODULE<<24)|(1<<16)|FUNC_SBM_DESTROY, desc);

	return ret;
}

int see_sbm_create(struct see_sbm *sbm)
{
	struct sbm_desc_uni *sbm_desc = NULL;
	struct sbm_rw_desc_uni *sbm_rw = NULL;
	int res = 0;

	if ((sbm == NULL) || (sbm->buf_size == 0)) {
		SBM_PRF("Creating SBM with parameter error\n");
		return -EINVAL;
	}

	if (sbm->buf_start == NULL) {
		SBM_PRF("SBM buffer NULL\n");
		return -EINVAL;
	}

	sbm_desc = kzalloc(sizeof(struct sbm_desc_uni), GFP_KERNEL);
	if (sbm_desc == NULL) {
		SBM_PRF("malloc SBM info fail\n");
		return -ENOMEM;
	}

	sbm_desc->sbm_cfg.buf_start = (u8 *)
	    dma_map_single(NULL, sbm->buf_start, sbm->buf_size, DMA_TO_DEVICE);
	sbm_desc->sbm_cfg.buf_end = sbm_desc->sbm_cfg.buf_start + sbm->buf_size;
	sbm_desc->local_read = sbm_desc->sbm_cfg.buf_start;

	sema_init(&sbm_desc->sema, 1);

	sbm_desc->sbm_rw = (struct sbm_rw_desc_uni *)
		ali_rpc_malloc_shared_mm(sizeof(struct sbm_rw_desc_uni));
	if (sbm_desc->sbm_rw == NULL) {
		SBM_PRF("malloc SBM rw desc fail\n");
		goto fail;
	}

	sbm_rw = sbm_desc->sbm_rw;
	sbm_rw->read = sbm_desc->sbm_cfg.buf_start;
	sbm_rw->write = sbm_desc->sbm_cfg.buf_start;

	sbm->id = -1;
	sbm->read_pointer = (void **)virt_to_phys(&sbm_rw->read);
	sbm->write_pointer = (void **)virt_to_phys(&sbm_rw->write);
	sbm->priv_data = (void *)sbm_desc;
	res = sbm_create(sbm);
	if (res != 0) {
		SBM_PRF("Create SEE SBM fail\n");
		goto fail;
	}

	return 0;

fail:
	if (sbm_desc != NULL) {
		if (sbm_desc->sbm_rw)
			ali_rpc_free_shared_mm(sbm_desc->sbm_rw,
				sizeof(struct sbm_desc_uni));

		kfree(sbm_desc);
	}

	sbm->id = -1;
	sbm->read_pointer = NULL;
	sbm->write_pointer = NULL;
	sbm->priv_data = NULL;

	return -ENOMEM;
}
EXPORT_SYMBOL(see_sbm_create);

int see_sbm_destroy(struct see_sbm *sbm)
{
	struct sbm_desc_uni *sbm_desc = NULL;
	int res = 0;

	if ((sbm == NULL) || (sbm->id < 0)) {
		SBM_PRF("Destroying SBM with parameter error\n");
		return -EINVAL;
	}

	res = sbm_destroy(sbm);
	if (res != 0) {
		SBM_PRF("Destroying SBM fail\n");
		return -EINVAL;
	}

	sbm_desc = (struct sbm_desc_uni *)(sbm->priv_data);
	if (sbm_desc == NULL) {
		SBM_PRF("SBM driver data NULL\n");
		return -EINVAL;
	}

	if (sbm_desc->sbm_rw)
		ali_rpc_free_shared_mm(sbm_desc->sbm_rw,
			sizeof(struct sbm_rw_desc_uni));

	dma_unmap_single(NULL, (dma_addr_t)sbm_desc->sbm_cfg.buf_start,
		sbm->buf_size, DMA_TO_DEVICE);

	kfree(sbm_desc);

	sbm->id = -1;
	sbm->read_pointer = NULL;
	sbm->write_pointer = NULL;
	sbm->priv_data = NULL;

	return 0;
}
EXPORT_SYMBOL(see_sbm_destroy);

/**
 * sbm_in_crange() - Check if an item is inside a cyclic range
 * @start_ptr: pointer to the beginning of the range
 * @item_ptr: pointer to the item in question
 * @end_ptr: pointer to the first byte after the range
 *
 * Returns true if item_ptr points to an address between start_ptr and end_ptr
 * inside a cyclic buffer, i.e. in the following cases:
 *
 *  [x & y]                               item_ptr
 *                                           v
 *            |.................============================.........|
 *                              ^                           ^
 *                          start_ptr                    end_ptr
 *
 *  [x & z]                                                  item_ptr
 *                                                              v
 *            |================.........................=============|
 *                             ^                        ^
 *                          end_ptr                 start_ptr
 *
 *  [y & z]           item_ptr
 *                       v
 *            |================.........................=============|
 *                             ^                        ^
 *                          end_ptr                 start_ptr
 *
 */
static inline int sbm_in_crange(void * const start_ptr,
		void * const item_ptr,
		void * const end_ptr)
{
	int x = (start_ptr <= item_ptr);
	int y = (item_ptr < end_ptr);
	int z = (end_ptr < start_ptr);

	return (x & y) | (x & z) | (y & z);
}

/**
 * sbm_next_item() - Calculate next write pointer after adding a new item
 * @read_ptr: current read pointer
 * @write_ptr: current write pointer
 * @item_len: length of item to add
 * @buf_start: pointer to the start of ring buffer
 * @buf_end: pointer to the first byte after the ring buffer
 *
 * Returns the value the write pointer should have after adding the item
 * or NULL if there is not enough free space in the ring buffer to add the
 * item.
 */
static inline void *sbm_next_item(void * const read_ptr,
		void * const write_ptr, const size_t item_len,
		void * const buf_start, void * const buf_end)
{
	char *ret = (char *)write_ptr + item_len;

	if ((void *)ret >= buf_end)
		ret = ret - (char *)buf_end + (char *)buf_start;

	if (sbm_in_crange(read_ptr, ret, write_ptr))
		ret = NULL;

	return ret;
}

/**
 * sbm_empty() - Check if cyclic buffer is empty
 * @read_ptr: current read pointer
 * @write_ptr: current write pointer
 */
static inline int sbm_empty(void * const read_ptr, void * const write_ptr)
{
	return read_ptr == write_ptr;
}

int see_enqueue_sbm_entry(struct see_sbm *sbm, void *data, size_t size,
			struct see_sbm_entry *entry)
{
	struct sbm_desc_uni *sbm_desc = NULL;
	struct sbm_rw_desc_uni *sbm_rw = NULL;
	size_t copy_size = 0;
	u8 *copy_addr = NULL;
	u8 *read_ptr = NULL;
	u8 *write_ptr = NULL;
	u8 *next_write_ptr = NULL;
	u8 *buf_start = NULL;
	u8 *buf_end = NULL;

	if ((sbm == NULL) || (entry == NULL) || (data == NULL) || (size == 0)) {
		SBM_PRF("Enqueuing SBM with parameter error\n");
		return -EINVAL;
	}

	sbm_desc = (struct sbm_desc_uni *)(sbm->priv_data);
	if (sbm_desc == NULL) {
		SBM_PRF("SBM driver data NULL\n");
		return -EINVAL;
	}

	sbm_rw = (struct sbm_rw_desc_uni *)sbm_desc->sbm_rw;
	if (sbm_rw == NULL) {
		SBM_PRF("SBM rw desc NULL\n");
		return -EINVAL;
	}

	down(&sbm_desc->sema);

	read_ptr  = (u8 *)phys_to_virt((u32)sbm_desc->local_read);
	write_ptr = (u8 *)phys_to_virt((u32)sbm_rw->write);
	buf_start = (u8 *)phys_to_virt((u32)sbm_desc->sbm_cfg.buf_start);
	buf_end   = (u8 *)phys_to_virt((u32)sbm_desc->sbm_cfg.buf_end);

	next_write_ptr = sbm_next_item(read_ptr, write_ptr, size,
						buf_start, buf_end);
	if (next_write_ptr != NULL) {
		if (write_ptr + size > buf_end) {
			copy_addr = data;
			copy_size = buf_end - write_ptr;
			memcpy((void *)write_ptr, copy_addr, copy_size);
			dma_sync_single_for_device(NULL,
				virt_to_phys(write_ptr),
				copy_size, DMA_TO_DEVICE);

			write_ptr = buf_start;

			copy_addr += copy_size;
			copy_size = size - copy_size;
			memcpy((void *)write_ptr, copy_addr, copy_size);
			dma_sync_single_for_device(NULL,
				virt_to_phys(write_ptr),
				copy_size, DMA_TO_DEVICE);
		} else {
			memcpy((void *)write_ptr, data, size);
			dma_sync_single_for_device(NULL,
				virt_to_phys(write_ptr),
				size, DMA_TO_DEVICE);
		}

		entry->entry = (void *)sbm_rw->write;
		entry->size = size;
		sbm_rw->write = (u8 *)virt_to_phys(next_write_ptr);

		up(&sbm_desc->sema);
		return 0;
	} else {
		up(&sbm_desc->sema);
		return -EBUSY;
	}
}
EXPORT_SYMBOL(see_enqueue_sbm_entry);

int see_query_sbm_entry(struct see_sbm *sbm, struct see_sbm_entry *sbm_entry)
{
	struct sbm_desc_uni *sbm_desc = NULL;
	struct sbm_rw_desc_uni *sbm_rw = NULL;
	u8 *read_ptr = NULL;
	u8 *write_ptr = NULL;
	u8 *item_ptr = NULL;
	u8 *buf_start = NULL;
	u8 *buf_end = NULL;
	u8 *local_read_ptr = NULL;
	int res = 0;

	if ((sbm == NULL) || (sbm_entry == NULL)
		|| (sbm_entry->entry == NULL)) {
		SBM_PRF("Querying SBM with parameter error\n");
		return -EINVAL;
	}

	sbm_desc = (struct sbm_desc_uni *)(sbm->priv_data);
	if (sbm_desc == NULL) {
		SBM_PRF("SBM driver data NULL\n");
		return -EINVAL;
	}

	sbm_rw = (struct sbm_rw_desc_uni *)sbm_desc->sbm_rw;
	if (sbm_rw == NULL) {
		SBM_PRF("SBM rw desc NULL\n");
		return -EINVAL;
	}

	read_ptr  = (u8 *)phys_to_virt((u32)sbm_rw->read);
	write_ptr = (u8 *)phys_to_virt((u32)sbm_rw->write);
	item_ptr  = (u8 *)phys_to_virt((u32)sbm_entry->entry);
	buf_start = (u8 *)phys_to_virt((u32)sbm_desc->sbm_cfg.buf_start);
	buf_end   = (u8 *)phys_to_virt((u32)sbm_desc->sbm_cfg.buf_end);
	local_read_ptr = (u8 *)phys_to_virt((u32)sbm_desc->local_read);

	if (sbm_empty(read_ptr, write_ptr))
		res = 0;
	else if (sbm_in_crange(write_ptr, item_ptr, read_ptr))
		res = 0;
	else
		res = -EBUSY;

	if (res == 0) {
		if (sbm_in_crange(local_read_ptr, item_ptr, read_ptr)) {
			local_read_ptr = item_ptr + sbm_entry->size;

			if (local_read_ptr >= buf_end)
				local_read_ptr -= (buf_end - buf_start);

		    sbm_desc->local_read = (u8 *)virt_to_phys(local_read_ptr);
		}
	}

	return res;
}
EXPORT_SYMBOL(see_query_sbm_entry);

MODULE_AUTHOR("ALi Corporation");
MODULE_DESCRIPTION("ALi Share Buffer Manager");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");

