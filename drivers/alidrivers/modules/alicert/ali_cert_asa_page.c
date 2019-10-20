/*
 * Some page utils for ASA
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
#include <linux/splice.h>
#include <linux/file.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/pagemap.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/kallsyms.h>
#include <linux/sched.h>

#include "ali_cert_asa_priv.h"
#include "ali_cert_asa_page.h"
#include "ali_cert_asa_sbm.h"

static struct cert_asa_page *cert_asa_alloc_page
(
	struct cert_asa_session *sess
)
{
	struct cert_asa_page *p = NULL;

	p = kzalloc(sizeof(struct cert_asa_page),
				GFP_KERNEL);
	if (unlikely(!p))
		return NULL;

	p->page = alloc_page(GFP_KERNEL | GFP_DMA);
	if (unlikely(!p->page)) {
		dev_dbg(sess->drv->dev, "alloc page failed!\n");
		kfree(p);
		return NULL;
	}

	p->page_size = sess->page_size;
	p->packet_size = sess->packet_size;
	p->flag = CERT_ASA_PAGE_DEFAULT;
	atomic_inc(&sess->page_c);

	return p;
}

static int cert_asa_free_page
(
	struct cert_asa_session *sess,
	struct cert_asa_page *p
)
{
	if (unlikely(!sess || !p))
		return -EINVAL;

	if (p->page_in)
		put_page(p->page_in);

	put_page(p->page);
	kfree(p);
	atomic_dec(&sess->page_c);

	return 0;
}

static inline int cert_asa_list_empty(
struct cert_asa_session *sess
)
{
	int flag = 0;
/*
	if (down_interruptible(&sess->sem))
		return -EINTR;
*/
	flag = list_empty(&sess->pages);

/*
	up(&sess->sem);
*/
	return flag;
}

