/*
 * Scramber Core driver
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


#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/idr.h>
#include <linux/dma-mapping.h>
#include <linux/highmem.h>
#include <linux/splice.h>
#include <linux/poll.h>
#include <linux/of.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/pagemap.h>
#include <linux/file.h>
#include <linux/syscalls.h>
#include <linux/list.h>
#include <ali_cache.h>
#include <ali_soc.h>
#include <linux/clk-provider.h>

#include <ca_dsc.h>
#include <ca_otp_dts.h>
#include "ca_scr_priv.h"
#include "ca_scr_ioctl.h"
#include "ca_scr_sysfs.h"
#include "ca_scr_dbgfs.h"
#include "ca_scr_rpc.h"

#define NO_CHRDEVS (1)
#define FIRST_MIN (0)

static int ca_scr_open(struct inode *inode, struct file *file)
{
	struct ca_scr_dev *scr = container_of(inode->i_cdev,
			struct ca_scr_dev, cdev);
	struct ca_scr_session *session;
	int ret;

	mutex_lock(&scr->mutex);

	if (scr->num_inst >= 8) {
		ret = -EBUSY;
		goto open_fail;
	}

	session = devm_kzalloc(scr->dev,
		sizeof(struct ca_scr_session), GFP_KERNEL);
	if (!session) {
		ret = -ENOMEM;
		goto open_fail;
	}

	scr->num_inst++;

	/*internal resource init*/
	memset(session, 0, sizeof(struct ca_scr_session));
	session->format = SCR_INVALID_SESSION_FORMAT;
	session->see_sess_id = SCR_INVALID_SESSION_ID;
	session->tsc_flag = SCR_INVALID_TSC_FLAG;
	session->crypt_mode = SCR_INVALID_CRYPTO_MODE_FLAG;
	session->parity = SCR_INVALID_PARITY_FLAG;
	session->algo = SCR_INVALID_ALGO_FLAG;

	INIT_LIST_HEAD(&session->key_list);
	ida_init(&session->key_ida);

	mutex_init(&session->rd_mutex);
	mutex_init(&session->wr_mutex);

	session->scr = scr;
	file->private_data = (void *)session;

	/* test sbm */
	ret = scr_open_sbm(session);
	if (ret)
		goto open_sbm_failed;

	if (ca_scr_se_register(session) != 0) {
		dev_dbg(scr->dev, "scr_se: failed register se\n");
		goto se_register_failed;
	}

	session->id = ida_simple_get(&scr->sess_ida,
		0, 0, GFP_KERNEL);

	mutex_unlock(&scr->mutex);
	ca_scr_dbgfs_add_session(session);
	return 0;

se_register_failed:
	scr_close_sbm(session);
open_sbm_failed:
	mutex_destroy(&session->rd_mutex);
	mutex_destroy(&session->wr_mutex);
	ida_destroy(&session->key_ida);
open_fail:
	mutex_unlock(&scr->mutex);
	return ret;
}

static int ca_scr_release(struct inode *inode, struct file *file)
{
	struct ca_scr_session *session = file->private_data;
	struct ca_scr_dev *scr;
	struct scr_inst_key *key = NULL,  *_key = NULL;

	if (!session)
		return -EBADF;

	/*Do not release resource in debug*/
	if (session->scr->debug_mode)
		return 0;

	scr = session->scr;

	mutex_lock(&scr->mutex);

	scr->num_inst--;

	ca_scr_se_unregister(session);
	/* test sbm */
	scr_close_sbm(session);

	/*clean the session && key_list*/
	list_for_each_entry_safe(key, _key, &session->key_list, key_node)
		scr_key_delete(key);

	ida_destroy(&session->key_ida);

	low_scr_delete_session(session);

	mutex_destroy(&session->rd_mutex);
	mutex_destroy(&session->wr_mutex);

	file->private_data = NULL;

	ida_simple_remove(&scr->sess_ida, session->id);

#ifdef CONFIG_DEBUG_FS
	ca_scr_dbgfs_del_session(session);
#endif

	devm_kfree(scr->dev, session);

	mutex_unlock(&scr->mutex);
	return 0;
}

