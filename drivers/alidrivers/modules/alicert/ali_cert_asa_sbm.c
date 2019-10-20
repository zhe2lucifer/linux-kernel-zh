/*
 * SBM for CERT ASA
 *
 * Copyright (c) 2015 ALi Corp
 *
 * This file is released under the GPLv2
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/sched.h>
#include <linux/wait.h>

#include "ali_cert_asa_sbm.h"

int cert_asa_sbm_add(struct cert_asa_session *sess)
{
	int ret = -1;

	if (unlikely(!sess || !sess->drv))
		return -EFAULT;

	memset(&sess->sbm_desc, 0,
			sizeof(sess->sbm_desc));
	sess->sbm_desc.id = -1;
	sess->sbm_desc.buf_size = CERT_ASA_NR_NODES * 2 *
			sizeof(struct cert_asa_node *);
	sess->sbm_desc.buf_start = devm_kzalloc(sess->drv->dev,
							sess->sbm_desc.buf_size,
							GFP_KERNEL);

	if (unlikely(!sess->sbm_desc.buf_start))
		return -ENOMEM;

	ret = see_sbm_create(&sess->sbm_desc);
	if (unlikely(ret < 0)) {
		dev_dbg(sess->drv->dev, "c sbm failed: %p, %d\n",
			sess->sbm_desc.buf_start,
			sess->sbm_desc.buf_size);
	} else {
		dev_dbg(sess->drv->dev,
		"got sbm id: %d\n", sess->sbm_desc.id);
	}

	return ret;
}

void cert_asa_sbm_del(struct cert_asa_session *sess)
{
	if (unlikely(!sess || !sess->drv))
		return;

	if (sess->sbm_desc.id >= 0)
		see_sbm_destroy(&sess->sbm_desc);

	if (sess->sbm_desc.buf_start)
		devm_kfree(sess->drv->dev, sess->sbm_desc.buf_start);
}


int cert_asa_sbm_epage(struct cert_asa_session *sess,
	struct cert_asa_page *p)
{
	int ret = -EBUSY;

	if (unlikely(!sess || !p))
		return -EFAULT;

	if ((p->len - p->rh - p->rt < p->packet_size) ||
		(p->flag & CERT_ASA_PAGE_ENQUEUED))
		return 0;

	p->pnode = &p->node;
	p->pnode->size = p->len - p->rh - p->rt;

	/*
	 * length and address should be aligned to the cache line for arm
	*/
	p->addr = dma_map_page(sess->drv->dev, p->page, 0,
					PAGE_SIZE, DMA_BIDIRECTIONAL);

	p->pnode->output = p->addr + p->offset +
					(p->offset_fixed ? 0 : p->rh);
	p->pnode->input = p->pnode->output;
	if (p->page_in) {
		p->addr_in = dma_map_page(sess->drv->dev,
				p->page_in, 0, PAGE_SIZE, DMA_TO_DEVICE);
		p->pnode->input =
			p->addr_in + p->offset_in + p->rh;
	}

	dev_dbg(sess->drv->dev,
		"enqueue:page[%p]|addr[%x]|off[%d]-->page[%p]|addr[%x]|off[%d], "
		"rh[%d], rt[%d], len[%d], "
		"p->pnode->size[%d]\n",
		p->page_in ? p->page_in : p->page,
		p->page_in ? p->addr_in : p->addr,
		p->page_in ? (p->offset_in + p->rh) :
			(p->offset + (p->offset_fixed ? 0 : p->rh)),
		p->page, p->addr, (p->offset + (p->offset_fixed ? 0 : p->rh)),
		p->rh, p->rt, p->len,
		p->pnode->size);

	dma_sync_single_for_device(NULL,
		virt_to_phys(&p->node),
		sizeof(struct cert_asa_node),
		DMA_TO_DEVICE);

	while (1) {
		ret = wait_event_interruptible_timeout(sess->wq_sbm,
			!see_enqueue_sbm_entry(&sess->sbm_desc, &p->pnode,
				sizeof(struct cert_asa_node *), &p->sbm_entry),
				CERT_ASA_WAIT_INTERVAL);
		if (likely(ret > 0))
			break;
		else if (ret < 0)
			return ret;
	}

	p->flag |= CERT_ASA_PAGE_ENQUEUED;

	if (!(atomic_inc_return(&sess->sbm_c) %
		(CERT_ASA_NR_NODES)))
		cert_asa_sbm_qpage(sess, p);

	return 0;
}

int cert_asa_sbm_qpage(struct cert_asa_session *sess,
	struct cert_asa_page *p)
{
	int ret = -EBUSY;
	int block = (*sess->f_flags & O_NONBLOCK) ? 0 : 1;

	if (unlikely(!sess || !p))
		return -EFAULT;

	dev_dbg(sess->drv->dev,
		"query p-page:%p-%p, addr:0x%x, rh&rt:0x%x-%x, offset&len:0x%x-%x\n",
		p, p->page, p->addr, p->rh, p->rt, p->offset, p->len);

	if ((p->flag & CERT_ASA_PAGE_QUERIED) ||
		(p->len - p->rh - p->rt < p->packet_size))
		return 0;

	if (unlikely(!(p->flag & CERT_ASA_PAGE_ENQUEUED)))
		return -EFAULT;

	do {
		ret = wait_event_interruptible_timeout(sess->wq_sbm,
			!see_query_sbm_entry(&sess->sbm_desc, &p->sbm_entry),
			CERT_ASA_WAIT_INTERVAL);
		if (likely(ret))
			break;
	} while (block);

	dma_unmap_page(sess->drv->dev, p->addr,
					PAGE_SIZE, DMA_FROM_DEVICE);
	if (p->page_in)
		dma_unmap_page(sess->drv->dev, p->addr_in,
					PAGE_SIZE, DMA_TO_DEVICE);

	if (ret > 0)
		p->flag |= CERT_ASA_PAGE_QUERIED;

	return (ret > 0) ? 0 : -ERESTARTSYS;
}

