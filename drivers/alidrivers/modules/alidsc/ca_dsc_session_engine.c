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
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/crypto.h>
#include <linux/uaccess.h>
#include <linux/idr.h>
#include <linux/pagemap.h>
#include <linux/dma-mapping.h>
#include <linux/highmem.h>
#include <ali_soc.h>

#include "ca_dsc.h"
#include "ca_dsc_priv.h"
#include "ca_dsc_session_engine.h"

static struct workqueue_struct *dsc_work_queue;

#define ENGINE_TYPE_RAW (0)
#define ENGINE_TYPE_TS188 (188)
#define ENGINE_TYPE_TS200 (200)

static struct ca_dsc_se_buffer *engine_get_buffer(
	struct ca_dsc_se *engine, struct page *page)
{
	struct ca_dsc_se_buffer *cb;
	int type[] = {
		[CA_FORMAT_RAW] = ENGINE_TYPE_RAW,
		[CA_FORMAT_TS188] = ENGINE_TYPE_TS188,
		[CA_FORMAT_TS188_LTSID] = ENGINE_TYPE_TS188,
		[CA_FORMAT_TS200] = ENGINE_TYPE_TS200,
	};
	struct ca_dsc_session *s =
		container_of(engine, struct ca_dsc_session, engine);

	cb = kzalloc(sizeof(*cb), GFP_KERNEL);
	if (!cb)
		return NULL;

	/*input page - allocate locally or get from outside*/
	if (!page) {
		cb->page = alloc_page(GFP_KERNEL | __GFP_DMA | __GFP_COLD);
		if (!cb->page)
			goto err_mem;
	} else {
		cb->page = page;
		get_page(cb->page);
	}

	/*output page*/
	if (!engine->in_place) {
		cb->opage = alloc_page(GFP_KERNEL | __GFP_DMA | __GFP_COLD);
		if (!cb->opage)
			goto err_mem;
	}

	/* store parent handle */
	cb->dev = s->dsc->dev;

	cb->id = atomic_inc_return(&engine->buf_count);

	/* round down buffer size */
	cb->size = PAGE_SIZE;

	cb->type = type[s->format];

	cb->i_off = 0;
	cb->o_off = 0;
	cb->len = 0;
	cb->parent = (void *)engine;

	dev_dbg(cb->dev, "dsc_se: allocated buffer#%d (size: %d)\n",
			 cb->id, cb->size);

#ifdef CONFIG_DEBUG_FS
	if (s->dsc->debug_mode)
		list_add_tail(&cb->get_node, &engine->buf_get);
#endif
	return cb;

err_mem:
	if (cb->page)
		put_page(cb->page);

	kfree(cb);
	return NULL;
}

static void engine_put_buffer(struct ca_dsc_se_buffer *cb)
{
	struct ca_dsc_se *engine;
	struct ca_dsc_session *s;

	dev_dbg(cb->dev, "dsc_se: release buffer#%d\n", cb->id);

	engine = (struct ca_dsc_se *)cb->parent;
	s = container_of(engine, struct ca_dsc_session, engine);

#ifdef CONFIG_DEBUG_FS
	if (s->dsc->debug_mode) {
		list_add_tail(&cb->put_node, &engine->buf_put);
		return;
	}
#endif
	if (cb->page)
		put_page(cb->page);

	if (cb->opage)
		put_page(cb->opage);

	memset(cb, 0, sizeof(*cb));
	kfree(cb);
}

static int engine_set_buffer(struct ca_dsc_se_buffer *eb,
	int i_off, int len)
{
	struct ca_dsc_se *engine = eb->parent;

	eb->len = len;
	eb->i_off = i_off;

	if (engine->in_place)
		eb->o_off = i_off;

	return 0;
}

static int engine_is_empty_queue(struct ca_dsc_se *engine)
{
	int ret;

	mutex_lock(&engine->queue_lock);

	ret = list_empty(&engine->buf_queue);

	mutex_unlock(&engine->queue_lock);

	return ret;
}