static unsigned int ca_scr_poll(struct file *file, poll_table *wait)
{
	struct ca_scr_session *s = file->private_data;
	struct ca_scr_se *engine;
	int ret;
	int w_mask = 0, r_mask = 0;

	if (!s)
		return -EBADF;

	engine = &s->engine;

	poll_wait(file, &engine->OutWq, wait);
	poll_wait(file, &engine->InWq, wait);

	ret = ca_scr_se_wr_avail(engine);
	if (ret)
		w_mask |= POLLOUT | POLLWRNORM;

	if (!w_mask)
		schedule_delayed_work(&engine->wq_w_checker, SCR_SCHE_DELAY);

	ret = ca_scr_se_buffer_done(engine);
	if (ret)
		r_mask |= POLLIN | POLLRDNORM;

	if (!r_mask)
		schedule_delayed_work(&engine->wq_r_checker, SCR_SCHE_DELAY);

	return w_mask | r_mask;
}

static ssize_t ca_scr_read(struct file *file, char __user *buf,
	size_t count, loff_t *f_pos)
{
	struct ca_scr_session *s = file->private_data;
	struct ca_scr_se *e;
	struct ca_scr_se_buffer *sbuf;
	int ret = 0, rd_bytes = 0;
	struct page *page;
	char *vaddr;
	int blocking = (file->f_flags & O_NONBLOCK) ? 0 : 1;
	int bsize;

	if (!s)
		return -EBADF;

	e = &s->engine;

	dev_dbg(e->session->scr->dev,
			"read: session#%d read request: %zd bytes\n",
			e->session->id, count);

	mutex_lock(&s->rd_mutex);

	while (count > 0) {
		/*check data available or not*/
		ret = ca_scr_se_buffer_done(e);
		if (blocking) {
			while (!ret) {
				if (schedule_timeout_interruptible(
					SCR_SCHE_DELAY)) {
					ret = -ERESTARTSYS;
					goto out;
				}
				ret = ca_scr_se_buffer_done(e);
			}
		} else if (!ret) {
			ret = -EAGAIN;
			break;
		}

		if (e->rd_tmpbuf.bfill) {
			bsize = min(count, e->rd_tmpbuf.bfill);

			if (copy_to_user(buf + rd_bytes,
					e->rd_tmpbuf.buf + e->rd_tmpbuf.boffset,
					bsize)) {
				ret = -EFAULT;
				goto out;
			}

			rd_bytes += bsize;
			count -= bsize;

			e->rd_tmpbuf.bfill -= bsize;
			e->rd_tmpbuf.boffset += bsize;
			if (e->rd_tmpbuf.bfill == 0)
				e->rd_tmpbuf.boffset = 0;

			continue;
		}

		/* dequeue buffer */
		sbuf = ca_scr_se_dequeue_buffer(e);
		if (!sbuf)
			continue;
		page = (e->in_place) ? sbuf->page : sbuf->opage;
		vaddr = kmap(page) + sbuf->o_off;

		/* fill userland buffer */
		bsize = min(count, (unsigned int)sbuf->len);
		if (copy_to_user(buf + rd_bytes, vaddr, bsize)) {
			ret = -EFAULT;
			kunmap(page);
			goto out;
		}

		/* resever the left data */
		if (bsize < sbuf->len) {
			memcpy(
				e->rd_tmpbuf.buf + e->rd_tmpbuf.boffset,
				vaddr + bsize,
				sbuf->len - bsize
			);

			e->rd_tmpbuf.bfill += (sbuf->len - bsize);
		}

		kunmap(page);

		/* increment read bytes */
		rd_bytes += bsize;
		/* decrement remaining bytes to read */
		count -= bsize;

		/* release buffer */
		e->ops->put_buffer(sbuf);
		blocking = 0;
	};

out:
	dev_dbg(e->session->scr->dev,
		"read: session#%d read returned %d bytes\n",
		e->session->id, rd_bytes);

	if (rd_bytes) {
		e->read_bytes += rd_bytes;
		ret = rd_bytes;
	}

	mutex_unlock(&s->rd_mutex);
	return ret;
}

