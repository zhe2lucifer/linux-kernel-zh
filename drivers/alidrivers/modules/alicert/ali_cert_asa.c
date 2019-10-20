/*
 * CERT ASA Driver for Advanced Scramble Algorithm
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
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/atomic.h>
#include <linux/kref.h>
#include <linux/eventfd.h>
#include <linux/list.h>
#include <ali_soc.h>

#include "ali_cert_asa_priv.h"
#include "ali_cert_asa_dbgfs.h"
#include "ali_cert_asa_splice.h"
#include "ali_cert_asa_page.h"
#include "ali_cert_asa_sbm.h"

static inline struct cert_asa_session *file2sess
(
	struct file *file
)
{
	return file ? file->private_data : NULL;
}

static inline struct cert_asa_drv *file2drv
(
	struct file *file
)
{
	return container_of(file->f_dentry->d_inode->i_cdev,
			struct cert_asa_drv, cdev);
}

static inline struct cert_asa_drv *sess2drv
(
	struct cert_asa_session *sess
)
{
	return sess ? sess->drv : NULL;
}

static inline struct cert_asa_key *hdl2key
(
	struct cert_asa_session *sess, int handle
)
{
	struct cert_asa_key *key;

	list_for_each_entry(key, &sess->keys, list) {
		if (key->handle == handle)
			return key;
	}

	return NULL;
}

static int cert_asa_add_pid(struct cert_asa_session *sess,
	struct ca_pid *pid)
{
	int ret = -1;
	struct cert_asa_key *key = NULL;
	struct cert_asa_drv *drv = sess2drv(sess);
	struct pid_node *new_pid_node;

	if (!pid || (pid->key_handle <= 0))
		return -EINVAL;

	key = hdl2key(sess, pid->key_handle);
	if (unlikely(!key || !key->cell)) {
		dev_dbg(drv->dev, "handle not found\n");
		return -ENXIO;
	}

	dev_dbg(drv->dev, "adding pid:0x%04x, ltsid:0x%02x\n",
		pid->pid, pid->ltsid);

	new_pid_node = devm_kzalloc(sess->drv->dev,
		sizeof(struct pid_node), GFP_KERNEL);
	new_pid_node->pid = pid;
	list_add_tail(&new_pid_node->list, &key->pid_list);

	ret = drv->see_ops->addpid(sess->see_sess, key->cell->pos,
			pid->ltsid, pid->pid);

	return ret;
}

static int cert_asa_del_pid(struct cert_asa_session *sess,
	struct ca_pid *pid)
{
	int ret = -1;
	struct cert_asa_key *key = NULL;
	struct cert_asa_drv *drv = sess2drv(sess);

	if (pid->key_handle <= 0)
		return -EINVAL;

	key = hdl2key(sess, pid->key_handle);

	if (unlikely(!key)) {
		dev_dbg(drv->dev, "handle not found\n");
		return -ENXIO;
	}

	dev_dbg(drv->dev, "deleting pid:0x%04x, ltsid:0x%02x\n",
		pid->pid, pid->ltsid);

	ret = drv->see_ops->delpid(sess->see_sess, pid->ltsid, pid->pid);

	return ret;
}

static int cert_asa_create_key(struct cert_asa_session *sess,
	int kl_fd)
{
	struct cert_asa_key *key;
	struct cert_asa_drv *drv = sess2drv(sess);
	int ret = -1;

	/* in case of somebody always allocating the key */
	if (atomic_read(&drv->nr_keys) >= CERT_ASA_MAX_ITEM) {
		dev_dbg(drv->dev, "key handle table is full!\n");
		return -EBUSY;
	}

	key = devm_kzalloc(drv->dev,
			sizeof(struct cert_asa_key),
			GFP_KERNEL);
	if (!key)
		return -ENOMEM;

	ret = fetch_key_cell_by_fd(kl_fd, &key->cell);
	if (unlikely(!key->cell)) {
		dev_dbg(drv->dev, "err fetch_key_cell_by_fd\n");
		devm_kfree(drv->dev, key);
		return ret;
	}
	get_key_cell(key->cell);

	list_add_tail(&key->list, &sess->keys);

	key->keyfd = kl_fd;
	key->handle = ida_simple_get(&sess->key_ida, 1, 0, GFP_KERNEL);

	INIT_LIST_HEAD(&key->pid_list);

	atomic_inc(&drv->nr_keys);

	dev_dbg(drv->dev, "create handle:%d, total hdls:0x%x\n",
		key->handle, drv->nr_keys.counter);

	return key->handle;
}

static int cert_asa_delete_key(struct cert_asa_session *sess,
	int handle)
{
	struct cert_asa_key *key;
	struct cert_asa_drv *drv = sess2drv(sess);
	struct list_head *pos;
	struct pid_node *p;

	key = hdl2key(sess, handle);

	if (!key) {
		dev_dbg(drv->dev, "not a valid handle!\n");
		return -ENXIO;
	}

	list_del_init(&key->list);
	atomic_dec(&drv->nr_keys);

	dev_dbg(drv->dev, "delete handle:%d, cur hdls:0x%x\n",
		key->handle, drv->nr_keys.counter);
	put_key_cell(key->cell);
	ida_simple_remove(&sess->key_ida, key->handle);

	list_for_each(pos, &key->pid_list) {
		p = list_entry(pos, struct pid_node, list);
		cert_asa_del_pid(sess, p->pid);
		devm_kfree(drv->dev, p);
	}
	INIT_LIST_HEAD(&key->pid_list);

	devm_kfree(drv->dev, key);
	return 0;
}

/**
 * Fill the residue page
*/
static void cert_asa_fill_rpage(struct cert_asa_page *p,
	struct cert_asa_page *rp)
{
	int residue_l = 0;
	void *src = NULL;
	void *dest = NULL;
	int packet_size = 0;

	if (!p || !rp)
		return;

	packet_size = rp->packet_size;

	residue_l = rp->len % packet_size;
	if (residue_l || (p->len % packet_size)) {
		dest = kmap(rp->page);
		src = kmap(p->page_in);
	} else {
		return;
	}

	if (p->len >= (packet_size - residue_l)) {
		if (residue_l) {
			p->rh = packet_size - residue_l;
			memcpy(dest + rp->len,
					src + p->offset_in,
					p->rh);
			rp->len += p->rh;
		}
		p->rt = (p->len - p->rh) % packet_size;
		if (p->rt) {
			memcpy(dest + rp->len,
				src + p->offset_in +
				p->len - p->rt,
				p->rt);
			rp->len += p->rt;
		}
	} else {
		p->rt = p->len;
		memcpy(dest + rp->len,
				src + p->offset_in,
				p->rt);
		rp->len += p->rt;
	}