/*******************************************
	To check whether there are accumulated buffers
	un-processed in the queue, before updating paras

	Return:
		-EBUSY: not available, cannot update parameters
		-EFAULT: errors
		0: available, not busy
*/
static int engine_is_busy(struct ca_dsc_se *engine)
{
	int ret = -EBUSY;
	struct ca_dsc_se_buffer *sbuf = NULL;
	struct ca_dsc_session *s =
		container_of(engine, struct ca_dsc_session, engine);

	mutex_lock(&engine->queue_lock);

	if (!engine->queued_buffers) {
		/* no buffer in the Engine */
		ret = 0;
	} else {
		/*check the last buffer process or not*/
		sbuf = list_entry(engine->buf_queue.prev,
			struct ca_dsc_se_buffer, node);
		if (!sbuf) {
			dev_dbg(s->dsc->dev,
				"engine: Session#%d last buffer un-check!!\n",
				s->id);

			ret = -EFAULT;
			goto out;
		}
		/*check whether the key is from CA_OTP_KEY_FP or not*/
		if (s->parity == CA_PARITY_OTP_KEY_FP) {
			ret = 0;
		}else if (!query_sbm_entry(s, &sbuf->entry))
			ret = 0;
	}

out:
	mutex_unlock(&engine->queue_lock);
	return ret;
}

static int engine_push_buffer(struct ca_dsc_se_buffer *sbuf)
{
	struct ca_dsc_se *engine = sbuf->parent;

	mutex_lock(&engine->queue_lock);

	sbuf->rq = 1;

	engine->queued_buffers++;
	engine->queued_bytes += sbuf->len;

	engine->total_dequeued_buffers--;
	engine->total_dequeued_bytes -= sbuf->len;

	/* add at the list head*/
	list_add(&sbuf->node, &engine->buf_queue);

	mutex_unlock(&engine->queue_lock);

	return 0;
}


static struct ca_dsc_se_ops engine_ops = {
	.get_buffer = engine_get_buffer,
	.put_buffer = engine_put_buffer,
	.set_buffer = engine_set_buffer,
	.is_empty = engine_is_empty_queue,
	.is_busy = engine_is_busy,
	.push_buffer = engine_push_buffer,
};