ssize_t ca_scr_write(struct file *file, const char __user *buf,
	size_t count, loff_t *offset)
{
	struct ca_scr_session *s = file->private_data;
	struct ca_scr_se *e = NULL;
	ssize_t wr_bytes = 0;
	int ret = 0;
	int blocking = (file->f_flags & O_NONBLOCK) ? 0 : 1;

	if (!s)
		return -EBADF;

	if (!s->fmt_flag)
		return -EPERM;

	if (!count || (s->format != CA_FORMAT_RAW &&
		count % s->pkt_size))
		return -EINVAL;

	e = &s->engine;

	dev_dbg(e->session->scr->dev,
			"write: session#%d write request: %zd bytes\n",
			e->session->id, count);

	mutex_lock(&s->wr_mutex);

	while (count > 0) {
		struct ca_scr_se_buffer *sbuf = NULL;
		int bsize;

		ret = ca_scr_se_wr_avail(e);
		if (blocking) {
			while (!ret) {
				if (schedule_timeout_interruptible(
					SCR_SCHE_DELAY)) {
					ret = -ERESTARTSYS;
					goto out;
				}
				ret = ca_scr_se_wr_avail(e);
			}
		} else if (!ret) {
			ret = -EAGAIN;
			break;
		}

		if (count + e->wr_tmpbuf.bfill < s->pkt_size) {
			if (copy_from_user(e->wr_tmpbuf.buf + e->wr_tmpbuf.bfill,
					buf + wr_bytes, count)) {
				ret = -EFAULT;
				goto out;
			}

			e->wr_tmpbuf.bfill += count;
			wr_bytes += count;
			count -= count;
			continue;
		}

		/* get free buffer */
		sbuf = e->ops->get_buffer(e, NULL);
		if (!sbuf)
			break;

		/* consume the accumuated buffer first*/
		if (e->wr_tmpbuf.bfill) {
			memcpy(kmap(sbuf->page), e->wr_tmpbuf.buf,
				e->wr_tmpbuf.bfill);

			kunmap(sbuf->page);
			sbuf->len += e->wr_tmpbuf.bfill;
			sbuf->size -= e->wr_tmpbuf.bfill;

			memset(e->wr_tmpbuf.buf, 0, PAGE_SIZE);
			e->wr_tmpbuf.bfill = 0;
		}

		/* adjust block size */
		bsize = (count >= sbuf->size) ? sbuf->size : count;
		/* round down block size if needed in TS format */
		if (sbuf->type && ((bsize + sbuf->len) % sbuf->type) &&
			(bsize + sbuf->len) > sbuf->type) {
			bsize -= (bsize + sbuf->len) % sbuf->type;
		}

		if (copy_from_user(kmap(sbuf->page) + sbuf->len,
				buf + wr_bytes, bsize)) {
			e->ops->put_buffer(sbuf);
			ret = -EFAULT;
			kunmap(sbuf->page);
			goto out;
		}

		/* set off && len */
		sbuf->len += bsize;
		kunmap(sbuf->page);

		/* queue_work */
		if (ca_scr_se_enqueue_buffer(sbuf) < 0) {
			e->ops->put_buffer(sbuf);
			break;
		}

		wr_bytes += bsize;
		count -= bsize;
		blocking = 0;
	};

	/*enqueue the rest data that not enough packet_size*/
	if (e->wr_tmpbuf.bfill) {
		struct ca_scr_se_buffer *sbuf = NULL;

		sbuf = e->ops->get_buffer(e, NULL);
		if (!sbuf)
			return -ENOMEM;

		memcpy(kmap(sbuf->page), e->wr_tmpbuf.buf,
			e->wr_tmpbuf.bfill);

		kunmap(sbuf->page);
		e->ops->set_buffer(sbuf, 0, e->wr_tmpbuf.bfill);

		/* queue_work */
		if (ca_scr_se_enqueue_buffer(sbuf) < 0) {
			e->ops->put_buffer(sbuf);
			return -EFAULT;
		}

		memset(e->wr_tmpbuf.buf, 0, PAGE_SIZE);
		e->wr_tmpbuf.bfill = 0;
	}

out:
	dev_dbg(e->session->scr->dev,
		"write: session#%d insert end of block mark (bsize: %zd)\n",
		e->session->id, wr_bytes);

	if (wr_bytes) {
		e->write_bytes += wr_bytes;
		ret = wr_bytes;
	}

	mutex_unlock(&s->wr_mutex);
	return ret;
}