	kunmap(p->page_in);
	kunmap(rp->page);
}

static int cert_asa_write_rp
(
	struct cert_asa_session *sess,
	struct cert_asa_page *p
)
{
	struct cert_asa_drv *drv = sess2drv(sess);
	struct cert_asa_page *rp = NULL;
	struct cert_asa_page *tp = NULL;
	size_t bsize = 0;
	int ret = 0;

	/**
	 * found the existed residue page
	 */
	rp = drv->page_ops->list_last(sess, &sess->rpages);
	if (IS_ERR_OR_NULL(rp) ||
		(rp->flag & CERT_ASA_PAGE_ENQUEUED)) {
		rp = drv->page_ops->alloc(sess);
		if (unlikely(!rp)) {
			ret = -ENOMEM;
			return ret;
		}
		ret = drv->page_ops->list_add(sess, rp, &sess->rpages);
		if (unlikely(ret != 0)) {
			drv->page_ops->free(sess, rp);
			return ret;
		}
	}

	cert_asa_fill_rpage(p, rp);

	if (rp && ((!p && (rp->len >= rp->packet_size)) ||
		(rp->len >= (rp->page_size -
		(2 * rp->packet_size))))) {
		bsize = rp->len % rp->packet_size;
		if (bsize) {
			rp->len -= bsize;
			tp = drv->page_ops->alloc(sess);
			if (unlikely(!tp)) {
				ret = -ENOMEM;
				return ret;
			}
			memcpy(kmap(tp->page), kmap(rp->page) +
				rp->len, bsize);
			kunmap(rp->page);
			kunmap(tp->page);
			tp->len = bsize;
		}
		ret = cert_asa_sbm_epage(sess, rp);
		if (tp) {
			ret = drv->page_ops->list_add(sess, tp, &sess->rpages);
			if (unlikely(ret != 0)) {
				drv->page_ops->free(sess, tp);
				return ret;
			}
		}
	}

	return ret;
}

static int cert_asa_write_diorp
(
	struct cert_asa_dio *dio,
	struct cert_asa_page *p
)
{
	struct cert_asa_session *sess = dio->sess;
	struct cert_asa_page *rp = NULL;
	struct cert_asa_page *tp = NULL;
	size_t bsize = 0;
	int ret = 0;

	list_for_each_entry(tp, &dio->rpages, list) {
		if (!tp->len || !(tp->flag & CERT_ASA_PAGE_ENQUEUED)) {
			rp = tp;
			break;
		}
	}

	if (!p && rp)
		return cert_asa_sbm_epage(sess, rp);

	cert_asa_fill_rpage(p, rp);

	if (rp && (rp->len >= (dio->page_size -
		(2 * dio->packet_size)))) {
		bsize = rp->len % dio->packet_size;
		rp->len -= bsize;
		tp = container_of(rp->list.next,
			struct cert_asa_page, list);
		if (bsize) {
			memcpy(kmap(tp->page), kmap(rp->page) +
				rp->len, bsize);
			kunmap(rp->page);
			kunmap(tp->page);
			tp->len = bsize;
		}
		ret = cert_asa_sbm_epage(sess, rp);
	}

	return ret;
}

/**
 * Read the residue area in the page header,
 * return the remain un-copied size.
*/
static size_t cert_asa_read_diorh
(
	struct cert_asa_dio *dio,
	struct cert_asa_page *p,
	struct cert_asa_page **rp,
	size_t size
)
{
	struct cert_asa_session *sess = dio->sess;
	struct cert_asa_drv *drv = sess2drv(sess);
	struct cert_asa_page *tmp = *rp;
	size_t remain = size;

	if (!size)
		return remain;

	if (!tmp) {
		tmp = list_first_entry_or_null(&dio->rpages,
			struct cert_asa_page, list);
		cert_asa_sbm_qpage(sess, tmp);
	}

	if (tmp && !IS_ERR(tmp) &&
		(tmp->flag & CERT_ASA_PAGE_QUERIED)) {
		memcpy(kmap(p->page) + p->offset,
				kmap(tmp->page) + tmp->offset, remain);
		kunmap(p->page);
		kunmap(tmp->page);
		tmp->len -= remain;
		tmp->offset += remain;
		remain = 0;
		if (!tmp->len) {
			list_del_init(&tmp->list);
			drv->page_ops->free(sess, tmp);
			tmp = NULL;
		}
		*rp = tmp;
	}
	return remain;
}

/**
 * Read the residue area in the page tail,
 * return the remain un-copied size.
*/
static size_t cert_asa_read_diort
(
	struct cert_asa_dio *dio,
	struct cert_asa_page *p,
	struct cert_asa_page **rp,
	size_t size
)
{
	struct cert_asa_session *sess = dio->sess;
	struct cert_asa_drv *drv = sess2drv(sess);
	struct cert_asa_page *tmp = *rp;
	size_t remain = size;
	size_t bsize = 0;

	if (!size)
		return remain;

	if (!tmp) {
		tmp = list_first_entry_or_null(&dio->rpages,
			struct cert_asa_page, list);
		cert_asa_sbm_qpage(sess, tmp);
	}

	if (tmp && !IS_ERR(tmp) &&
		(tmp->flag & CERT_ASA_PAGE_QUERIED)) {
		bsize = min(remain, tmp->len);
		memcpy(kmap(p->page) + p->offset + p->len - p->rt,
				kmap(tmp->page) + tmp->offset, bsize);
		kunmap(p->page);
		kunmap(tmp->page);
		tmp->len -= bsize;
		tmp->offset += bsize;
		remain -= bsize;
		if (!tmp->len) {
			list_del_init(&tmp->list);
			drv->page_ops->free(sess, tmp);
			tmp = NULL;
		}
		*rp = tmp;
	}
	return remain;
}


static void cert_asa_dio_free(struct cert_asa_dio *dio)
{
	struct cert_asa_page *p = NULL;
	struct cert_asa_page *tp = NULL;
	struct cert_asa_drv *drv = NULL;

	if (!dio)
		return;

	drv = sess2drv(dio->sess);

	kfree(dio->in);
	dio->in = NULL;

	kfree(dio->out);
	dio->out = NULL;

	list_for_each_entry_safe(p, tp, &dio->pages, list) {
		list_del_init(&p->list);
		drv->page_ops->free(dio->sess, p);
	}

	list_for_each_entry_safe(p, tp, &dio->rpages, list) {
		list_del_init(&p->list);
		drv->page_ops->free(dio->sess, p);
	}

	down(&dio->sess->sem);
	list_del_init(&dio->list);
	up(&dio->sess->sem);
	kfree(dio);
}

