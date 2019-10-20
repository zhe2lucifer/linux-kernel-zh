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
#include <ali_cache.h>
#include <alidefinition/adf_scr.h>
#include <ca_dsc.h>
#include "ca_scr_priv.h"
#include "ca_scr_session_engine.h"

#define SCR_BUFFER_IN_PLACE_PROCESSING

static struct workqueue_struct *scr_work_queue;

#define ENGINE_TYPE_RAW (0)
#define ENGINE_TYPE_TS188 (188)
#define ENGINE_TYPE_TS200 (200)

static struct ca_scr_se_buffer *scr_engine_get_buffer(
	struct ca_scr_se *engine, struct page *page)
{
	struct ca_scr_se_buffer *cb;
	int type[] = {
		[CA_FORMAT_RAW] = ENGINE_TYPE_RAW,
		[CA_FORMAT_TS188] = ENGINE_TYPE_TS188,
		[CA_FORMAT_TS188_LTSID] = ENGINE_TYPE_TS188,
		[CA_FORMAT_TS200] = ENGINE_TYPE_TS200,
	};

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
	cb->dev = engine->session->scr->dev;

	cb->id = atomic_inc_return(&engine->buf_count);

	/* round down buffer size */
	cb->size = PAGE_SIZE;

	cb->type = type[engine->session->format];

	cb->i_off = 0;
	cb->o_off = 0;
	cb->len = 0;
	cb->parent = (void *)engine;

	dev_dbg(cb->dev, "scr_se: allocated buffer#%d (size: %d)\n",
			 cb->id, cb->size);

	return cb;

err_mem:
	if (cb->page)
		put_page(cb->page);

	kfree(cb);
	return NULL;
}


static void scr_engine_put_buffer(struct ca_scr_se_buffer *cb)
{
	if (!cb)
		return;

	dev_dbg(cb->dev, "scr_se: release buffer#%d\n", cb->id);

	if (cb->page)
		put_page(cb->page);

	if (cb->opage)
		put_page(cb->opage);

	kfree(cb);
}

static int scr_engine_set_buffer(struct ca_scr_se_buffer *eb,
	int i_off, int len)
{
	struct ca_scr_se *engine = eb->parent;

	eb->len = len;
	eb->i_off = i_off;

	if (engine->in_place)
		eb->o_off = i_off;

	return 0;
}

static int scr_engine_is_empty_queue(struct ca_scr_se *engine)
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
static int scr_engine_is_busy(struct ca_scr_se *engine)
{
	int ret = -EBUSY;
	struct ca_scr_se_buffer *sbuf = NULL;

	mutex_lock(&engine->queue_lock);

	if (!engine->queued_buffers) {
		/* no buffer in the Engine */
		ret = 0;
	} else {
		/*check the last buffer process or not*/
		sbuf = list_entry(engine->buf_queue.prev,
			struct ca_scr_se_buffer, node);
		if (!sbuf) {
			dev_dbg(engine->session->scr->dev,
				"engine: Session#%d last buffer un-check!!\n",
			engine->session->id);

			ret = -EFAULT;
			goto out;
		}

		if (!scr_query_sbm_entry(engine->session, &sbuf->entry))
			ret = 0;
	}

out:
	mutex_unlock(&engine->queue_lock);
	return ret;
}