static int ca_scr_vm_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct ca_scr_se *engine = NULL;
	struct ca_scr_se_buffer *sbuf = NULL;
	struct ca_scr_session *s = vma->vm_private_data;
	int ret = 0;
	long bl;
	char *a;
	struct page *page = NULL;

	engine = &s->engine;

	while (!ret) {
		ret = wait_event_interruptible_timeout(
			engine->OutWq, ca_scr_se_buffer_done(engine),
			SCR_SCHE_DELAY);
		if (ret == -ERESTARTSYS) {
			dev_info(engine->session->scr->dev, "VM_FAULT_NOPAGE\n");
			return VM_FAULT_NOPAGE;
		}
	}

	mutex_lock(&s->rd_mutex);

	/* dequeue buffer */
	sbuf = ca_scr_se_dequeue_buffer(engine);

	mutex_unlock(&s->rd_mutex);

	if (!sbuf)
		return VM_FAULT_SIGBUS;

	dev_dbg(engine->session->scr->dev,
		"scr_vm_fault: buffer#%d release %d bytes for session#%d\n",
		sbuf->id, sbuf->len, engine->session->id);

	engine->mmap_bytes += sbuf->len;

	page = sbuf->opage;

	a = kmap(page);
	/* if last 32 bits of page are unused then they are used for length.
	 */
	bl = PAGE_SIZE - sbuf->len - sizeof(u32);
	if (bl >= 0) {
		u32 *p = (u32 *)(a + PAGE_SIZE - sizeof(u32));
		*p = sbuf->len; /* length */
	}
	/* avoid leaking of kernel memory to user land */
	if (bl > 0)
		memset(a + sbuf->len, 0, bl);
	else if (bl < 0)
		memset(a + sbuf->len, 0, PAGE_SIZE - sbuf->len);

	kunmap(page);

	/* remain current page then release the internal sbuf */
	get_page(page);
	engine->ops->put_buffer(sbuf);

	vmf->page = page;

	return 0;
}

static const struct vm_operations_struct ca_scr_vmops = {
	.fault = ca_scr_vm_fault,
};

static int ca_scr_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct ca_scr_session *s = file->private_data;

	if (!s)
		return -EBADF;

	if (s->engine.in_place)
		return -EPERM;

	vma->vm_flags |= VM_DONTCOPY | VM_DONTEXPAND | VM_NONLINEAR;
	vma->vm_private_data = s;
	vma->vm_ops = &ca_scr_vmops;

	return 0;
}


static void ca_scr_release_spd(struct splice_pipe_desc *s, unsigned int i)
{
	return;
}

static const struct pipe_buf_operations ca_scr_pipe_buf_ops = {
	.can_merge = 0,
	.map = generic_pipe_buf_map,
	.unmap = generic_pipe_buf_unmap,
	.confirm = generic_pipe_buf_confirm,
	.release = generic_pipe_buf_release,
	.steal = generic_pipe_buf_steal,
	.get = generic_pipe_buf_get,
};

static ssize_t ca_scr_splice_read(struct file *file, loff_t *ppos,
				  struct pipe_inode_info *pipe, size_t len,
				  unsigned int flags)
{
	struct ca_scr_session *s = file->private_data;
	struct ca_scr_se *engine;
	struct ca_scr_se_buffer *sbuf;
	struct ca_scr_se_buffer *sbufs[PIPE_DEF_BUFFERS];
	struct page *pages[PIPE_DEF_BUFFERS];
	struct partial_page partial[PIPE_DEF_BUFFERS];
	struct splice_pipe_desc spd = {
		.pages = pages,
		.partial = partial,
		.nr_pages_max = PIPE_DEF_BUFFERS,
		.flags = flags,
		.ops = &ca_scr_pipe_buf_ops,
		.spd_release = ca_scr_release_spd,
	};
	int count = len, i = 0, j;
	int ret = 0, rd_bytes = 0;
	int blocking = (file->f_flags & O_NONBLOCK) ? 0 : 1;

	if (!s)
		return -EBADF;

	if (s->engine.in_place)
		return -EPERM;

	dev_dbg(s->scr->dev,
			"splice_read: session#%d read request: %zd bytes\n",
			s->id, count);

	mutex_lock(&s->rd_mutex);

	engine = &s->engine;
	if (splice_grow_spd(pipe, &spd)) {
		ret = -ENOMEM;
		goto out;
	}

	while (count && (i < spd.nr_pages_max)) {
		/*check data available or not*/
		ret = ca_scr_se_buffer_done(engine);
		if (blocking) {
			while (!ret) {
				if (schedule_timeout_interruptible(
					SCR_SCHE_DELAY)) {
					ret = -ERESTARTSYS;
					goto out;
				}
				ret = ca_scr_se_buffer_done(engine);
			}
		} else if (!ret) {
			ret = -EAGAIN;
			break;
		}

		/* dequeue buffer */
		sbuf = ca_scr_se_dequeue_buffer(engine);
		if (!sbuf)
			continue;

		/* fill spd */
		spd.partial[i].len = sbuf->len;
		spd.partial[i].offset = sbuf->o_off;
		spd.pages[i] = sbuf->opage;

		sbufs[i] = sbuf;
		get_page(spd.pages[i]);

		/* increment read bytes */
		rd_bytes += sbuf->len;
		/* decrement remaining bytes to read */
		count -= sbuf->len;
		i++;

		blocking = 0;
	}

out:
	if (i) {
		spd.nr_pages = i;
		ret = splice_to_pipe(pipe, &spd);

		if (ret)
			engine->splice_read_bytes += ret;

		for (j = 0; j < (i - spd.nr_pages); j++)
			engine->ops->put_buffer(sbufs[j]);

		for (j = i - 1; j >= (i - spd.nr_pages); j--) {
			engine->ops->push_buffer(sbufs[j]);
			put_page(spd.pages[j]);
		}
	}

	splice_shrink_spd(&spd);
	mutex_unlock(&s->rd_mutex);
	return ret;
}