static struct cert_asa_dio *cert_asa_dio_alloc
(
	struct cert_asa_session *sess,
	struct ca_dio_write_read *dio_info
)
{
	struct cert_asa_drv *drv = sess2drv(sess);
	size_t size = dio_info->length;
	int page_off_in = (unsigned long)dio_info->input & (PAGE_SIZE - 1);
	int page_off_out = (unsigned long)dio_info->output & (PAGE_SIZE - 1);
	int nr_pages_in = DIV_ROUND_UP(page_off_in + size, PAGE_SIZE);
	int nr_pages_out = DIV_ROUND_UP(page_off_out + size, PAGE_SIZE);
	struct cert_asa_page *p = NULL;
	struct cert_asa_dio *dio = NULL;
	int ret = -1;
	int i = 0;

	dio = kzalloc(sizeof(struct cert_asa_dio),
			GFP_KERNEL);
	if (!dio)
		return ERR_PTR(-ENOMEM);

	if (down_interruptible(&sess->sem))
		return ERR_PTR(-EINTR);
	list_add_tail(&dio->list, &sess->dios);
	dio->packet_size = sess->packet_size;
	dio->page_size = sess->page_size;
	up(&sess->sem);

	dio->sess = sess;
	dio->unaligned = (page_off_in != page_off_out);
	dio->async = (DIO_FLAG_ASYNC == dio_info->mode);
	dio->user_in = dio_info->input;
	dio->user_out = dio_info->output;
	dio->user_len = dio_info->length;

	INIT_LIST_HEAD(&dio->pages);
	INIT_LIST_HEAD(&dio->rpages);

	/* handle mm_fault for output buffer...
	 * in case of user just allocated this buffer, no page mapped.
	*/
	dio->out = kzalloc(sizeof(struct page *) * nr_pages_out,
				GFP_KERNEL);
	if (!dio->out) {
		ret = -ENOMEM;
		goto err;
	}

	ret = get_user_pages_fast((unsigned long)dio_info->output,
		nr_pages_out, 1, dio->out);
	if (ret <= 0) {
		dev_dbg(drv->dev, "err pages_out\n");
		ret = -EFAULT;
		goto err;
	}

	dio->nr_pages_out = ret;

	if (!dio->unaligned) {
		dio->in = kzalloc(sizeof(struct page *) * nr_pages_in,
				GFP_KERNEL);
		if (!dio->in) {
			ret = -ENOMEM;
			goto err;
		}

		ret = get_user_pages_fast((unsigned long)dio_info->input,
			nr_pages_in, 0, dio->in);
		if (ret <= 0) {
			dev_dbg(drv->dev, "err pages_in\n");
			ret = -EFAULT;
			goto err;
		}
		dio->nr_pages_in = ret;

		for (i = 0; i < dio->nr_pages_in; i++) {
			p =  kzalloc(sizeof(struct cert_asa_page),
						GFP_KERNEL);
			if (unlikely(!p)) {
				ret = -ENOMEM;
				goto err;
			}
			p->packet_size = dio->packet_size;
			p->page_size = dio->page_size;
			p->page_in = dio->in[i];
			p->page = dio->out[i];
			p->offset_in = i ? 0 : page_off_in;
			p->offset = p->offset_in;
			p->len = (((dio->nr_pages_in - 1) == i) ?
					size : (PAGE_SIZE - p->offset_in));
			size -= p->len;
			list_add_tail(&p->list, &dio->pages);
		}
		ret = DIV_ROUND_UP(nr_pages_in * dio->packet_size,
				dio->page_size - (2 * dio->packet_size));
		for (i = 0; i < ret; i++) {
			p = drv->page_ops->alloc(sess);
			if (unlikely(!p)) {
				ret = -ENOMEM;
				goto err;
			}
			list_add_tail(&p->list, &dio->rpages);
		}
	} else {
		ret = DIV_ROUND_UP((PAGE_SIZE - dio->page_size) *
					dio->nr_pages_out, dio->page_size);
		for (i = 0; i < dio->nr_pages_out + ret; i++) {
			p = drv->page_ops->alloc(sess);
			if (unlikely(!p)) {
				ret = -ENOMEM;
				goto err;
			}
			list_add_tail(&p->list, &dio->pages);
		}
	}

	if (dio->async)
		dio->eventfd = eventfd_ctx_fdget(dio_info->eventfd);
	init_waitqueue_head(&dio->wq);

	return dio;
err:
	cert_asa_dio_free(dio);
	return ERR_PTR(ret);
}

static void cert_asa_dio_work(struct work_struct *work)
{
	struct cert_asa_session *sess = NULL;
	struct cert_asa_dio *dio = NULL;
	struct cert_asa_page *p = NULL;
	struct cert_asa_page *rp = NULL;
	int ret = -1;
	int i = 0;
	int page_off_out = 0;
	int dest_off = 0;
	int dest_len = 0;
	int remain_len = 0;

	int bsize = 0;

	dio = container_of(work, struct cert_asa_dio, work);

	sess = dio->sess;

	if (!dio->unaligned) {
		list_for_each_entry(p, &dio->pages, list) {
			ret = cert_asa_sbm_qpage(sess, p);
			if (unlikely(ret != 0)) {
				dev_dbg(sess->drv->dev,
					"query_page() failed, ret:%d\n", ret);
				goto out;
			}

			while (p->rh)
				p->rh = cert_asa_read_diorh(dio, p, &rp, p->rh);
			while (p->rt)
				p->rt = cert_asa_read_diort(dio, p, &rp, p->rt);
		}
	} else {
		page_off_out = (unsigned long)dio->user_out & (PAGE_SIZE - 1);
		remain_len = dio->user_len;
		for (i = 0; i < dio->nr_pages_out; i++) {
			dest_off = i ? 0 : page_off_out;
			dest_len = (((dio->nr_pages_out - 1) == i) ?
					remain_len : (PAGE_SIZE - dest_off));

			remain_len -= dest_len;

			list_for_each_entry_safe(p, rp, &dio->pages, list) {
				ret = cert_asa_sbm_qpage(sess, p);
				if (unlikely(ret != 0))
					goto out;
				bsize = min_t(int, p->len, dest_len);
				memcpy(kmap(dio->out[i]) + dest_off,
					kmap(p->page) + p->offset, bsize);
				kunmap(p->page);
				kunmap(dio->out[i]);
				p->len -= bsize;
				p->offset += bsize;
				dest_len -= bsize;
				dest_off += bsize;
				if (!p->len) {
					list_del_init(&p->list);
					sess->drv->page_ops->free(dio->sess, p);
				}
				if (!dest_len)
					break;
			}
		}
	}

out:
	if (dio->async && !IS_ERR_OR_NULL(dio->eventfd)) {
		eventfd_signal(dio->eventfd, 1);
		eventfd_ctx_put(dio->eventfd);
	}

	dio->done = 1;
	dio->status = ret;
	if (dio->async)
		cert_asa_dio_free(dio);
}

