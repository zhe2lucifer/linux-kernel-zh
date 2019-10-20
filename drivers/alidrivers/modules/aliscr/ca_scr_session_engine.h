/*
 * DeScrambler Core driver
 * Copyright(C) 2014 ALi Corporation. All rights reserved.
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

#ifndef _CA_SCR_SESSION_ENGINE_H_
#define _CA_SCR_SESSION_ENGINE_H_

#include "ca_dsc.h"
#include "ca_scr_priv.h"
#include "ca_scr_sbm.h"

#define SESSION_ENGINE_TS188	188
#define SESSION_ENGINE_TS200	200

struct ca_scr_se;

#ifndef PAGE_SIZE
#define PAGE_SIZE (4096)
#endif

struct ca_scr_se_buffer {
	/* buffer id */
	int id;
	/* buffer block size: max available allocated size */
	int size;

	/* format type : raw, 188,200 */
	int type;

	struct page *page;
	struct page *opage;
	int len;
	unsigned int i_off;
	unsigned int o_off;
	dma_addr_t dma_hdl;
	dma_addr_t odma_hdl;

	/* parent device */
	struct device *dev;
	/* parent private link */
	void *parent;

	/* work struct */
	struct work_struct work;
	/* list entry */
	struct list_head node;
	/* 1 if processing is finished, 0 if processing is pending */
	int done;
	/* sbm entry of this buffer*/
	struct see_sbm_entry entry;
	/* sbm package*/
	struct scr_sbm_packet pkt;
	struct scr_sbm_packet *ppkt;
	/* 1 if this is the last buffer in transfer */
	int last;
	/*1: indicate this sbuf queued work*/
	int queued;
	int rq;
};

struct ca_scr_se_ops {
	/* request free crypto buffer */
	struct ca_scr_se_buffer *(*get_buffer) (struct ca_scr_se *engine,
		struct page *page);
	/* release previously acquired crypto buffer */
	void (*put_buffer) (struct ca_scr_se_buffer *eb);
	/* set buffer len && offset*/
	int (*set_buffer) (struct ca_scr_se_buffer *eb, int i_off, int len);
	/* check queue empty or not*/
	int (*is_empty)(struct ca_scr_se *engine);
	/* check queue busy to update key or parameter or not*/
	int (*is_busy)(struct ca_scr_se *engine);
	/* push buffer at list head*/
	int (*push_buffer)(struct ca_scr_se_buffer *eb);
};

struct scr_buffer_item {
	struct page *page;
	unsigned int pgoffs;    /* offset of the page */
	unsigned int pgfill;    /* fill level of the page */
};

struct scr_rw_buffer_item {
	char buf[PAGE_SIZE];
	unsigned int bfill;
	int boffset;
};

struct ca_scr_se {
	wait_queue_head_t OutWq;
	wait_queue_head_t InWq;
	/* work of wait queue read checking */
	struct delayed_work wq_r_checker;
	/* work of wait queue write checking */
	struct delayed_work wq_w_checker;

	/* total enqueued buffers */
	int total_enqueued_buffers;
	/* total dequeued buffers */
	int total_dequeued_buffers;
	/* total enqueued bytes */
	long long total_enqueued_bytes;
	/* total dequeued bytes */
	long long total_dequeued_bytes;
	/* enqueued buffers */
	int queued_buffers;
	/* enqueued bytes */
	long long queued_bytes;

	long long read_bytes;
	long long write_bytes;
	long long mmap_bytes;
	long long splice_read_bytes;
	long long splice_write_bytes;

	struct ca_scr_se_ops *ops;
	/* ca_scr_session handle */
	struct ca_scr_session *session;
	/* mutex to queue protection */
	struct mutex queue_lock;
	/* queued buffer list */
	struct list_head buf_queue;
	/* 0: in/out buffer seperate, 1: out in-place in buffer*/
	int in_place;
	/*1: force the engine to release unfinished buffer*/
	int force;
	/*last buffer in the queue*/
	struct ca_scr_se_buffer *last;

	struct scr_buffer_item tmpbuf;
	struct scr_rw_buffer_item wr_tmpbuf;
	struct scr_rw_buffer_item rd_tmpbuf;
	atomic_t buf_count;
};

int ca_scr_wq_create(void);
void ca_scr_wq_delete(void);

/*session engine register*/
int ca_scr_se_register(struct ca_scr_session *session);
/*session engine unregister*/
int ca_scr_se_unregister(struct ca_scr_session *session);
int ca_scr_se_wr_avail(struct ca_scr_se *engine);
int ca_scr_se_queue_last(struct ca_scr_se *engine);
int ca_scr_se_enqueue_buffer(struct ca_scr_se_buffer *sbuf);
struct ca_scr_se_buffer *ca_scr_se_dequeue_buffer(
		struct ca_scr_se *engine);
int ca_scr_se_buffer_done(struct ca_scr_se *engine);

#endif /*_CA_SCR_SESSION_ENGINE_H_*/