static int scr_engine_push_buffer(struct ca_scr_se_buffer *sbuf)
{
	struct ca_scr_se *engine = sbuf->parent;

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


static struct ca_scr_se_ops engine_ops = {
	.get_buffer = scr_engine_get_buffer,
	.put_buffer = scr_engine_put_buffer,
	.set_buffer = scr_engine_set_buffer,
	.is_empty = scr_engine_is_empty_queue,
	.is_busy = scr_engine_is_busy,
	.push_buffer = scr_engine_push_buffer,
};

static int ca_scr_se_crypt_buffer(
		struct ca_scr_se_buffer *sbuf,
		struct ca_scr_session *s)
{
	struct ca_scr_se *engine;

	if (!s)
		return -EFAULT;

	engine = (struct ca_scr_se *) sbuf->parent;
	if (!engine)
		return -EFAULT;

#ifdef CONFIG_DEBUG_FS
	if (engine->session->scr->not_gothrough_hw) {
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

	dev_dbg(s->scr->dev,
		"scr_se: session#%d buffer#%d contains %d bytes to crypt\n",
		s->id, sbuf->id, sbuf->len);

	sbuf->dma_hdl = dma_map_page(sbuf->dev, sbuf->page,
		sbuf->i_off, sbuf->len, DMA_BIDIRECTIONAL);

	if (!engine->in_place)
		sbuf->odma_hdl = dma_map_page(sbuf->dev, sbuf->opage,
			sbuf->o_off, sbuf->len, DMA_BIDIRECTIONAL);

	if (s->crypt_mode == CA_ENCRYPT)
		sbuf->pkt.crypto = SCR_ENCRYPT;
	else
		sbuf->pkt.crypto = SCR_DECRYPT;

	sbuf->pkt.sess_id = s->see_sess_id;
	sbuf->pkt.input = (unsigned char *)sbuf->dma_hdl;
	sbuf->pkt.output = (engine->in_place) ?
		(unsigned char *)sbuf->dma_hdl :
		(unsigned char *)sbuf->odma_hdl;
	sbuf->pkt.len = sbuf->type ? sbuf->len / sbuf->type : sbuf->len;
	sbuf->pkt.type = sbuf->type;

	dma_sync_single_for_device(NULL,
		virt_to_phys(&sbuf->pkt),
		sizeof(struct scr_sbm_packet),
		DMA_TO_DEVICE);

	sbuf->ppkt = &sbuf->pkt;
	return scr_write_sbm(s, (char *)&sbuf->ppkt,
		sizeof(struct scr_sbm_packet *), &sbuf->entry);
}


static void ca_sce_se_crypt(struct work_struct *w)
{
	struct ca_scr_se *engine;
	struct ca_scr_se_buffer *b;

	b = container_of(w, struct ca_scr_se_buffer, work);

	engine = (struct ca_scr_se *) b->parent;
	if (!engine)
		return;

	if (b->len == 0) {
		dev_dbg(engine->session->scr->dev,
			"session#%d buffer#%d, len 0, something wrong...\n",
			engine->session->id,
			b->id);

		return;
	}

	dev_dbg(engine->session->scr->dev,
			"scr_se: session#%d buffer#%d ready to encrypt\n",
			engine->session->id,
			b->id);

	/* perform crypto */
	mutex_lock(&engine->queue_lock);
	ca_scr_se_crypt_buffer(b, engine->session);
	mutex_unlock(&engine->queue_lock);

	dev_dbg(engine->session->scr->dev,
			"scr_se: session#%d buffer#%d ready to go out\n",
			engine->session->id,
			b->id);

	wake_up(&engine->OutWq);
}

int ca_scr_se_wr_avail(struct ca_scr_se *engine)
{
	int ret;

	mutex_lock(&engine->queue_lock);

	ret = (engine->queued_buffers < SCR_SBM_NR_NODES);

	mutex_unlock(&engine->queue_lock);

	return ret;
}

int ca_scr_se_queue_last(struct ca_scr_se *engine)
{
	int ret = 0;
	mutex_lock(&engine->queue_lock);

	if (!engine->last) {
		dev_dbg(engine->session->scr->dev,
			"engine: Session#%d no last buffer\n",
			engine->session->id);

		ret = -EFAULT;
		goto out;
	}

	if (engine->last->queued) {
		dev_dbg(engine->session->scr->dev,
			"engine: Session#%d buffer#%d queued aready!!\n",
			engine->session->id, engine->last->id);

		ret = -EFAULT;
		goto out;
	}

	queue_work(scr_work_queue, &engine->last->work);
	engine->last->queued = 1;
	engine->last = NULL;

out:
	mutex_unlock(&engine->queue_lock);

	return ret;
}


int ca_scr_se_enqueue_buffer(struct ca_scr_se_buffer *sbuf)
{
	struct ca_scr_se *engine = sbuf->parent;

	if (!engine)
		return -EFAULT;

	dev_dbg(engine->session->scr->dev,
			"engine: enqueue buffer#%d for session#%d\n",
			sbuf->id, engine->session->id);

	/* lock list */
	mutex_lock(&engine->queue_lock);

	/* increase total of queued buffers */
	engine->queued_buffers++;
	engine->queued_bytes += sbuf->len;

	engine->total_enqueued_buffers++;
	engine->total_enqueued_bytes += sbuf->len;

	/* list add */
	list_add_tail(&sbuf->node, &engine->buf_queue);

	/* init work */
	INIT_WORK(&sbuf->work, ca_sce_se_crypt);

	if (engine->last) {
		/* New buffer comes,
		queue_work previous enqueued buffer anyway*/
		if (!engine->last->queued) {
			queue_work(scr_work_queue, &engine->last->work);
			engine->last->queued = 1;
			engine->last = NULL;
		} else {
			dev_dbg(engine->session->scr->dev,
			"engine: Session#%d buffer#%d queued aready!!\n",
			engine->session->id, engine->last->id);
		}
	}

	/* queue work if uncork*/
	if (!engine->session->ts_chaining ||
		engine->session->opt == CA_SET_UNCORK) {
		queue_work(scr_work_queue, &sbuf->work);
		sbuf->queued = 1;
	} else if (engine->session->ts_chaining &&
		engine->session->opt == CA_SET_CORK) {
		/*new buffer work later*/
		engine->last = sbuf;
	} else {
		dev_dbg(engine->session->scr->dev,
			"engine: Something wrong!!!\n");
	}

	/* unlock list */
	mutex_unlock(&engine->queue_lock);

	return 0;
}

struct ca_scr_se_buffer *ca_scr_se_dequeue_buffer(
		struct ca_scr_se *engine)
{
	struct ca_scr_se_buffer *sbuf = NULL;

	/* lock list queue */
	mutex_lock(&engine->queue_lock);

	sbuf = list_first_entry_or_null(&engine->buf_queue,
			struct ca_scr_se_buffer, node);

	if (!sbuf) {
		mutex_unlock(&engine->queue_lock);
		return NULL;
	}

	/* don't dequeue unfinished buffers unless force*/
	if (!sbuf->done && !engine->force) {
		mutex_unlock(&engine->queue_lock);
		return NULL;
	}

	/* In case dequeue the last buffer without queue_work*/
	if (engine->last == sbuf)
		engine->last = NULL;

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

int ca_scr_se_buffer_done(struct ca_scr_se *engine)
{
	struct ca_scr_se_buffer *sbuf;

	mutex_lock(&engine->queue_lock);

	sbuf = list_first_entry_or_null(&engine->buf_queue,
			struct ca_scr_se_buffer, node);

	mutex_unlock(&engine->queue_lock);

	if (sbuf && sbuf->done)
		return 1;

	if (sbuf && sbuf->len) {
		/*reader might be faster than writer*/
		if (!sbuf->entry.entry)
			return 0;

		if (!scr_query_sbm_entry(engine->session, &sbuf->entry)) {
			sbuf->done = 1;

			dma_unmap_page(sbuf->dev, sbuf->dma_hdl,
				sbuf->len, DMA_TO_DEVICE);

			if (!engine->in_place)
				dma_unmap_page(sbuf->dev, sbuf->odma_hdl,
					sbuf->len, DMA_FROM_DEVICE);
		}
	}

	if (sbuf && !sbuf->len)
		sbuf->done = 1;

	return sbuf && sbuf->done;
}

void ca_scr_poll_read_checker(struct work_struct *work)
{
	struct ca_scr_se *engine;
	int ret;

	engine = container_of(container_of(work, struct delayed_work, work),
				struct ca_scr_se, wq_r_checker);

	ret = ca_scr_se_buffer_done(engine);
	if (ret)
		wake_up_interruptible(&engine->OutWq);
	else
		schedule_delayed_work(&engine->wq_r_checker, SCR_SCHE_DELAY);
}

static void ca_scr_poll_write_checker(struct work_struct *work)
{
	struct ca_scr_se *engine;
	int ret;

	engine = container_of(container_of(work, struct delayed_work, work),
				struct ca_scr_se, wq_w_checker);

	ret = ca_scr_se_wr_avail(engine);
	if (ret)
		wake_up_interruptible(&engine->InWq);
	else
		schedule_delayed_work(&engine->wq_w_checker, SCR_SCHE_DELAY);
}

int ca_scr_se_register(struct ca_scr_session *session)
{
	struct ca_scr_se *engine = &session->engine;
	int ret = 0;

	if (!engine)
		return -EFAULT;

	memset(engine, 0, sizeof(*engine));

	/* store ca_dsc session */
	engine->session = session;

	/* register scr sw engine core operations */
	engine->ops = &engine_ops;

	/* init wait queue */
	init_waitqueue_head(&engine->OutWq);
	init_waitqueue_head(&engine->InWq);
	/* init wait queue checker*/
	INIT_DELAYED_WORK(&engine->wq_r_checker,
		ca_scr_poll_read_checker);
	INIT_DELAYED_WORK(&engine->wq_w_checker,
		ca_scr_poll_write_checker);

	/* init buffer list */
	INIT_LIST_HEAD(&engine->buf_queue);
	/* init mutex lock list */
	mutex_init(&engine->queue_lock);

	dev_dbg(session->scr->dev, "se: session#%d registered\n",
			session->id);

	return ret;
}

int ca_scr_se_unregister(struct ca_scr_session *session)
{
	struct ca_scr_se *engine = &session->engine;
	struct ca_scr_se_buffer *sbuf;
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
				ca_scr_se_buffer_done(engine), HZ/100);

		/*engine->force = 1;*/
		sbuf = ca_scr_se_dequeue_buffer(engine);
		if (sbuf)
			engine->ops->put_buffer(sbuf);
	}

	/* init mutex lock list */
	mutex_destroy(&engine->queue_lock);

	dev_dbg(session->scr->dev, "se: session#%d unregistered\n",
			session->id);
	return ret;
}

int ca_scr_wq_create(void)
{
	if (scr_work_queue == NULL)
		scr_work_queue = create_singlethread_workqueue("ca_scr_wq");

	return (scr_work_queue == NULL) ? -EFAULT : 0;
}

void ca_scr_wq_delete(void)
{
	/* flush workqueue */
	flush_workqueue(scr_work_queue);
	/* remove it from system */
	destroy_workqueue(scr_work_queue);
}