static int cert_asa_dio_rw(struct cert_asa_session *sess,
	struct ca_dio_write_read *dio_info)
{
	int ret = -1;
	struct cert_asa_drv *drv = sess2drv(sess);
	struct cert_asa_page *p = NULL;
	int wr_bytes = 0;
	int bsize = 0;

	struct cert_asa_dio *dio = NULL;

	if (dio_info->length % sess->packet_size) {
		dev_dbg(drv->dev, "dio size is not TS packets aligned\n");
		return -EINVAL;
	}

	dio = cert_asa_dio_alloc(sess, dio_info);
	if (IS_ERR_OR_NULL(dio))
		return PTR_ERR(dio);

	list_for_each_entry(p, &dio->pages, list) {
		if (!dio->unaligned) {
			ret = cert_asa_write_diorp(dio, p);
		} else {
			bsize = (dio->user_len - wr_bytes) > dio->page_size ?
				dio->page_size : (dio->user_len - wr_bytes);
			ret = copy_from_user(kmap(p->page),
				dio->user_in + wr_bytes, bsize);
			kunmap(p->page);
			p->len = bsize;
			wr_bytes += bsize;
		}

		ret |= cert_asa_sbm_epage(sess, p);
		if (unlikely(ret != 0)) {
			dev_dbg(drv->dev,
				"failed, ret:%d\n", ret);
			break;
		}
	}

	ret |= cert_asa_write_diorp(dio, NULL);
	if (ret) {
		cert_asa_dio_free(dio);
		goto out;
	}

	INIT_WORK(&dio->work, cert_asa_dio_work);
	schedule_work(&dio->work);

	while (!dio->async) {
		ret = wait_event_interruptible_timeout(dio->wq,
				dio->done,
				CERT_ASA_WAIT_INTERVAL);
		if (likely(ret > 0)) {
			cert_asa_dio_free(dio);
			break;
		} else if (unlikely(ret < 0)) {
			return ret;
		}
	}

	ret = dio->status ? -EIO : 0;

out:
	return ret;
}

static long cert_asa_ioctl
(
	struct file *file,
	unsigned int cmd,
	unsigned long  arg
)
{
	int ret = -1;
	int ts_fmt = -1;
	struct cert_asa_session *sess = file2sess(file);
	struct cert_asa_drv *drv = file2drv(file);
	struct ca_pid pid;
	struct ca_create_kl_key akl_key;
	struct ca_dio_write_read dio_rw;
	int lock = 1;

	if (unlikely(!drv || !sess)) {
		dev_dbg(drv->dev, "invalid session : 0x%x\n",
			sess ? sess->see_sess : 0);
		return -EBADF;
	}

	if (!arg)	{
		ret = -EINVAL;
		dev_dbg(drv->dev, "Invalid Param\n");
		goto out;
	}

	switch (CERT_IO_CMD(cmd)) {
	case CERT_IO_CMD(CA_DIO_WRITE_READ):
	case CERT_IO_CMD(CA_SET_FORMAT):
		lock = 0;
		break;
	default:
		break;
	}

	if (lock && down_interruptible(&sess->sem))
		return -EINTR;

	switch (CERT_IO_CMD(cmd)) {
	case CERT_IO_CMD(CA_SET_FORMAT):
	{
		ret = copy_from_user((void *)&ts_fmt,
			(void __user *)arg, sizeof(ts_fmt));
		if (unlikely(0 != ret)) {
			dev_dbg(drv->dev, "copy_from_user, ret:%d\n", ret);
			goto out;
		}

		if ((sess->ts_fmt_configured) &&
			(sess->ts_fmt != ts_fmt)) {
			ret = -EEXIST;
			goto out;
		}

		if ((ts_fmt != CA_FORMAT_TS188) &&
			(ts_fmt != CA_FORMAT_TS200) &&
			(ts_fmt != CA_FORMAT_TS188_LTSID)) {
			ret = -EOPNOTSUPP;
			goto out;
		}

		down(&sess->wr_sem);
		drv->page_ops->wait_done(sess);

		ret = drv->see_ops->setfmt(sess->see_sess, ts_fmt);
		if (0 != ret) {
			dev_dbg(drv->dev, "see_asa_set_format\n");
			up(&sess->wr_sem);
			goto out;
		}

		sess->ts_fmt = ts_fmt;
		sess->ts_fmt_configured = 1;
		if (CA_FORMAT_TS200 == ts_fmt)
			sess->packet_size = CERT_ASA_TS200_SIZE;
		else
			sess->packet_size = CERT_ASA_TS188_SIZE;
		sess->page_size = rounddown(PAGE_SIZE, sess->packet_size);
		up(&sess->wr_sem);
		break;
	}

	case CERT_IO_CMD(CA_CREATE_KL_KEY):
	{
		if (!sess->ts_fmt_configured) {
			dev_dbg(drv->dev, "CA_SET_FORMAT pre-needed\n");
			ret = -EPERM;
			goto out;
		}
		ret = copy_from_user((void *)&akl_key,
				(void __user *)arg, sizeof(akl_key));
		if (unlikely(0 != ret)) {
			dev_dbg(drv->dev, "copy_from_user, ret:%d\n", ret);
			goto out;
		}

		if ((akl_key.algo != CA_ALGO_ASA) ||
			(akl_key.kl_fd < 0)) {
			ret = -EINVAL;
			goto out;
		}
		if (akl_key.valid_mask) {
			ret = -ENOTSUPP;
			goto out;
		}

		ret = cert_asa_create_key(sess, akl_key.kl_fd);
		break;
	}

	case CERT_IO_CMD(CA_ADD_PID):
	case CERT_IO_CMD(CA_DEL_PID):
	{
		ret = copy_from_user((void *)&pid,
				(void __user *)arg, sizeof(pid));
		if (0 != ret) {
			dev_dbg(drv->dev, "copy_from_user, ret:%d\n", ret);
			goto out;
		}

		down(&sess->wr_sem);
		drv->page_ops->wait_done(sess);

		if (CERT_IO_CMD(CA_ADD_PID) == CERT_IO_CMD(cmd))
			ret = cert_asa_add_pid(sess, &pid);
		else
			ret = cert_asa_del_pid(sess, &pid);

		if (0 != ret)
			dev_dbg(drv->dev, "dealing with pid, ret:%d\n", ret);

		up(&sess->wr_sem);

		break;
	}

	case CERT_IO_CMD(CA_DELETE_KEY):
	{
		down(&sess->wr_sem);
		drv->page_ops->wait_done(sess);

		ret = cert_asa_delete_key(sess, arg);

		up(&sess->wr_sem);
		break;
	}

	case CERT_IO_CMD(CA_DIO_WRITE_READ):
	{
		ret = copy_from_user((void *)&dio_rw,
				(void __user *)arg, sizeof(dio_rw));
		if (unlikely(0 != ret)) {
			dev_dbg(drv->dev, "copy_from_user, ret:%d\n", ret);
			goto out;
		}

		ret = cert_asa_dio_rw(sess, &dio_rw);

		break;
	}

	default:
		dev_dbg(drv->dev, "unsupport cmd=0x%x\n", cmd);
		ret = -ENOIOCTLCMD;
		break;
	}

out:
	if (lock)
		up(&sess->sem);
	return ret;
}