static int scr_from_pipe(struct pipe_inode_info *pipe,
	struct pipe_buffer *buf, struct splice_desc *sd)
{
	struct file *filp = sd->u.file;
	struct ca_scr_session *s = NULL;
	struct ca_scr_se *e = NULL;
	struct ca_scr_se_buffer *sbuf = NULL;
	int len;

	if (unlikely(!filp))
		return -EBADF;

	s = (struct ca_scr_session *)filp->private_data;
	if (unlikely(!s))
		return -EBADF;

	if (unlikely(!buf->page))
		return -EPIPE;

	e = &s->engine;

	if (!e->tmpbuf.page) {
		int o_fill, o_size;

		o_size = min(buf->len, sd->len);
		o_fill = o_size - (o_size % s->pkt_size);

		if (o_fill < o_size) {
			e->tmpbuf.pgfill = 0;
			e->tmpbuf.page = alloc_page(GFP_KERNEL | __GFP_DMA);
			if (!e->tmpbuf.page)
				return -ENOMEM;
		}

		len = 0;

		if (o_fill >= s->pkt_size) {
			len = o_fill;

			sbuf = e->ops->get_buffer(e, buf->page);
			if (!sbuf)
				return -ENOMEM;

			e->ops->set_buffer(sbuf, buf->offset, len);

			/* queue_work */
			if (ca_scr_se_enqueue_buffer(sbuf) < 0) {
				e->ops->put_buffer(sbuf);
				return -EFAULT;
			}
		}
	} else {
		int i_size, o_size = 0;

		i_size = s->pkt_size - e->tmpbuf.pgfill;
		o_size = min(buf->len, sd->len);
		o_size = min(i_size, o_size);

		memcpy(
			kmap(e->tmpbuf.page) + e->tmpbuf.pgfill,
			kmap(buf->page) + buf->offset,
			o_size
		);

		kunmap(e->tmpbuf.page);
		kunmap(buf->page);

		e->tmpbuf.pgfill += o_size;
		len = o_size;

		if (e->tmpbuf.pgfill == s->pkt_size) {
			sbuf = e->ops->get_buffer(e, e->tmpbuf.page);
			if (!sbuf)
				return -ENOMEM;

			e->ops->set_buffer(sbuf, 0, s->pkt_size);
			sbuf->last = (sd->len >= sd->total_len);

			/* queue_work */
			if (ca_scr_se_enqueue_buffer(sbuf) < 0) {
				e->ops->put_buffer(sbuf);
				return -EFAULT;
			}

			put_page(e->tmpbuf.page);
			e->tmpbuf.page = NULL;
			e->tmpbuf.pgfill = 0;
		}
	}

	return len;
}