static int ca_dsc_se_crypt_buffer(struct ca_dsc_se_buffer *sbuf)
{
	struct ca_dsc_se *engine;
	struct ca_dsc_session *s;
	struct AES_CRYPT_WITH_FP aes_crypt_fp;
	int ret = RET_FAILURE;
	engine = (struct ca_dsc_se *) sbuf->parent;
	if (!engine)
		return -EFAULT;

	s = container_of(engine, struct ca_dsc_session, engine);

#ifdef CONFIG_DEBUG_FS
	if (s->dsc->not_gothrough_hw) {
		sbuf->done = 1;

		if (!engine->in_place) {
			memcpy(kmap(sbuf->opage) + sbuf->o_off,
				kmap(sbuf->page) + sbuf->i_off,
				sbuf->len);

			kunmap(sbuf->opage);
			kunmap(sbuf->page);
		}

		return 0;
	}
#endif

	dev_dbg(s->dsc->dev,
		"dsc_se: session#%d buffer#%d contains %d bytes to crypt\n",
		s->id, sbuf->id, sbuf->len);

	sbuf->dma_hdl = dma_map_page(sbuf->dev, sbuf->page,
		sbuf->i_off, sbuf->len, DMA_TO_DEVICE);

	if (!engine->in_place)
		sbuf->odma_hdl = dma_map_page(sbuf->dev, sbuf->opage,
			sbuf->o_off, sbuf->len, DMA_FROM_DEVICE);

	sbuf->pkt.crypto = (CA_ENCRYPT == s->crypt_mode) ?
					DSC_ENCRYPT : DSC_DECRYPT;
	sbuf->pkt.algo = s->sub_module;
	sbuf->pkt.sid = s->stream_id;
	sbuf->pkt.dev_hdl = s->sub_dev_see_hdl;
	sbuf->pkt.input = (unsigned char *)sbuf->dma_hdl;
	sbuf->pkt.output = (engine->in_place) ?
		(unsigned char *)sbuf->dma_hdl :
		(unsigned char *)sbuf->odma_hdl;
	sbuf->pkt.len = sbuf->type ? sbuf->len / sbuf->type : sbuf->len;
	sbuf->pkt.type = sbuf->type;

	dma_sync_single_for_device(NULL,
		virt_to_phys(&sbuf->pkt),
		sizeof(struct dsc_sbm_packet),
		DMA_TO_DEVICE);

	sbuf->ppkt = (struct dsc_sbm_packet *)virt_to_phys(&sbuf->pkt);

	/*It's wired for ARM chips like C3921, to get phys addr 0x8xxx_xxxx
	when virt is 0xCxxx_xxx by virt_to_phys().

	The phys should be 0x0xxx_xxxx.
	*/
	if (ali_sys_ic_get_chip_id() == ALI_C3921)
		sbuf->ppkt = (struct dsc_sbm_packet *)((unsigned long)sbuf->ppkt & (~0x80000000));
		
	/*When key is from CA_OTP_KEY_FP, driver uses remote call to do encryption,
	* others use sbm to do operaion
	*/
	if (s->parity == CA_PARITY_OTP_KEY_FP)
	{
		memset(&aes_crypt_fp, 0, sizeof(aes_crypt_fp));
		aes_crypt_fp.crypt_mode = DSC_ENCRYPT;
		aes_crypt_fp.input = sbuf->pkt.input;
		ret = ali_aes_ioctl(s->dsc, (AES_DEV *)s->sub_dev_see_hdl, 
			DSC_IO_CMD(IO_AES_CRYPT_WITH_FP),
			(__u32)&aes_crypt_fp);
		if (ret != RET_SUCCESS) {
			dev_dbg(s->dsc->dev, "ERR AES_CRYPT_WITH_FP-%x\n", ret);
			ret = -EIO;
		}
		memcpy(kmap(sbuf->opage) + sbuf->o_off, 
			aes_crypt_fp.output, 16);
		kunmap(sbuf->opage);
		sbuf->done = 1;
		return ret;
	}
	else
	{
		return write_sbm(s, (char *)&sbuf->ppkt, sizeof(struct dsc_sbm_packet *), &sbuf->entry);
	}
}

#if 0
static void ca_dsc_se_crypt(struct work_struct *w)
{
	struct ca_dsc_se *engine;
	struct ca_dsc_se_buffer *b;
	struct ca_dsc_session *s;

	b = container_of(w, struct ca_dsc_se_buffer, work);

	engine = (struct ca_dsc_se *) b->parent;
	if (!engine)
		return;

	s = container_of(engine, struct ca_dsc_session, engine);

	if (b->len == 0) {
		dev_dbg(s->dsc->dev,
			"session#%d buffer#%d, len 0, something wrong...\n",
			s->id,
			b->id);

		return;
	}

	dev_dbg(s->dsc->dev,
			"dsc_se: session#%d buffer#%d ready to crypt\n",
			s->id,
			b->id);

	/* perform crypto */
	ca_dsc_se_crypt_buffer(b);

	dev_dbg(s->dsc->dev,
			"dsc_se: session#%d buffer#%d ready to go out\n",
			s->id,
			b->id);

	wake_up(&engine->OutWq);
}
#endif