static int cert_asa_open
(
	struct inode *inode,
	struct file  *file
)
{
	struct cert_asa_drv *drv = file2drv(file);
	struct cert_asa_session *sess = NULL;
	int ret = -1;

	if (unlikely(!drv))
		return -EBADF;

	sess = devm_kzalloc(drv->dev,
			sizeof(struct cert_asa_session),
			GFP_KERNEL);
	if (NULL == sess)
		return -ENOMEM;

	sema_init(&sess->sem, 1);
	sema_init(&sess->rd_sem, 1);
	sema_init(&sess->wr_sem, 1);
	init_waitqueue_head(&sess->wq_rd);
	init_waitqueue_head(&sess->wq_wr);
	init_waitqueue_head(&sess->wq_sbm);
	INIT_LIST_HEAD(&sess->keys);
	INIT_LIST_HEAD(&sess->pages);
	INIT_LIST_HEAD(&sess->rpages);
	INIT_LIST_HEAD(&sess->dios);
	ida_init(&sess->key_ida);
	sess->ts_fmt = CA_FORMAT_TS188;
	sess->packet_size = CERT_ASA_TS188_SIZE;
	sess->page_size = rounddown(PAGE_SIZE, sess->packet_size);
	sess->drv = drv;
	sess->f_flags = &file->f_flags;
	file->private_data = (void *)sess;

	INIT_DELAYED_WORK(&sess->poll_rd, cert_asa_work_pollrd);
	INIT_DELAYED_WORK(&sess->poll_wr, cert_asa_work_pollwr);

	ret = cert_asa_sbm_add(sess);
	if (ret < 0)
		goto out;

	ret = drv->see_ops->open(sess->sbm_desc.id);
	sess->see_sess = ret;
	dev_dbg(drv->dev, "got see_sess: 0x%x\n",
					sess->see_sess);
	if (ret < 0) {
		dev_dbg(drv->dev, "err see_sess: 0x%x\n",
					sess->see_sess);
		goto out;
	}

	cert_asa_dbgfs_add(sess);

	ret = 0;

out:
	if (ret) {
		cert_asa_sbm_del(sess);
		devm_kfree(drv->dev, sess);
	}

	return ret;
}

static int cert_asa_close
(
	struct inode *inode,
	struct file  *file
)
{
	struct cert_asa_drv *drv = file2drv(file);
	struct cert_asa_session *sess = file2sess(file);
	struct cert_asa_key *key = NULL;
	struct cert_asa_page *p = NULL;
	struct cert_asa_page *tp = NULL;

	if (unlikely(!drv || !sess))
		return -EBADF;
/*
	struct cert_asa_dio *dio = NULL;
	list_for_each_entry(dio, &sess->dios, list)
		cancel_work_sync(&dio->work);
*/
	while (!list_empty(&sess->dios))
		msleep_interruptible(1);

	down(&sess->rd_sem);/*not mandatory*/
	down(&sess->wr_sem);/*not mandatory*/
	down(&sess->sem);

	/*cancel the delayed work*/
	cancel_delayed_work_sync(&sess->poll_rd);
	cancel_delayed_work_sync(&sess->poll_wr);

	cert_asa_dbgfs_del(sess);

	while (!list_empty(&sess->keys)) {
		list_for_each_entry(key, &sess->keys, list) {
			cert_asa_delete_key(sess, key->handle);
			break;
		}
	}

	if (sess->see_sess >= 0) {
		dev_dbg(drv->dev, "free see_sess 0x%x\n",
			sess->see_sess);
		drv->see_ops->close(sess->see_sess);
		sess->see_sess = -1;
	}
	cert_asa_sbm_del(sess);

	ida_destroy(&sess->key_ida);

	list_for_each_entry_safe(p, tp, &sess->pages, list)
		drv->page_ops->free(sess, p);

	list_for_each_entry_safe(p, tp, &sess->rpages, list)
		drv->page_ops->free(sess, p);

	list_del_init(&sess->keys);
	list_del_init(&sess->pages);
	list_del_init(&sess->rpages);
	list_del_init(&sess->dios);
	sess->drv = NULL;
	file->private_data = NULL;
	up(&sess->sem);
	up(&sess->wr_sem);
	up(&sess->rd_sem);

	devm_kfree(drv->dev, sess);

	return 0;
}

static void cert_asa_munmap(struct vm_area_struct *vma)
{
	struct cert_asa_session *sess = vma->vm_private_data;

	dev_dbg(sess->drv->dev, "munmap vm_start:%lx, end:%lx\n",
		vma->vm_start, vma->vm_end);
}

static int cert_asa_fault(struct vm_area_struct *vma,
	struct vm_fault *vmf)
{
	struct cert_asa_session *sess = vma->vm_private_data;
	struct cert_asa_drv *drv = sess2drv(sess);
	struct cert_asa_page *p = NULL; /* page from page list  */
	struct cert_asa_page *rp = NULL; /* residue page */
	struct cert_asa_page *op = NULL; /*output page */
	int ret = 0;
	int erase_l = 0;
	int output_l = 0;
	size_t remain = 0;

	unsigned char *kmap_addr = NULL;

	if (unlikely(!sess || !drv))
		return VM_FAULT_NOPAGE;

	down(&sess->rd_sem);