static ssize_t ca_scr_splice_write(struct pipe_inode_info *pipe,
				   struct file *filp, loff_t *ppos,
				   size_t count, unsigned int flags)
{
	struct ca_scr_session *s = filp->private_data;
	ssize_t ret;
	int blocking = (filp->f_flags & O_NONBLOCK) ? 0 : 1;
	struct ca_scr_se_buffer *sbuf = NULL;
	struct ca_scr_se *e = NULL;

	if (!s)
		return -EBADF;

	if (!s->fmt_flag)
		return -EPERM;

	if (!count || (s->format != CA_FORMAT_RAW &&
		count % s->pkt_size))
		return -EINVAL;

	if (s->engine.in_place)
		return -EPERM;

	dev_dbg(s->scr->dev,
		"splice_write: session#%d write request: %zd bytes\n",
		s->id, count);

	mutex_lock(&s->wr_mutex);

	ret = ca_scr_se_wr_avail(&s->engine);
	if (blocking) {
		while (!ret) {
			if (schedule_timeout_interruptible(SCR_SCHE_DELAY)) {
				ret = -ERESTARTSYS;
				goto out;
			}
			ret = ca_scr_se_wr_avail(&s->engine);
		}
	} else if (!ret) {
		ret = -EAGAIN;
		goto out;
	}

	ret = splice_from_pipe(pipe, filp, ppos, count, flags, scr_from_pipe);
	if (ret > 0) {
		s->engine.splice_write_bytes += ret;
		*ppos += ret;
	}

	/*enqueue the rest data that not enough packet_size*/
	e = &s->engine;
	if (e->tmpbuf.page && e->tmpbuf.pgfill) {
		sbuf = e->ops->get_buffer(e, e->tmpbuf.page);
		if (!sbuf)
			return -ENOMEM;

		e->ops->set_buffer(sbuf, 0, e->tmpbuf.pgfill);

		/* queue_work */
		if (ca_scr_se_enqueue_buffer(sbuf) < 0) {
			e->ops->put_buffer(sbuf);
			goto out;
		}

		put_page(e->tmpbuf.page);
		e->tmpbuf.page = NULL;
		e->tmpbuf.pgfill = 0;
	}

	dev_dbg(s->scr->dev,
		"splice_write: session#%d dsc_from_pipe %d bytes\n",
		s->id, ret);

out:
	mutex_unlock(&s->wr_mutex);
	return ret;
}


static const struct file_operations ca_scr_fops = {
	.owner		= THIS_MODULE,
	.open		= ca_scr_open,
	.read		= ca_scr_read,
	.write		= ca_scr_write,
	.poll			= ca_scr_poll,
	.mmap		= ca_scr_mmap,
	.splice_read	= ca_scr_splice_read,
	.splice_write	= ca_scr_splice_write,
	.release		= ca_scr_release,
	.unlocked_ioctl	= ca_scr_ioctl,
};

static int ca_scr_probe_dt(struct see_client *clnt,
	u32 *dev_index)
{
	struct device_node *dn = clnt->dev.of_node;
	const char *clk_name = NULL;
	struct clk *clk;
	int ret;

	dev_info(&clnt->dev, "parsing SCR@%d\n", clnt->service_id);

	of_property_read_string(dn, "clock-names", &clk_name);
	clk = devm_clk_get(&clnt->dev, clk_name);
	if (IS_ERR(clk)) {
		dev_dbg(&clnt->dev, "get clk error\n");
		return -EINVAL;
	}
	clk_prepare(clk);
	clk_enable(clk);

	ret = of_property_read_u32(dn, (const char *)"dev-index",
		dev_index);
	if (ret) {
		dev_dbg(&clnt->dev, "get dev-index error\n");
		return -EINVAL;
	}

	return 0;
}