int ca_dsc_se_enqueue_buffer(struct ca_dsc_se_buffer *sbuf)
{
	struct ca_dsc_se *engine = sbuf->parent;
	struct ca_dsc_session *s =
		container_of(engine, struct ca_dsc_session, engine);

	if (!engine)
		return -EFAULT;

	dev_dbg(s->dsc->dev,
			"engine: enqueue buffer#%d for session#%d\n",
			sbuf->id, s->id);

	ca_dsc_se_crypt_buffer(sbuf);

	/* lock list */
	mutex_lock(&engine->queue_lock);

	/* increase total of queued buffers */
	engine->queued_buffers++;
	engine->queued_bytes += sbuf->len;

	engine->total_enqueued_buffers++;
	engine->total_enqueued_bytes += sbuf->len;

	list_add_tail(&sbuf->node, &engine->buf_queue);

#if 0
	/* init work */
	INIT_WORK(&sbuf->work, ca_dsc_se_crypt);

	/* queue work immediately */
	queue_work(dsc_work_queue, &sbuf->work);
#endif

	mutex_unlock(&engine->queue_lock);

	/* wake up reader */
	wake_up(&engine->OutWq);

	return 0;
}

struct ca_dsc_se_buffer *ca_dsc_se_dequeue_buffer(
		struct ca_dsc_se *engine)
{
	struct ca_dsc_se_buffer *sbuf = NULL;

	/* lock list queue */
	mutex_lock(&engine->queue_lock);

	sbuf = list_first_entry_or_null(&engine->buf_queue,
			struct ca_dsc_se_buffer, node);

	if (!sbuf) {
		mutex_unlock(&engine->queue_lock);
		return NULL;
	}

	/* don't dequeue unfinished buffers unless force*/
	if (!sbuf->done && !engine->force) {
		mutex_unlock(&engine->queue_lock);
		return NULL;
	}

	/* remove current node */
	list_del_init(&sbuf->node);
	/* decrease counter */
	engine->queued_buffers--;
	engine->queued_bytes -= sbuf->len;

	engine->total_dequeued_buffers++;
	engine->total_dequeued_bytes += sbuf->len;

	/* unlock list */
	mutex_unlock(&engine->queue_lock);

	/* wake up */
	wake_up(&engine->InWq);

	return sbuf;
}

int ca_dsc_se_buffer_done(struct ca_dsc_se *engine)
{
	int ret = 0;
	struct ca_dsc_se_buffer *sbuf, *next;
	struct ca_dsc_session *s =
		container_of(engine, struct ca_dsc_session, engine);

	mutex_lock(&engine->queue_lock);

	sbuf = list_first_entry_or_null(&engine->buf_queue,
			struct ca_dsc_se_buffer, node);
	if (!sbuf)
		goto unlock;

	if (sbuf->done && sbuf->wait_next_done) {
		ret = 1;
		goto unlock;
	}

	/*check next sbuf done or not if necessary -- If the session in CORK mode,
	the current sbuf need to be corked until the next sbuf done, because some
	partial data of current sbuf will be processed with next sbuf data by HW*/
	if (s->opt == CA_SET_CORK) {
	next = list_first_entry_or_null(&sbuf->node,
		struct ca_dsc_se_buffer, node);
		if (!next) {
			/*current sbuf is the last, but still waiting uncork command*/
			sbuf->wait_next_done = 0;
			dev_dbg(s->dsc->dev, "cork, next sbuf null!\n");
			goto unlock;
		}

		if (!next->entry.entry) {
			/*next sbut has not queue_work() yet*/
			sbuf->wait_next_done = 0;
			dev_dbg(s->dsc->dev, "cork, next sbuf not sbm enqueue!\n");
			goto unlock;
		}

		if (!query_sbm_entry(s, &next->entry)) {
			/*this is expected*/
			sbuf->wait_next_done = 1;
		} else {
			sbuf->wait_next_done = 0;
			dev_dbg(s->dsc->dev, "cork, next sbuf not done!\n");
			goto unlock;
		}
	} else if (s->opt == CA_SET_UNCORK) {
		sbuf->wait_next_done = 1;
	}

	/*check current sbuf done or not*/
	if (sbuf->len) {
		/*reader might be faster than writer*/
		if (!sbuf->entry.entry) {
			ret = 0;
			goto unlock;
		}

		if (!query_sbm_entry(s, &sbuf->entry)) {
			sbuf->done = 1;

			dma_unmap_page(sbuf->dev, sbuf->dma_hdl,
				sbuf->len, DMA_TO_DEVICE);

			if (!engine->in_place)
				dma_unmap_page(sbuf->dev, sbuf->odma_hdl,
					sbuf->len, DMA_FROM_DEVICE);
		}
	} else {
		sbuf->done = 1;
	}

	ret = sbuf->done && sbuf->wait_next_done;

unlock:
	mutex_unlock(&engine->queue_lock);
	return ret;
}

