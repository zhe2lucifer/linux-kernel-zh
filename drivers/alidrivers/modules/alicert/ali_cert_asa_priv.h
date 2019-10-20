/*
 * Driver structures for CERT ASA
 *
 * Copyright (c) 2015 ALi Corp
 *
 * This file is released under the GPLv2
 */

#ifndef _ALI_CERT_ASA_PRIV_H
#define _ALI_CERT_ASA_PRIV_H

#include <linux/kernel.h>
#include <alidefinition/adf_cert.h>
#include <ca_asa.h>
#include <ali_sbm_client.h>
#include <linux/idr.h>

#include "ali_cert_asa_rpc.h"
#include "ali_cert_asa_splice.h"
#include "ali_cert_utils.h"
#include "ali_cert_priv.h"

#include "../ali_kl_fd_framework/ca_kl_fd_dispatch.h"

#define CERT_ASA_DEV "dsc2"

#define CERT_ASA_TS188_SIZE (188)
#define CERT_ASA_TS200_SIZE (200)

#define CERT_ASA_MAX_ITEM (64)

#define CERT_ASA_MAX_PAGES (CERT_ASA_NR_NODES)

/* Define the page stage */
#define CERT_ASA_PAGE_DEFAULT (0)
#define CERT_ASA_PAGE_ENQUEUED (1)
#define CERT_ASA_PAGE_QUERIED (2)

/**
 * interval 1000us, expired time in block mode is 500ms,
 * and 1000us when non_block mode.
 */
#define CERT_ASA_POLL_INTERVAL (msecs_to_jiffies(20))
#define CERT_ASA_WAIT_INTERVAL (msecs_to_jiffies(20))
#define CERT_ASA_BLOCKED_WAIT_CYCLE (500 / 20) /* 500 ms */
#define CERT_ASA_NONBLOCKED_WAIT_CYCLE (1)

/**
 * expired time in dio sync mode is 1500ms,
 */
#define CERT_ASA_DIO_WAIT_CYCLE (1500)

/**
 * expired time in vm_fault is 500ms,
 */
#define CERT_ASA_VMFAULT_CYCLE (500)

struct pid_node {
	struct ca_pid *pid;
	struct list_head list;
};

struct cert_asa_key {
	int handle;
	int keyfd;
	struct kl_key_cell *cell;
	struct list_head list;

	struct list_head pid_list;
};

struct cert_asa_page {
	struct page *page_in; /* input page from pipe */
	int offset_in; /* data offset within the input page */
	dma_addr_t addr_in; /* dma phys address of the input page */
	size_t packet_size; /* packet_size 188, 200*/
	size_t page_size; /* page size (packet_size aligned) */
	struct page *page;  /* output page to pipe */
	dma_addr_t addr; /* dma phys address of the output page */
	size_t offset; /* data offset within the output page */
	size_t len; /* data length within the output page */
	size_t rh; /* residue len in the input page header */
	size_t rt; /* residue len in the input page tail */
	int flag; /* in which stage */
	int offset_fixed; /*using fixed offset for output page */
	struct see_sbm_entry sbm_entry;
	struct list_head list;
	struct cert_asa_node node;
	struct cert_asa_node *pnode;
};

struct cert_asa_session {
	struct cert_asa_drv *drv;

	struct ida key_ida;
	struct list_head keys;
	struct list_head pages;
	wait_queue_head_t wq_wr;
	wait_queue_head_t wq_rd;

	struct semaphore sem;
	struct semaphore rd_sem;
	struct semaphore wr_sem;

	int see_sess;
	int ts_fmt;
	int ts_fmt_configured;
	size_t packet_size;
	size_t page_size;

	unsigned int *f_flags; /* file flags */

	struct list_head rpages; /* splice residue pages */
	int nr_combined; /* nr of combined residue bytes */

	wait_queue_head_t wq_sbm;
	struct see_sbm sbm_desc;
	atomic_t sbm_c; /* total sbm enqueued count of this session */

	/*statistics*/
	atomic_t page_c; /* total allocated pages count for this session */
	atomic_t nr_bytes; /* number of bytes contained in this session */

	unsigned long long rd_bytes;
	unsigned long long wr_bytes;
	unsigned long long splice_rd_bytes;
	unsigned long long splice_wr_bytes;
	unsigned long long mmap_bytes;

	struct list_head dios;

	struct delayed_work poll_rd;
	struct delayed_work poll_wr;

#ifdef CONFIG_DEBUG_FS
	struct dentry *session_dir;
	struct dentry *debugfs;
	struct dentry *choice;
#endif
};

struct cert_asa_dio {
	struct cert_asa_session *sess;

	wait_queue_head_t wq;
	struct work_struct work;
	struct list_head pages; /* contains dio pages */
	struct list_head rpages;  /* contains residue data crossing two pages */
	size_t nr_pages_in;
	size_t nr_pages_out;

	struct page **in;
	struct page **out;

	size_t packet_size; /* packet_size 188, 200*/
	size_t page_size; /* page size (packet_size aligned) */

	void *user_in;
	void *user_out;
	size_t user_len;

	int done;
	int status;
	int unaligned;

	int async;
	void *eventfd;

	struct list_head list;
};

struct cert_asa_page_ops {
	struct cert_asa_page* (*alloc)(
		struct cert_asa_session *sess);

	int (*free)(struct cert_asa_session *sess,
		struct cert_asa_page *p);

	int (*list_empty) (struct cert_asa_session *sess);

	int (*list_full)(struct cert_asa_session *sess);

	int (*list_add)(struct cert_asa_session *sess,
		struct cert_asa_page *p,
		struct list_head *head);

	void (*list_del)(struct cert_asa_session *sess,
		struct cert_asa_page *p);

	struct cert_asa_page* (*list_first)(
		struct cert_asa_session *sess,
		struct list_head *head);

	struct cert_asa_page* (*list_last)(
		struct cert_asa_session *sess,
		struct list_head *head);

	struct cert_asa_page* (*acquire_first)(
		struct cert_asa_session *sess,
		struct list_head *head);

	struct cert_asa_page* (*acquire_last)(
		struct cert_asa_session *sess,
		struct list_head *head);

	void (*wait_done)(struct cert_asa_session *sess);

	int (*poll_rd)(struct cert_asa_session *sess);

	int (*poll_wr)(struct cert_asa_session *sess);

	size_t (*read_rh)(
		struct cert_asa_session *sess,
		struct cert_asa_page *p,
		struct cert_asa_page **rp,
		size_t size);

	size_t (*read_rt)(
		struct cert_asa_session *sess,
		struct cert_asa_page *p,
		struct cert_asa_page **rp,
		size_t size);
};


struct cert_asa_drv {
	struct semaphore sem;
	dev_t devt;
	struct device *dev;
	struct class *dev_class;
	struct cdev cdev;
	atomic_t nr_keys;
	const struct cert_asa_see_ops *see_ops;
	struct cert_asa_splice_ops splice_ops;
	const struct cert_asa_page_ops *page_ops;
	struct see_client *see_clnt;
	struct cert_driver *parent;
#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_dir;
#endif
};

int cert_asa_probe(struct cert_driver *parent);
int cert_asa_remove(struct cert_driver *parent);

#endif