	while (!output_l) {
		p = drv->page_ops->acquire_first(sess, &sess->pages);
		if (IS_ERR_OR_NULL(p)) {
			ret = VM_FAULT_SIGBUS;
			goto out;
		}

		if (p->rh) {
			output_l = p->rh;
			remain = drv->page_ops->read_rh(sess, p, &rp, p->rh);
			output_l -= rp ? remain : output_l;
		} else if (p->len - p->rt) {
			op = p;
			output_l = p->len - p->rt;
			p->len -= output_l;
		} else if (p->rt) {
			output_l = p->packet_size;
			remain = drv->page_ops->read_rt(sess, p, &rp,
					p->packet_size);
			output_l -= rp ? remain : output_l;
		}

		if (rp && output_l) {
			op = drv->page_ops->alloc(sess);
			if (unlikely(!op)) {
				ret = VM_FAULT_OOM;
				goto out;
			}
			kmap_addr = kmap(rp->page);
			memcpy(kmap(op->page),
					kmap_addr + rp->offset,
					output_l);
			kunmap(rp->page);
			kunmap(op->page);
			rp->len -= output_l;
			rp->offset += output_l;
			if (!rp->len) {
				drv->page_ops->list_del(sess, rp);
				drv->page_ops->free(sess, rp);
			}
		}

		if (op) {
			/**
			 * remain the cur pipe page
			 */
			get_page(op->page);
			vmf->page = op->page;

			/**
			 * Special information for user...
			 */
			erase_l = (int)PAGE_SIZE - output_l - sizeof(long);
			if (erase_l >= 0) {
				kmap_addr = kmap(op->page);
				memset(kmap_addr + output_l, 0, erase_l);
				*(unsigned long *)(kmap_addr + PAGE_SIZE -
					 sizeof(unsigned long)) = output_l;
				kunmap(op->page);
			} else {
				dev_dbg(drv->dev, "bad page for vmsplice??\n");
			}
		}

		/**
		 * del from the page list
		 */
		if (!p->len) {
			drv->page_ops->list_del(sess, p);
			drv->page_ops->free(sess, p);
		}

		if (op && (op != p))
			drv->page_ops->free(sess, op);
	}

out:
	if (output_l) {
		wake_up_interruptible(&sess->wq_wr);
		atomic_sub(output_l, &sess->nr_bytes);
		sess->mmap_bytes += output_l;
	}
	up(&sess->rd_sem);
	return ret;
}

static const struct vm_operations_struct cert_asa_vm_ops = {
	/* callback - when page fault */
	.fault = cert_asa_fault,
	/* callback - when the vm-area is released */
	.close = cert_asa_munmap,
};

static int cert_asa_mmap(struct file *file,
	struct vm_area_struct *vma)
{
	struct cert_asa_session *sess = file2sess(file);

	if (unlikely(!sess))
		return -EBADF;

	dev_dbg(sess->drv->dev, "vm_start:%lx, end:%lx\n",
		vma->vm_start, vma->vm_end);

	vma->vm_flags |= VM_DONTCOPY | VM_DONTEXPAND | VM_NONLINEAR;
	vma->vm_private_data = sess;
	vma->vm_ops = &cert_asa_vm_ops;

	return 0;
}

static ssize_t cert_asa_read(struct file *file,
	char __user *buf, size_t size, loff_t *f_pos)
{
	struct cert_asa_session *sess = file2sess(file);
	struct cert_asa_drv *drv = file2drv(file);
	struct cert_asa_page *p = NULL; /* page from page list  */
	struct cert_asa_page *rp = NULL; /* residue page */
	struct cert_asa_page *op = NULL; /*output page */
	int ret = 0;
	size_t rd_bytes = 0;
	size_t bsize = 0;

	if (!size)
		return size;

	if (unlikely(!sess || !drv))
		return -EBADF;

	ret = drv->page_ops->list_empty(sess);
	if (ret)
		return ret;

	if (down_interruptible(&sess->rd_sem))
		return -EINTR;

	dev_dbg(drv->dev,
		"read: uaddr[%p], size[%d]\n",
		buf, size);

	while (size > rd_bytes) {
		p = drv->page_ops->acquire_first(sess, &sess->pages);
		if (IS_ERR_OR_NULL(p)) {
			ret = PTR_ERR(p);
			goto out;
		}

		if (p->rh) {
			bsize = min(p->rh, size - rd_bytes);
			bsize -= drv->page_ops->read_rh(sess, p, &rp, bsize);
			op = rp;
		} else if (p->len - p->rt) {
			bsize = min(p->len - p->rt, size - rd_bytes);
			op = p;
		} else if (p->rt) {
			bsize = min(p->rt, size - rd_bytes);
			bsize -= drv->page_ops->read_rt(sess, p, &rp, bsize);
			op = rp;
		}

		dev_dbg(drv->dev, "\tout:page[%p], size[%d], plen[%d], rh[%d], rt[%d]\n",
			p->page, bsize, p->len, p->rh, p->rt);

		if (op && bsize) {
			ret = copy_to_user(buf + rd_bytes,
				kmap(op->page) + op->offset, bsize);
			kunmap(op->page);
			if (ret != 0) {
				dev_dbg(drv->dev,
					"copy_to_user() failed, ret:%d\n", ret);
				goto out;
			}

			rd_bytes += bsize;
			op->offset += bsize;
			op->len -= bsize;
		}

		if (!p->len) {
			drv->page_ops->list_del(sess, p);
			drv->page_ops->free(sess, p);
		}
		if (rp && !rp->len) {
			drv->page_ops->list_del(sess, rp);
			drv->page_ops->free(sess, rp);
			rp = NULL;
		}
		if (!op && !bsize)
			goto out;

		op = NULL;
	}

out:
	if (rd_bytes) {
		wake_up_interruptible(&sess->wq_wr);
		atomic_sub(rd_bytes, &sess->nr_bytes);

		sess->rd_bytes += rd_bytes;
	}
	up(&sess->rd_sem);
	return rd_bytes ? rd_bytes : ret;
}