static void ca_dsc_poll_read_checker(struct work_struct *work)
{
	struct ca_dsc_se *engine;
	int ret;

	engine = container_of(container_of(work, struct delayed_work, work),
				struct ca_dsc_se, wq_r_checker);

	ret = ca_dsc_se_buffer_done(engine);
	if (ret)
		wake_up_interruptible(&engine->OutWq);
	else
		schedule_delayed_work(&engine->wq_r_checker, 10);
}

static void ca_dsc_poll_write_checker(struct work_struct *work)
{
	struct ca_dsc_se *engine;
	int ret;

	engine = container_of(container_of(work, struct delayed_work, work),
				struct ca_dsc_se, wq_w_checker);

	ret = ca_dsc_se_wr_avail(engine);
	if (ret)
		wake_up_interruptible(&engine->InWq);
	else
		schedule_delayed_work(&engine->wq_w_checker, 10);
}


int ca_dsc_se_register(struct ca_dsc_session *session)
{
	struct ca_dsc_se *engine = &session->engine;
	int ret = 0;

	if (!engine)
		return -EFAULT;

	memset(engine, 0, sizeof(*engine));

	/* register dsc sw engine core operations */
	engine->ops = &engine_ops;

	/* init wait queue */
	init_waitqueue_head(&engine->OutWq);
	init_waitqueue_head(&engine->InWq);
	/* init wait queue checker*/
	INIT_DELAYED_WORK(&engine->wq_r_checker,
		ca_dsc_poll_read_checker);
	INIT_DELAYED_WORK(&engine->wq_w_checker,
		ca_dsc_poll_write_checker);

	/* init buffer list */
	INIT_LIST_HEAD(&engine->buf_queue);
#ifdef CONFIG_DEBUG_FS
	INIT_LIST_HEAD(&engine->buf_get);
	INIT_LIST_HEAD(&engine->buf_put);
#endif
	/* init mutex lock list */
	mutex_init(&engine->queue_lock);

	dev_dbg(session->dsc->dev, "se: session#%d registered\n",
			session->id);
	return ret;
}

int ca_dsc_se_unregister(struct ca_dsc_session *session)
{
	struct ca_dsc_se *engine = &session->engine;
	struct ca_dsc_se_buffer *sbuf;
	int ret = 0;

	if (!engine)
		return -EFAULT;

	cancel_delayed_work_sync(&engine->wq_r_checker);
	cancel_delayed_work_sync(&engine->wq_w_checker);

	/*
	 * cancel the work and manually free the buffers
	 */
	while (!list_empty(&engine->buf_queue)) {
		wait_event_timeout(engine->OutWq,
				ca_dsc_se_buffer_done(engine), HZ/100);

		engine->force = 1;
		sbuf = ca_dsc_se_dequeue_buffer(engine);
		if (sbuf)
			engine->ops->put_buffer(sbuf);
	}

	/* init mutex lock list */
	mutex_destroy(&engine->queue_lock);

	dev_dbg(session->dsc->dev, "se: session#%d unregistered\n",
			session->id);
	return ret;
}

int ca_dsc_wq_create(void)
{
	if (dsc_work_queue == NULL)
		dsc_work_queue = create_singlethread_workqueue("ca_dsc_wq");

	return (dsc_work_queue == NULL) ? -EFAULT : 0;
}

void ca_dsc_wq_delete(void)
{
	/* flush workqueue */
	flush_workqueue(dsc_work_queue);
	/* remove it from system */
	destroy_workqueue(dsc_work_queue);
}