static int cert_asa_page_empty
(
	struct cert_asa_session *sess
)
{
	int ret = -1;

	if (unlikely(!sess))
		return -EFAULT;

	if (cert_asa_list_empty(sess)) {
		if (*sess->f_flags & O_NONBLOCK)
			return -EAGAIN;

		while (1) {
			ret = wait_event_interruptible_timeout(
				sess->wq_rd,
				!cert_asa_list_empty(sess),
				CERT_ASA_WAIT_INTERVAL);
			if (likely(ret > 0))
				break;
			else if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static int cert_asa_page_full
(
	struct cert_asa_session *sess
)
{
	int threshhold = CERT_ASA_MAX_PAGES;
	int ret = -1;

	if (unlikely(!sess))
		return -EFAULT;

	if (atomic_read(&sess->page_c) >= threshhold) {
		if (*sess->f_flags & O_NONBLOCK)
			return -EAGAIN;

		while (1) {
			ret = wait_event_interruptible_timeout(
				sess->wq_wr,
				atomic_read(&sess->page_c) < threshhold,
				CERT_ASA_WAIT_INTERVAL);
			if (likely(ret > 0))
				break;
			else if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static int cert_asa_add_page
(
	struct cert_asa_session *sess,
	struct cert_asa_page *p,
	struct list_head *head
)
{
	if (unlikely(!sess || !p))
		return -EFAULT;

	if (down_interruptible(&sess->sem))
		return -EINTR;
	list_add_tail(&p->list, head);
	up(&sess->sem);

	return 0;
}

static void cert_asa_del_page
(
	struct cert_asa_session *sess,
	struct cert_asa_page *p
)
{
	if (unlikely(!sess || !p))
		return;

	down(&sess->sem);
	list_del_init(&p->list);
	up(&sess->sem);
}

static struct cert_asa_page *cert_asa_list_first
(
	struct cert_asa_session *sess,
	struct list_head *head
)
{
	struct cert_asa_page *p;

	if (unlikely(!sess || !head))
		return ERR_PTR(-EFAULT);

	if (down_interruptible(&sess->sem))
		return ERR_PTR(-EINTR);
	p = list_first_entry_or_null(head,
			struct cert_asa_page, list);
	up(&sess->sem);

	return p;
}

static struct cert_asa_page *cert_asa_list_last
(
	struct cert_asa_session *sess,
	struct list_head *head
)
{
	struct cert_asa_page *p = NULL;

	if (unlikely(!sess || !head))
		return ERR_PTR(-EFAULT);

	/*if (down_interruptible(&sess->sem))
		return ERR_PTR(-EINTR);*/

	p = (!list_empty(head) ?
		list_entry(head->prev, struct cert_asa_page, list) :
		NULL);

	/*up(&sess->sem);*/

	return p;
}

static struct cert_asa_page *cert_asa_acquire_first
(
	struct cert_asa_session *sess,
	struct list_head *head
)
{
	struct cert_asa_page *p = NULL;
	int ret = -1;

	if (unlikely(!sess))
		return ERR_PTR(-EFAULT);

	p = cert_asa_list_first(sess, head);
	if (unlikely(IS_ERR_OR_NULL(p)))
		return p;

	if (unlikely(!(p->flag & CERT_ASA_PAGE_ENQUEUED) &&
		(p->len - p->rh - p->rt >= p->packet_size))) {
		while (1) {
			ret = wait_event_interruptible_timeout(
				sess->wq_rd,
				p->flag & CERT_ASA_PAGE_ENQUEUED,
				CERT_ASA_WAIT_INTERVAL);
			if (likely(ret > 0))
				break;
			else if (ret == -ERESTARTSYS)
				return (struct cert_asa_page *)ret;
		}
	}

	ret = cert_asa_sbm_qpage(sess, p);
	if (unlikely(ret != 0)) {
		dev_dbg(sess->drv->dev,
			"query_page() first failed, ret=%d\n", ret);
		return ERR_PTR(ret);
	}

	return p;
}

static struct cert_asa_page *cert_asa_acquire_last
(
	struct cert_asa_session *sess,
	struct list_head *head
)
{
	struct cert_asa_page *p = NULL;
	int ret = -1;

	if (unlikely(!sess))
		return ERR_PTR(-EFAULT);

	p = cert_asa_list_last(sess, head);
	if (unlikely(IS_ERR_OR_NULL(p)))
		return p;

	if (p->len - p->rh - p->rt < p->packet_size)
		return NULL;

	if (unlikely(!(p->flag & CERT_ASA_PAGE_ENQUEUED) &&
		(p->len - p->rh - p->rt >= p->packet_size))) {
		while (1) {
			ret = wait_event_interruptible_timeout(
				sess->wq_rd,
				p->flag & CERT_ASA_PAGE_ENQUEUED,
				CERT_ASA_WAIT_INTERVAL);
			if (likely(ret > 0))
				break;
			else if (ret < 0)
				return ERR_PTR(ret);
		}
	}

	ret = cert_asa_sbm_qpage(sess, p);
	if (unlikely(ret != 0)) {
		dev_dbg(sess->drv->dev, "query_page() last failed\n");
		return ERR_PTR(ret);
	}

	return p;
}

/**
 * Hanlding the residue area in the page,
 * return the number of remain bytes.
*/
static size_t cert_asa_handle_rp
(
	struct cert_asa_session *sess,
	struct cert_asa_page **rp,
	size_t size
)
{
	struct cert_asa_page *tmp = *rp;
	size_t remain = size;

	if (!sess || !size)
		return remain;

	if (!tmp)
		tmp = cert_asa_acquire_first(sess, &sess->rpages);
	if (!IS_ERR_OR_NULL(tmp) &&
		(tmp->flag & CERT_ASA_PAGE_QUERIED)) {
		remain -= min(tmp->len, size);
		*rp = tmp;
	} else {
		*rp = NULL;
	}

	return remain;
}

/**
 * Hanlding the residue area in the page header,
 * for read(), splice_read() and vm_fault() operations.
 * return the number of remain bytes.
*/
static size_t cert_asa_read_rh
(
	struct cert_asa_session *sess,
	struct cert_asa_page *p,
	struct cert_asa_page **rp,
	size_t size
)
{
	size_t remain = size;
	size_t len = 0;

	if (sess->nr_combined > 0) {
		len = min_t(int, sess->nr_combined, remain);
		p->len -= len;
		p->rh -= len;
		sess->nr_combined -= len;
		remain -= len;
		*rp = NULL;
	} else {
		/**
		 * handle the residue in the page header
		 */
		remain = cert_asa_handle_rp(sess, rp, remain);
		p->rh -= size - remain;
		p->len -= size - remain;
	}

	return remain;
}

/**
 * Hanlding the residue area in the page tail,
 * for read(), splice_read() and vm_fault() operations.
 * return the number of remain bytes.
*/
static size_t cert_asa_read_rt
(
	struct cert_asa_session *sess,
	struct cert_asa_page *p,
	struct cert_asa_page **rp,
	size_t size
)
{
	size_t remain = size;
	size_t len = size;
	struct cert_asa_page *tp = NULL;

	if (sess->nr_combined > 0) {
		len = min_t(int, sess->nr_combined, p->rt);
		len = min(len, remain);
		p->len -= len;
		p->rt -= len;
		sess->nr_combined -= len;
		remain -= len;
		*rp = NULL;
	} else if (remain) {
		/**
		 * handle the residue in the page tail
		 */
		if (remain > p->rt) {
			tp = cert_asa_list_first(sess, &p->list);
			len = (!IS_ERR_OR_NULL(tp) &&
				((tp->rh + p->rt) == p->packet_size)) ?
				remain : p->rt;
		}
		remain = cert_asa_handle_rp(sess, rp, len);
		sess->nr_combined = (int)(len - remain - p->rt);
		p->rt = (sess->nr_combined >= 0) ? 0 :
				 (-sess->nr_combined);
		p->len = p->rt;
		remain = size - (len - remain);
	}

	return remain;
}

/* all the page are processed by see ? */
static void cert_asa_page_waitdone
(
	struct cert_asa_session *sess
)
{
	if (cert_asa_list_empty(sess))
		return;

	while (IS_ERR_OR_NULL(cert_asa_acquire_last(sess,
			&sess->pages))) {
		if (!IS_ERR_OR_NULL(cert_asa_acquire_last(sess,
			&sess->rpages)))
			break;
	}
}

void cert_asa_work_pollrd(struct work_struct *work)
{
	struct cert_asa_session *sess = container_of(
		container_of(work, struct delayed_work, work),
		struct cert_asa_session, poll_rd);
	struct cert_asa_page *p;

	p = cert_asa_list_first(sess, &sess->pages);
	if (!IS_ERR_OR_NULL(p)) {
		if (p->flag & CERT_ASA_PAGE_QUERIED)
			goto done;

		if ((p->flag & CERT_ASA_PAGE_ENQUEUED) &&
			!see_query_sbm_entry(&sess->sbm_desc,
			&p->sbm_entry))
			goto done;
	}

	p = cert_asa_list_first(sess, &sess->rpages);
	if (unlikely(!IS_ERR_OR_NULL(p))) {
		if (p->flag & CERT_ASA_PAGE_QUERIED)
			goto done;

		if ((p->flag & CERT_ASA_PAGE_ENQUEUED) &&
			!see_query_sbm_entry(&sess->sbm_desc,
			&p->sbm_entry))
			goto done;
	}

	schedule_delayed_work(&sess->poll_rd, CERT_ASA_POLL_INTERVAL);

	return;

done:
	wake_up_interruptible(&sess->wq_rd);
	return;
}

void cert_asa_work_pollwr(struct work_struct *work)
{
	struct cert_asa_session *sess = container_of(
		container_of(work, struct delayed_work, work),
		struct cert_asa_session, poll_wr);

	if (atomic_read(&sess->page_c) < CERT_ASA_MAX_PAGES)
		wake_up_interruptible(&sess->wq_wr);
	else
		schedule_delayed_work(&sess->poll_wr,
			CERT_ASA_POLL_INTERVAL);
}


/* check the first page for "poll read" is available or not.
0: not available
others: available
*/
static int cert_asa_page_pollrd
(
	struct cert_asa_session *sess
)
{
	struct cert_asa_page *p;

	p = cert_asa_list_first(sess, &sess->pages);
	if (!IS_ERR_OR_NULL(p)) {
		if (p->flag & CERT_ASA_PAGE_QUERIED)
			return 1;
		if ((p->flag & CERT_ASA_PAGE_ENQUEUED) &&
			!see_query_sbm_entry(&sess->sbm_desc,
			&p->sbm_entry))
			return 1;
	}

	p = cert_asa_list_first(sess, &sess->rpages);
	if (unlikely(!IS_ERR_OR_NULL(p))) {
		if (p->flag & CERT_ASA_PAGE_QUERIED)
			return 1;
		if ((p->flag & CERT_ASA_PAGE_ENQUEUED) &&
			!see_query_sbm_entry(&sess->sbm_desc,
			&p->sbm_entry))
			return 1;
	}

	schedule_delayed_work(&sess->poll_rd, CERT_ASA_POLL_INTERVAL);

	return 0;
}

/* check for "poll write" is available or not.
0: not available
others: available
*/
static int cert_asa_page_pollwr
(
	struct cert_asa_session *sess
)
{
	if (atomic_read(&sess->page_c) < CERT_ASA_MAX_PAGES)
		return 1;

	schedule_delayed_work(&sess->poll_wr, CERT_ASA_POLL_INTERVAL);

	return 0;
}

static const struct cert_asa_page_ops cert_asa_ops_page = {
	.alloc = cert_asa_alloc_page,
	.free = cert_asa_free_page,
	.list_empty = cert_asa_page_empty,
	.list_full = cert_asa_page_full,
	.list_add = cert_asa_add_page,
	.list_del = cert_asa_del_page,
	.list_first = cert_asa_list_first,
	.list_last = cert_asa_list_last,
	.acquire_first = cert_asa_acquire_first,
	.acquire_last = cert_asa_acquire_last,
	.wait_done = cert_asa_page_waitdone,
	.poll_rd = cert_asa_page_pollrd,
	.poll_wr = cert_asa_page_pollwr,
	.read_rh = cert_asa_read_rh,
	.read_rt = cert_asa_read_rt
};

void cert_asa_page_register(void *data)
{
	struct cert_asa_drv *drv = (struct cert_asa_drv *)data;

	drv->page_ops = &cert_asa_ops_page;
}