static ssize_t cert_asa_write(struct file *file,
	const char __user *buf, size_t size, loff_t *offset)
{
	int ret = 0;
	size_t wr_bytes = 0;
	size_t bsize = 0;
	struct cert_asa_session *sess = file2sess(file);
	struct cert_asa_drv *drv = file2drv(file);
	struct cert_asa_page *p = NULL;
	struct cert_asa_page *rp = NULL;
	int residue_l = 0;

	if (!size)
		return size;

	if (unlikely(!sess || !drv))
		return -EBADF;

	ret = drv->page_ops->list_full(sess);
	if (ret)
		return ret;

	if (down_interruptible(&sess->wr_sem))
		return -EINTR;

	dev_dbg(drv->dev,
		"write: uaddr[%p], size[%d]\n",
		buf, size);

	while (wr_bytes < size) {
		p = drv->page_ops->alloc(sess);
		if (!p) {
			ret = -ENOMEM;
			goto out;
		}

		bsize = (p->page_size >= (size - wr_bytes)) ?
				(size - wr_bytes) : p->page_size;

		rp = drv->page_ops->list_last(sess, &sess->rpages);
		if (!IS_ERR_OR_NULL(rp) &&
			(rp->len % rp->packet_size)) {
			bsize = min(bsize,
				rp->packet_size - (rp->len % rp->packet_size));
		}

		ret = copy_from_user(kmap(p->page), buf + wr_bytes, bsize);
		kunmap(p->page);
		if (unlikely(ret != 0)) {
			dev_dbg(drv->dev, "from_user() failed, ret:%d\n", ret);
			goto out;
		}

		p->len = bsize;
		if (p->len < p->packet_size) {
			p->page_in = p->page;
			ret = cert_asa_write_rp(sess, p);
			if (unlikely(ret != 0)) {
				dev_dbg(drv->dev, "write_rp failed\n");
				goto out;
			}
			residue_l = 1;
			p->page_in = NULL;
		}

		dev_dbg(drv->dev, "\tin:page[%p], size[%d], plen[%d], rh[%d], rt[%d]\n",
			p->page, bsize, p->len, p->rh, p->rt);

		ret = cert_asa_sbm_epage(sess, p);
		if (ret != 0) {
			dev_dbg(drv->dev,
				"enqueue_page() failed, ret:%d\n", ret);
			goto out;
		}

		ret = drv->page_ops->list_add(sess, p, &sess->pages);
		if (ret != 0)
			goto out;

		wr_bytes += bsize;
	}

	if (residue_l)
		cert_asa_write_rp(sess, NULL);

	/* query the last page */
	if (file->f_flags & O_SYNC)
		cert_asa_sbm_qpage(sess, p);

out:
	if (unlikely(ret))
		drv->page_ops->free(sess, p);

	if (wr_bytes) {
		wake_up_interruptible(&sess->wq_rd);
		atomic_add(wr_bytes, &sess->nr_bytes);

		sess->wr_bytes += wr_bytes;
	}
	up(&sess->wr_sem);
	return wr_bytes ? wr_bytes : ret;
}

static unsigned int cert_asa_poll(struct file *file,
	struct poll_table_struct *wait)
{
	struct cert_asa_session *sess = file2sess(file);
	unsigned int flag = 0;

	if (unlikely(!sess))
		return -EBADF;

	poll_wait(file, &sess->wq_wr, wait);
	poll_wait(file, &sess->wq_rd, wait);

	if (sess->drv->page_ops->poll_wr(sess))
		flag |= POLLOUT | POLLWRNORM;

	if (sess->drv->page_ops->poll_rd(sess))
		flag |= POLLIN | POLLRDNORM;

	return flag;
}

static ssize_t cert_asa_splice_read(struct file *file, loff_t *ppos,
				  struct pipe_inode_info *pipe, size_t size,
				  unsigned int flags)
{
	struct cert_asa_session *sess = file2sess(file);
	struct cert_asa_drv *drv = file2drv(file);
	struct cert_asa_page *p = NULL; /* page from page list  */
	struct cert_asa_page *rp = NULL; /* residue page */
	struct cert_asa_page *op = NULL; /*output page to pipe */
	struct page *pipe_pages[PIPE_DEF_BUFFERS];
	struct partial_page partial[PIPE_DEF_BUFFERS];
	struct splice_pipe_desc spd = {
		.pages = pipe_pages,
		.partial = partial,
		.nr_pages = 0,
		.nr_pages_max = PIPE_DEF_BUFFERS,
		.flags = flags,
		.ops = NULL,
		.spd_release = NULL,
	};
	size_t nr_pages = DIV_ROUND_UP(size, PAGE_SIZE);
	size_t rd_bytes = 0;
	int ret = 0;
	int len = 0;

	if (unlikely(!sess || !drv || !pipe))
		return -EBADF;

	ret = drv->page_ops->list_empty(sess);
	if (ret)
		return ret;

	if (down_interruptible(&sess->rd_sem))
		return -EINTR;

	dev_dbg(drv->dev, "splice read:%p, rdsize:0x%x, destbufs:%d\n",
			pipe, size, pipe->buffers - pipe->nrbufs);

	if (drv->splice_ops.grow_spd(pipe, &spd)) {
		ret = -ENOMEM;
		goto out;
	}

	spd.ops = drv->splice_ops.buf_ops;
	spd.spd_release = drv->splice_ops.spd_release;
	nr_pages = min(nr_pages, spd.nr_pages_max);
	nr_pages = min(nr_pages, pipe->buffers - pipe->nrbufs);
	while ((rd_bytes < size) && (spd.nr_pages < nr_pages)) {
		p = drv->page_ops->acquire_first(sess, &sess->pages);
		if (IS_ERR_OR_NULL(p))
			break;

		if (p->rh) {
			/**
			 * handle the residue in the page header
			 */
			len = min(p->rh, size - rd_bytes);
			len -= drv->page_ops->read_rh(sess, p, &rp, len);
			spd.partial[spd.nr_pages].len = len;
			op = rp;
		} else if (p->len - p->rt) {
			/**
			 * handle the packet aligned area
			 */
			spd.partial[spd.nr_pages].len = min(size - rd_bytes,
					p->len - p->rt);
			op = p;
		} else if (p->rt) {
			/**
			 * handle the residue in the page tail
			 */
			len = min(p->packet_size, size - rd_bytes);
			len -= drv->page_ops->read_rt(sess, p, &rp, len);
			spd.partial[spd.nr_pages].len = len;
			op = rp;
		}

		if (op && spd.partial[spd.nr_pages].len) {
			rd_bytes += spd.partial[spd.nr_pages].len;
			spd.pages[spd.nr_pages] = op->page;
			spd.partial[spd.nr_pages].offset = op->offset;
			get_page(op->page);
			op->offset += spd.partial[spd.nr_pages].len;
			op->len -= spd.partial[spd.nr_pages].len;
			spd.nr_pages++;
		}

		if (!p->len) {
			drv->page_ops->list_del(sess, p);
			drv->page_ops->free(sess, p);
		}

		if (rp && !rp->len) {
			drv->page_ops->list_del(sess, rp);
			drv->page_ops->free(sess, rp);
			rp = NULL;
		}
		if (!op && !spd.partial[spd.nr_pages].len)
			break;
	}

	dev_dbg(drv->dev,
		"splice read nr_pages:%d, rd_bytes:0x%x\n",
		spd.nr_pages, rd_bytes);

	ret = rd_bytes ? drv->splice_ops.to_pipe(pipe, &spd) : 0;

out:
	if (ret > 0) {
		wake_up_interruptible(&sess->wq_wr);
		atomic_sub(ret, &sess->nr_bytes);
	}
	drv->splice_ops.shrink_spd(&spd);
	up(&sess->rd_sem);
	return ret;
}