static int ca_scr_probe(struct see_client *clnt)
{
	struct ca_scr_dev *scr;
	int ret = -1;
	char basename[16];
	u32 dev_index = 1;

	dev_info(&clnt->dev, "probing SCR SEE driver @%d\n", clnt->service_id);

	if (of_have_populated_dt()) {
		ret = ca_scr_probe_dt(clnt, &dev_index);
		if (ret < 0) {
			dev_dbg(&clnt->dev, "Failed to parse DT\n");
			return ret;
		}
	}

	scr = devm_kzalloc(&clnt->dev, sizeof(struct ca_scr_dev), GFP_KERNEL);
	if (!scr)
		return -ENOMEM;
	scr->clnt = clnt;

	sprintf(basename, "%s%d", CA_DSC_BASENAME, dev_index);

	/*
	* Character device initialisation
	*/
	ret = of_get_major_minor(clnt->dev.of_node,&scr->devt, 
			FIRST_MIN, NO_CHRDEVS, basename);
	if (ret  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		goto chrdev_alloc_fail;
	}

	cdev_init(&scr->cdev, &ca_scr_fops);
	ret = cdev_add(&scr->cdev, scr->devt, 1);
	if (ret < 0)
		goto cdev_add_fail;

	scr->class = class_create(THIS_MODULE, "ca_scr");
	if (IS_ERR(scr->class)) {
		ret = PTR_ERR(scr->dev);
		goto class_create_fail;
	}
	scr->dev = device_create(scr->class, &clnt->dev, scr->devt,
		scr, basename);
	if (IS_ERR(scr->dev)) {
		ret = PTR_ERR(scr->dev);
		goto device_create_fail;
	}

	mutex_init(&scr->mutex);
	/*open see scr module*/
	_scr_api_attach(scr);

	/* Init devices' handler at see*/
	scr->see_scr_id = hld_dev_get_by_id(HLD_DEV_TYPE_SCR, 0);
	if (NULL == scr->see_scr_id) {
		dev_dbg(&clnt->dev, "Get SCR handler error!\n");
		goto sysfs_fail;
	}

	/*for debug*/
	ret = ca_scr_sysfs_create(scr);
	if (ret)
		goto sysfs_fail;

	ret = ca_scr_wq_create();
	if (ret)
		goto sysfs_fail;

	ca_scr_dbgfs_create(scr);
	dev_set_drvdata(&clnt->dev, scr);
	dev_set_drvdata(scr->dev, scr);
	ida_init(&scr->sess_ida);

	scr->debug_mode = 0;
#ifdef CONFIG_DEBUG_FS
	scr->not_gothrough_hw = 0;
#endif
	dev_info(&clnt->dev, "driver probed\n");
	return 0;

	ca_scr_dbgfs_remove(scr);
	ca_scr_sysfs_remove(scr);
sysfs_fail:
	device_destroy(scr->class, scr->devt);
device_create_fail:
	class_destroy(scr->class);
class_create_fail:
	cdev_del(&scr->cdev);
cdev_add_fail:
	unregister_chrdev_region(scr->devt, NO_CHRDEVS);
chrdev_alloc_fail:
	devm_kfree(&clnt->dev, scr);
	return ret;
}

static int ca_scr_remove(struct see_client *clnt)
{
	struct clk *clk;
	struct device_node *dn = clnt->dev.of_node;
	const char *clk_name = NULL;
	struct ca_scr_dev *scr = dev_get_drvdata(&clnt->dev);
	if (!scr)
		return -ENODEV;

	dev_info(&clnt->dev, "removing SCR SEE driver @%d\n",
		clnt->service_id);

	of_property_read_string(dn, "clock-names", &clk_name);
	clk = devm_clk_get(&clnt->dev, clk_name);
	if (IS_ERR(clk)) {
		dev_dbg(&clnt->dev, "get clk error\n");
	} else {
		clk_disable(clk);
		clk_unprepare(clk);
	}

	ca_scr_dbgfs_remove(scr);
	ca_scr_sysfs_remove(scr);
	dev_set_drvdata(&clnt->dev, NULL);
	dev_set_drvdata(scr->dev, NULL);

	mutex_destroy(&scr->mutex);

	device_destroy(scr->class, scr->devt);
	class_destroy(scr->class);
	cdev_del(&scr->cdev);
	unregister_chrdev_region(scr->devt, NO_CHRDEVS);

	ca_scr_wq_delete();
	ida_destroy(&scr->sess_ida);

	devm_kfree(&clnt->dev, scr);
	dev_info(&clnt->dev, "driver removed\n");

	return 0;
}

static const struct of_device_id see_scr_matchtbl[] = {
	{ .compatible = "alitech,scr" },
	{ }
};

static struct see_client_driver scr_drv = {
	.probe	= ca_scr_probe,
	.remove	= ca_scr_remove,
	.driver	= {
		.name		= "SCR",
		.of_match_table	= see_scr_matchtbl,
	},
	.see_min_version = SEE_MIN_VERSION(0, 1, 1, 0),
};

module_see_client_driver(scr_drv);


MODULE_AUTHOR("ALi Corporation");
MODULE_DESCRIPTION("ALi Scramble Core");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.2.0");