static int __cert_asa_from_pipe(struct pipe_inode_info *pipe,
		struct pipe_buffer *buf, struct splice_desc *sd)
{
	int ret = -1;
	struct cert_asa_session *sess = NULL;
	struct cert_asa_drv *drv = NULL;
	struct cert_asa_page *p = NULL;

	if (unlikely(!pipe || !buf || !sd))
		return -EINVAL;

	if (!sd->len)
		return 0;

	sess = file2sess(sd->u.file);
	drv = file2drv(sd->u.file);
	if (unlikely(!sess || !drv))
		return -EINVAL;

	p = drv->page_ops->alloc(sess);
	if (unlikely(!p)) {
		ret = -ENOMEM;
		goto out;
	}

	p->offset_fixed = 1;
	p->page_in = buf->page;
	p->offset_in = buf->offset;
	p->len = sd->len;

	ret = cert_asa_write_rp(sess, p);
	ret |= cert_asa_sbm_epage(sess, p);
	if (unlikely(ret < 0)) {
		drv->page_ops->free(sess, p);
		goto out;
	}

	ret = drv->page_ops->list_add(sess, p, &sess->pages);
	if (unlikely(ret != 0)) {
		drv->page_ops->free(sess, p);
		goto out;
	}

	buf->ops->get(pipe, buf);
	ret = p->len;

out:
	return ret;
}

static ssize_t cert_asa_splice_write(struct pipe_inode_info *pipe,
				   struct file *file, loff_t *ppos,
				   size_t size, unsigned int flags)
{
	struct cert_asa_session *sess = file2sess(file);
	struct cert_asa_drv *drv = file2drv(file);
	struct cert_asa_page *p = NULL;
	int ret = -1;

	if (unlikely(!sess || !drv))
		return -EBADF;

	ret = drv->page_ops->list_full(sess);
	if (ret)
		return ret;

	if (down_interruptible(&sess->wr_sem))
		return -EINTR;

	dev_dbg(drv->dev,
		"splice write:%p, wrsize:0x%x, pipebufs:%d\n",
		pipe, size, pipe->nrbufs);

	ret = drv->splice_ops.from_pipe(pipe, file, ppos, size,
				flags, __cert_asa_from_pipe);
	if (unlikely(ret < 0))
		goto out;

	/**
	 * needs to enqueue the residue pages
	 */
	if (cert_asa_write_rp(sess, NULL) < 0) {
		ret = -EBUSY;
		goto out;
	}

	/**
	 * query the last page
	 */
	if (file->f_flags & O_SYNC) {
		p = drv->page_ops->list_last(sess, &sess->pages);
		if (!IS_ERR_OR_NULL(p))
			cert_asa_sbm_qpage(sess, p);
	}

out:
	if (ret > 0) {
		wake_up_interruptible(&sess->wq_rd);
		atomic_add(ret, &sess->nr_bytes);
	}
	up(&sess->wr_sem);
	return ret;
}

static const struct file_operations g_cert_asa_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = cert_asa_ioctl,
	.open = cert_asa_open,
	.release = cert_asa_close,
	.read = cert_asa_read,
	.write = cert_asa_write,
	.poll = cert_asa_poll,
	.mmap = cert_asa_mmap,
	.splice_read = cert_asa_splice_read,
	.splice_write = cert_asa_splice_write,
};

int cert_asa_probe(struct cert_driver *parent)
{
	int ret = -1;
	struct cert_asa_drv *drv = NULL;
	struct device_node *child;

	drv = kzalloc(sizeof(struct cert_asa_drv),
			GFP_KERNEL);
	if (!drv)
		return -ENOMEM;

	sema_init(&drv->sem, 1);

	child = of_get_child_by_name(parent->clnt->dev.of_node, CERT_ASA_DEV);
	if (!child) {
		pr_err("Don't find child <functions> of DTS node<%s>!\n",CERT_ASA_DEV);
		ret = alloc_chrdev_region(&drv->devt, 0, 1, CERT_ASA_DEV);
		if (ret < 0)
		{
			drv->devt =0;
			return -1;
		}
	}
	else{
		ret = of_get_major_minor(child,&drv->devt, 0, 1, CERT_ASA_DEV);
		if (ret  < 0) {
			pr_err("unable to get major and minor for char devive\n");
			return ret;
		}
	}

	cdev_init(&drv->cdev, &g_cert_asa_fops);

	drv->cdev.owner = THIS_MODULE;

	ret = cdev_add(&drv->cdev, drv->devt, 1);
	if (ret < 0)
		goto out;

	drv->dev_class = class_create(THIS_MODULE, CERT_ASA_DEV);
	if (IS_ERR_OR_NULL(drv->dev_class)) {
		ret = PTR_ERR(drv->dev_class);
		goto out;
	}

	drv->dev = device_create(drv->dev_class, NULL, drv->devt, drv,
						CERT_ASA_DEV);
	if (IS_ERR_OR_NULL(drv->dev)) {
		ret = PTR_ERR(drv->dev);
		goto out;
	}

	parent->asa_drv = drv;
	drv->see_clnt = parent->clnt;
	if (0 != see_cert_asa_register(drv)) {
		ret = -ENXIO;
		dev_dbg(&parent->clnt->dev, "see_cert_asa_register failed\n");
		goto out;
	}

	cert_asa_dbgfs_create(drv);
	cert_asa_splice_register(drv);
	cert_asa_page_register(drv);
	dev_dbg(&parent->clnt->dev, "CERT-ASA driver probed\n");

	ret = 0;

out:
	if (unlikely(ret)) {
		if (drv->dev_class) {
			device_destroy(drv->dev_class, drv->devt);
			cdev_del(&drv->cdev);
			class_destroy(drv->dev_class);
			unregister_chrdev_region(drv->devt, 1);
		}
		kfree(drv);
	}
	return ret;
}

int cert_asa_remove(struct cert_driver *parent)
{
	struct cert_asa_drv *drv = parent->asa_drv;

	down(&drv->sem);
	cert_asa_dbgfs_remove(drv);
	see_cert_asa_unregister(drv);

	/*up(&drv->sem);*/

	unregister_chrdev_region(drv->devt, 1);
	device_destroy(drv->dev_class, drv->devt);
	cdev_del(&drv->cdev);
	class_destroy(drv->dev_class);
	kfree(drv);
	dev_dbg(&parent->clnt->dev, "CERT-ASA driver removed\n");
	return 0;
}

