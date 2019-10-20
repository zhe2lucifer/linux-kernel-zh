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
#include <linux/clk-provider.h>
#include <ali_soc.h>

#include <ca_dsc.h>
#include <ca_otp_dts.h>
#include "ca_dsc_priv.h"
#include "ca_dsc_ioctl.h"
#include "ca_dsc_sysfs.h"
#include "ca_dsc_dbgfs.h"
#include "ca_dsc_rpc.h"
//===================================================================================================//

#define CA_DSC_VERSION "1.3.1"
#define NO_CHRDEVS (1)
#define FIRST_MIN (0)
//===================================================================================================//

static struct class *g_dsc_class;
#ifdef CONFIG_ALI_STANDBY_TO_RAM
extern int calc_aes_cmac(void);
#endif
//===================================================================================================//

static int ca_dsc_open(struct inode *inode, struct file *file)
{
	struct ca_dsc_dev *dsc = container_of(inode->i_cdev,
			struct ca_dsc_dev, cdev);
	struct ca_dsc_session *s;
	int ret;

	mutex_lock(&dsc->mutex);

	if (dsc->num_inst >= VIRTUAL_DEV_NUM) {
		ret = -EBUSY;
		goto open_fail;
	}

	s = devm_kzalloc(dsc->dev,
		sizeof(struct ca_dsc_session), GFP_KERNEL);
	if (!s) {
		ret = -ENOMEM;
		goto open_fail;
	}

	dsc->num_inst++;

	/*internal resource init*/
	memset(s, 0, sizeof(struct ca_dsc_session));
	s->dma_mode = DSC_INVALID_DMA_MODE;
	s->stream_id = ALI_INVALID_CRYPTO_STREAM_ID;
	s->sub_dev_id = ALI_INVALID_DSC_SUB_DEV_ID;
	s->tsc_flag = DSC_INVALID_TSC_FLAG;
	s->crypt_mode = DSC_INVALID_CRYPTO_MODE;
	s->parity = DSC_INVALID_PARITY;
	s->algo = DSC_INVALID_ALGO;
	s->sub_module = DSC_INVALID_SUB_MODULE;
	s->sub_dev_see_hdl = 0;

	INIT_LIST_HEAD(&s->key_list);
	ida_init(&s->key_ida);

	/* flags*/
	mutex_init(&s->rd_mutex);
	mutex_init(&s->wr_mutex);

	s->dsc = dsc;
	file->private_data = (void *)s;

	/* init sbm */
	ret = open_sbm(s);
	if (ret)
		goto open_sbm_failed;

	s->id = ida_simple_get(&dsc->sess_ida,
		0, 0, GFP_KERNEL);

	if (ca_dsc_se_register(s) != 0) {
		dev_dbg(dsc->dev, "dsc_se: failed register se\n");
		goto se_register_failed;
	}

	mutex_unlock(&dsc->mutex);
	ca_dsc_dbgfs_add_session(s);

	return 0;

se_register_failed:
	ida_simple_remove(&dsc->sess_ida, s->id);
	close_sbm(s);
open_sbm_failed:
	mutex_destroy(&s->rd_mutex);
	mutex_destroy(&s->wr_mutex);
	ida_destroy(&s->key_ida);
open_fail:
	mutex_unlock(&dsc->mutex);
	return ret;
}

static int ca_dsc_release(struct inode *inode, struct file *file)
{
	struct ca_dsc_session *s = file->private_data;
	struct ca_dsc_dev *dsc;
	struct ali_inst_key *key, *_key;

	if (!s)
		return -EBADF;

#ifdef CONFIG_DEBUG_FS
	/*Do not release resource in debug*/
	if (s->dsc->debug_mode)
		return 0;
#endif
	dsc = s->dsc;

	mutex_lock(&dsc->mutex);

	dsc->num_inst--;

	ca_dsc_se_unregister(s);

	/* release sbm */
	close_sbm(s);

	/*clean the key_list*/
	list_for_each_entry_safe(key, _key,
		&s->key_list, key_node) {
		dsc_delete_crypto_stream(s, key->key_handle);
		inst_key_delete(key);
	}

	ida_destroy(&s->key_ida);

	dsc_release_internel_resource(s);
	mutex_destroy(&s->rd_mutex);
	mutex_destroy(&s->wr_mutex);

	file->private_data = NULL;

	ida_simple_remove(&dsc->sess_ida, s->id);

#ifdef CONFIG_DEBUG_FS
	ca_dsc_dbgfs_del_session(s);
#endif

	devm_kfree(dsc->dev, s);

	mutex_unlock(&dsc->mutex);
	return 0;
}

static unsigned int ca_dsc_poll(struct file *file, poll_table *wait)
{
	struct ca_dsc_session *s = file->private_data;
	struct ca_dsc_se *engine;
	int ret;
	int w_mask = 0, r_mask = 0;

	if (!s)
		return -EBADF;

	engine = &s->engine;

	poll_wait(file, &engine->OutWq, wait);
	poll_wait(file, &engine->InWq, wait);

	ret = ca_dsc_se_wr_avail(engine);
	if (ret)
		w_mask |= POLLOUT | POLLWRNORM;

	if (!w_mask)
		schedule_delayed_work(&engine->wq_w_checker, 10);

	ret = ca_dsc_se_buffer_done(engine);
	if (ret)
		r_mask |= POLLIN | POLLRDNORM;
	if (!r_mask)
		schedule_delayed_work(&engine->wq_r_checker, 10);

	return w_mask | r_mask;
}

static ssize_t ca_dsc_read(struct file *file, char __user *buf,
	size_t count, loff_t *f_pos)
{
	struct ca_dsc_session *s = file->private_data;
	struct ca_dsc_se *e;
	struct ca_dsc_se_buffer *sbuf;
	int ret = 0, rd_bytes = 0;
	struct page *page;
	char *vaddr;
	int blocking = (file->f_flags & O_NONBLOCK) ? 0 : 1;
	int bsize;

	if (!s)
		return -EBADF;

	e = &s->engine;

	dev_dbg(s->dsc->dev, "read: session#%d read request: %zd bytes\n",
		s->id, count);

	mutex_lock(&s->rd_mutex);

	while (count > 0) {

		/*check data available or not*/
		ret = ca_dsc_se_buffer_done(e);
		if (blocking) {
			while (!ret) {
				if (schedule_timeout_interruptible(
					DSC_SCHE_DELAY)) {
					ret = -ERESTARTSYS;
					goto out;
				}
				ret = ca_dsc_se_buffer_done(e);
			}
		} else if (!ret) {
			ret = -EAGAIN;
			break;
		}

		/* dequeue buffer */
		sbuf = ca_dsc_se_dequeue_buffer(e);
		if (!sbuf)
			continue;
		page = (e->in_place) ? sbuf->page : sbuf->opage;
		vaddr = kmap(page) + sbuf->o_off;

		/* fill userland buffer */
		bsize = min(count, sbuf->len);

		if (copy_to_user(buf + rd_bytes, vaddr, bsize)) {
			ret = -EFAULT;
			kunmap(page);
			goto out;
		}

		sbuf->o_off += bsize;
		sbuf->len -= bsize;

		kunmap(page);

		rd_bytes += bsize;
		count -= bsize;

		if (sbuf->len)
			/* put buffer back into the queue */
			e->ops->push_buffer(sbuf);
		else
			/* release the buffer */
			e->ops->put_buffer(sbuf);

		blocking = 0;
	};

out:
	dev_dbg(s->dsc->dev, "read: session#%d read returned %d bytes\n",
		s->id, rd_bytes);

	if (rd_bytes) {
		e->read_bytes += rd_bytes;
		ret = rd_bytes;
	}

	mutex_unlock(&s->rd_mutex);

	return ret;
}

ssize_t ca_dsc_write(struct file *file, const char __user *buf,
	size_t count, loff_t *pos)
{
	struct ca_dsc_session *s = file->private_data;
	struct ca_dsc_se *e = NULL;
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

	dev_dbg(s->dsc->dev, "write: session#%d count %zd\n", s->id, count);

	mutex_lock(&s->wr_mutex);

	/* Implicit cork in chaining mode */
	if (s->ts_chaining)
		s->opt = CA_SET_CORK;

	while (count > 0) {
		struct ca_dsc_se_buffer *sbuf = NULL;
		int bsize;

		ret = ca_dsc_se_wr_avail(e);
		if (blocking) {
			while (!ret) {
				if (schedule_timeout_interruptible(
					DSC_SCHE_DELAY)) {
					ret = -ERESTARTSYS;
					goto out;
				}

				ret = ca_dsc_se_wr_avail(e);
			}
		} else if (!ret) {
			ret = -EAGAIN;
			break;
		}

		bsize = min(count, (unsigned int)PAGE_SIZE);

		/* round down block size if needed in TS format */
		if (s->format != CA_FORMAT_RAW)
			bsize -= bsize % s->pkt_size;

		/* get free buffer */
		sbuf = e->ops->get_buffer(e, NULL);
		if (!sbuf) {
			ret = -ENOMEM;
			break;
		}

		if (sbuf->type && sbuf->type != s->pkt_size) {
			e->ops->put_buffer(sbuf);
			ret = -EIO;
			break;
		}

		if (copy_from_user(kmap(sbuf->page), buf + wr_bytes, bsize)) {
			e->ops->put_buffer(sbuf);
			ret = -EFAULT;
			kunmap(sbuf->page);
			break;
		}

		/* set off && len */
		sbuf->len += bsize;
		kunmap(sbuf->page);

		/* queue_work */
		ret = ca_dsc_se_enqueue_buffer(sbuf);
		if (ret < 0) {
			e->ops->put_buffer(sbuf);
			break;
		}

		wr_bytes += bsize;
		count -= bsize;
		blocking = 0;
	};

	if (wr_bytes) {
		e->write_bytes += wr_bytes;
		ret = wr_bytes;
	}

	dev_dbg(s->dsc->dev, "write: session#%d ret %zd\n", s->id, ret);
out:
	mutex_unlock(&s->wr_mutex);
	return ret;
}

static int ca_dsc_vm_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct ca_dsc_se *engine = NULL;
	struct ca_dsc_se_buffer *sbuf = NULL;
	struct ca_dsc_session *s = vma->vm_private_data;
	int ret = 0;
	long bl;
	char *a;
	struct page *page = NULL;

	engine = &s->engine;

	while (!ret) {
		ret = wait_event_interruptible_timeout(
			engine->OutWq, ca_dsc_se_buffer_done(engine), 20);
		if (ret == -ERESTARTSYS) {
			dev_dbg(s->dsc->dev, "VM_FAULT_NOPAGE session #%d\n",
				s->id);
			return VM_FAULT_NOPAGE;
		}
	}

	mutex_lock(&s->rd_mutex);

	/* dequeue buffer */
	sbuf = ca_dsc_se_dequeue_buffer(engine);

	mutex_unlock(&s->rd_mutex);

	if (!sbuf)
		return VM_FAULT_SIGBUS;

	dev_dbg(s->dsc->dev,
		"dsc_vm_fault: buffer#%d release %d bytes for session#%d\n",
		sbuf->id, sbuf->len, s->id);

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

static const struct vm_operations_struct ca_dsc_vm_ops = {
	.fault = ca_dsc_vm_fault,
};

static int ca_dsc_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct ca_dsc_session *s = file->private_data;

	if (!s)
		return -EBADF;

	if (s->engine.in_place)
		return -EPERM;

	vma->vm_flags |= VM_DONTCOPY | VM_DONTEXPAND | VM_NONLINEAR;
	vma->vm_private_data = s;
	vma->vm_ops = &ca_dsc_vm_ops;

	return 0;
}

static void ca_dsc_release_spd(struct splice_pipe_desc *s, unsigned int i)
{
	return;
}

static const struct pipe_buf_operations ca_dsc_pipe_buf_ops = {
	.can_merge = 0,
	.map = generic_pipe_buf_map,
	.unmap = generic_pipe_buf_unmap,
	.confirm = generic_pipe_buf_confirm,
	.release = generic_pipe_buf_release,
	.steal = generic_pipe_buf_steal,
	.get = generic_pipe_buf_get,
};

static ssize_t ca_dsc_splice_read(struct file *file, loff_t *ppos,
				  struct pipe_inode_info *pipe, size_t count,
				  unsigned int flags)
{
	struct ca_dsc_session *s = file->private_data;
	struct ca_dsc_se *engine;
	struct ca_dsc_se_buffer *sbuf;
	struct ca_dsc_se_buffer *sbufs[PIPE_DEF_BUFFERS];
	struct page *pages[PIPE_DEF_BUFFERS];
	struct partial_page partial[PIPE_DEF_BUFFERS];
	struct splice_pipe_desc spd = {
		.pages = pages,
		.partial = partial,
		.nr_pages_max = PIPE_DEF_BUFFERS,
		.flags = flags,
		.ops = &ca_dsc_pipe_buf_ops,
		.spd_release = ca_dsc_release_spd,
	};
	int i = 0, j, remaining_len = 0, len;
	int ret = 0, rd_bytes = 0;
	int blocking = (file->f_flags & O_NONBLOCK) ? 0 : 1;

	if (!s)
		return -EBADF;

	if (s->engine.in_place)
		return -EPERM;

	dev_dbg(s->dsc->dev,
			"splice_read: session#%d request %zd bytes\n",
			s->id, count);

	mutex_lock(&s->rd_mutex);

	engine = &s->engine;
	if (splice_grow_spd(pipe, &spd)) {
		ret = -ENOMEM;
		goto out;
	}

	while ((count > 0) && (i < spd.nr_pages_max)) {
		/*check data available or not*/
		ret = ca_dsc_se_buffer_done(engine);
		if (blocking) {
			while (!ret) {
				if (schedule_timeout_interruptible(
					DSC_SCHE_DELAY)) {
					ret = -ERESTARTSYS;
					goto out;
				}
				ret = ca_dsc_se_buffer_done(engine);
			}
		} else if (!ret) {
			ret = -EAGAIN;
			break;
		}

		/* dequeue buffer */
		sbuf = ca_dsc_se_dequeue_buffer(engine);
		if (!sbuf)
			continue;

		len = min(count, sbuf->len);
		remaining_len = sbuf->len - len;

		/* fill spd */
		spd.partial[i].len = len;
		spd.partial[i].offset = sbuf->o_off;
		spd.pages[i] = sbuf->opage;

		sbufs[i] = sbuf;
		get_page(spd.pages[i]);

		/* increment read bytes */
		rd_bytes += len;
		/* decrement remaining bytes to read */
		count -= len;

		i++;

		blocking = 0;
	}

out:
	if (i) {
		int k = i;

		/* transfer up to spd.nr_pages,
		 * spd.nr_pages contains back the remaining number of page to
		 * transfer. */
		spd.nr_pages = i;
		ret = splice_to_pipe(pipe, &spd);

		if (ret)
			engine->splice_read_bytes += ret;

		if (spd.nr_pages == 0 && remaining_len) {
			sbufs[i-1]->o_off += len;
			sbufs[i-1]->len = remaining_len;
			k--;
		}

		for (j = i - 1; j >= 0; j--) {
			if (j < k - spd.nr_pages)
				engine->ops->put_buffer(sbufs[j]);
			else
				engine->ops->push_buffer(sbufs[j]);

			if (j >= i - spd.nr_pages)
				put_page(spd.pages[j]);
		}
	}

	splice_shrink_spd(&spd);
	mutex_unlock(&s->rd_mutex);

	dev_dbg(s->dsc->dev, "splice_read: session#%d ret %zd bytes\n",
		s->id, ret);
	return ret;
}

static inline int dsc_enqueue_page(struct ca_dsc_session *s, struct page *p,
				   int offs, int len)
{
	struct ca_dsc_se *e = &s->engine;
	struct ca_dsc_se_buffer *sbuf = e->ops->get_buffer(e, p);
	if (!sbuf)
		return -ENOMEM;

	e->ops->set_buffer(sbuf, offs, len);

	if (ca_dsc_se_enqueue_buffer(sbuf) < 0) {
		e->ops->put_buffer(sbuf);
		return -EFAULT;
	}
	return len;
}

static int dsc_from_pipe(struct pipe_inode_info *pipe,
	struct pipe_buffer *buf, struct splice_desc *sd)
{
	struct file *filp = sd->u.file;
	struct ca_dsc_session *s = NULL;
	struct ca_dsc_se *e = NULL;
	int o_size = min(buf->len, sd->len);;

	if (unlikely(!filp))
		return -EBADF;

	s = (struct ca_dsc_session *)filp->private_data;
	if (unlikely(!s))
		return -EBADF;

	if (unlikely(!buf->page))
		return -EPIPE;

	e = &s->engine;

	/* in raw mode always enqueue the buffer */
	if (s->pkt_size == PAGE_SIZE)
		return dsc_enqueue_page(s, buf->page, buf->offset,
					o_size);

	if (!e->tmpbuf.page) {
		int o_fill = o_size - (o_size % s->pkt_size);

		if (o_fill < o_size) {
			e->tmpbuf.pgfill = 0;
			e->tmpbuf.page = alloc_page(GFP_KERNEL | __GFP_DMA);
			if (!e->tmpbuf.page)
				return -ENOMEM;
		}

		if (o_fill)
			return dsc_enqueue_page(s, buf->page, buf->offset,
						o_fill);
	} else {
		int i_size = s->pkt_size - e->tmpbuf.pgfill;
		o_size = min(i_size, o_size);
	}

	memcpy(
		kmap(e->tmpbuf.page) + e->tmpbuf.pgfill,
		kmap(buf->page) + buf->offset,
		o_size);

	kunmap(e->tmpbuf.page);
	kunmap(buf->page);

	e->tmpbuf.pgfill += o_size;

	if (e->tmpbuf.pgfill == s->pkt_size) {
		int ret = dsc_enqueue_page(s, e->tmpbuf.page, 0, s->pkt_size);
		if (ret < 0)
			return ret;

		put_page(e->tmpbuf.page);
		e->tmpbuf.page = NULL;
		e->tmpbuf.pgfill = 0;
	}

	return o_size;
}

static ssize_t ca_dsc_splice_write(struct pipe_inode_info *pipe,
				   struct file *filp, loff_t *ppos,
				   size_t count, unsigned int flags)
{
	struct ca_dsc_session *s = filp->private_data;
	ssize_t ret;
	int blocking = (filp->f_flags & O_NONBLOCK) ? 0 : 1;

	if (!s)
		return -EBADF;

	if (!s->fmt_flag)
		return -EPERM;

	if (!count || (s->format != CA_FORMAT_RAW &&
		count % s->pkt_size))
		return -EINVAL;

	if (s->engine.in_place)
		return -EPERM;

	dev_dbg(s->dsc->dev,
		"splice_write: session#%d count %zd bytes\n", s->id, count);

	mutex_lock(&s->wr_mutex);

	/* implicit cork in chaining mode */
	if (s->ts_chaining)
		s->opt = CA_SET_CORK;

	ret = ca_dsc_se_wr_avail(&s->engine);
	if (blocking) {
		while (!ret) {
			if (schedule_timeout_interruptible(DSC_SCHE_DELAY)) {
				ret = -ERESTARTSYS;
				goto out;
			}
			ret = ca_dsc_se_wr_avail(&s->engine);
		}
	} else if (!ret) {
		ret = -EAGAIN;
		goto out;
	}

	ret = splice_from_pipe(pipe, filp, ppos, count, flags, dsc_from_pipe);
	if (ret < 0)
		goto out;

	s->engine.splice_write_bytes += ret;
	*ppos += ret;

	dev_dbg(s->dsc->dev,
		"splice_write: session#%d dsc_from_pipe %d bytes\n",
		s->id, ret);

out:
	mutex_unlock(&s->wr_mutex);
	return ret;
}

static const struct file_operations ca_dsc_fops = {
	.owner		= THIS_MODULE,
	.open		= ca_dsc_open,
	.read		= ca_dsc_read,
	.write		= ca_dsc_write,
	.poll			= ca_dsc_poll,
	.mmap		= ca_dsc_mmap,
	.splice_read	= ca_dsc_splice_read,
	.splice_write	= ca_dsc_splice_write,
	.release		= ca_dsc_release,
	.unlocked_ioctl	= ca_dsc_ioctl,
};

static int ca_dsc_probe_dt(struct see_client *clnt,
	u32 *dev_index)
{
	struct device_node *dn = clnt->dev.of_node;
	int ret;

	dev_dbg(&clnt->dev, "parsing DSC@%d\n", clnt->service_id);

	/*dev index*/
	ret = of_property_read_u32(dn, (const char *)"dev-index",
		dev_index);
	if (ret) {
		dev_dbg(&clnt->dev, "get dev-index error\n");
		return ret;
	}

	return 0;
}

#ifdef CONFIG_MEMORY_INTEGRITY_CHECKING
//HW integrity check to protect the MAIN CODE and RO_DATA section. 
static void ali_mem_background_checking(void)
{
	extern int _stext;
	extern int __end_rodata;

	u32 memchk_start, memchk_end;

	memchk_start = __pa(&_stext);
    memchk_end = __pa(&__end_rodata);

    memchk_start &= 0x0FFFFFFF;
    memchk_start |= 0xA0000000;
    memchk_end &= 0x0FFFFFFF;
    memchk_end |= 0xA0000000;
    
    ali_trig_ram_mon(memchk_start, memchk_end, 0, 0, 0);
}
#endif

static int ca_dsc_probe(struct see_client *clnt)
{
	struct ca_dsc_dev *dsc;
	int idx, ret = -1;
	char basename[16];
	u32 dev_index = 0;

	dev_dbg(&clnt->dev, "probing DSC@%d\n", clnt->service_id);

	if (of_have_populated_dt()) {
		ret = ca_dsc_probe_dt(clnt, &dev_index);
		if (ret < 0) {
			dev_dbg(&clnt->dev, "Failed to parse DT\n");
			return ret;
		}
	}

	dsc = devm_kzalloc(&clnt->dev, sizeof(struct ca_dsc_dev), GFP_KERNEL);
	if (!dsc)
		return -ENOMEM;
	dsc->clnt = clnt;

	sprintf(basename, "%s%d", CA_DSC_BASENAME, dev_index);

	dsc->type = dev_index;

	/*
	* Character device initialisation
	*/
	ret = of_get_major_minor(clnt->dev.of_node,&dsc->devt, 
			FIRST_MIN, NO_CHRDEVS, basename);
	if (ret  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		goto chrdev_alloc_fail;
	}

	cdev_init(&dsc->cdev, &ca_dsc_fops);
	ret = cdev_add(&dsc->cdev, dsc->devt, 1);
	if (ret < 0)
		goto cdev_add_fail;

	dsc->dev = device_create(g_dsc_class, &clnt->dev, dsc->devt,
		dsc, basename);
	if (IS_ERR(dsc->dev)) {
		ret = PTR_ERR(dsc->dev);
		goto device_create_fail;
	}

	mutex_init(&dsc->mutex);

	/*open see dsc module*/
	ali_m36_dsc_see_init();

	/* Init devices' handler at see*/
	dsc->see_dsc_id = hld_dev_get_by_id(HLD_DEV_TYPE_DSC, 0);
	if (NULL == dsc->see_dsc_id) {
		dev_dbg(&clnt->dev, "Get DSC handler error!\n");
		goto sysfs_fail;
	}

	for (idx = 0; idx < VIRTUAL_DEV_NUM; idx++) {
		dsc->see_aes_id[idx] = hld_dev_get_by_id(HLD_DEV_TYPE_AES, idx);
		dsc->see_des_id[idx] = hld_dev_get_by_id(HLD_DEV_TYPE_DES, idx);
		dsc->see_csa_id[idx] = hld_dev_get_by_id(HLD_DEV_TYPE_CSA, idx);
		dsc->see_sha_id[idx] = hld_dev_get_by_id(HLD_DEV_TYPE_SHA, idx);
		if (NULL == dsc->see_aes_id[idx] ||
			NULL == dsc->see_des_id[idx] ||
			NULL == dsc->see_csa_id[idx] ||
			NULL == dsc->see_sha_id[idx]) {
			dev_dbg(&clnt->dev, "Get Sub device's handler error!\n");
			goto sysfs_fail;
		}
	}
	
	ret = ca_dsc_sysfs_create(dsc);
	if (ret)
		goto sysfs_fail;

	ret = ca_dsc_wq_create();
	if (ret)
		goto sysfs_fail;

	ca_dsc_dbgfs_create(dsc);
	dev_set_drvdata(&clnt->dev, dsc);
	dev_set_drvdata(dsc->dev, dsc);
	ida_init(&dsc->sess_ida);

#ifdef CONFIG_MEMORY_INTEGRITY_CHECKING
	ali_mem_background_checking();
#endif

#ifdef CONFIG_DEBUG_FS
	dsc->debug_mode = 0;
	dsc->not_gothrough_hw = 0;
#endif
	dev_info(&clnt->dev, "probed ver %s\n", CA_DSC_VERSION);
	return 0;

	ca_dsc_dbgfs_remove(dsc);
	ca_dsc_sysfs_remove(dsc);
sysfs_fail:
	device_destroy(g_dsc_class, dsc->devt);
device_create_fail:
	cdev_del(&dsc->cdev);
cdev_add_fail:
	unregister_chrdev_region(dsc->devt, NO_CHRDEVS);
chrdev_alloc_fail:
	devm_kfree(&clnt->dev, dsc);
	return ret;
}

static int ca_dsc_remove(struct see_client *clnt)
{
	struct clk *clk;
	struct device_node *dn = clnt->dev.of_node;
	const char *clk_name = NULL;
	struct ca_dsc_dev *dsc = dev_get_drvdata(&clnt->dev);
	if (!dsc)
		return -ENODEV;

	dev_info(&clnt->dev, "removing DSC@%d\n",
		clnt->service_id);

	of_property_read_string(dn, "clock-names", &clk_name);
	clk = devm_clk_get(&clnt->dev, clk_name);

	if (IS_ERR(clk)) {
		dev_dbg(&clnt->dev, "get clk error\n");
	} else {
		clk_disable(clk);
		clk_unprepare(clk);
	}

	ca_dsc_dbgfs_remove(dsc);
	ca_dsc_sysfs_remove(dsc);

	dev_set_drvdata(&clnt->dev, NULL);
	dev_set_drvdata(dsc->dev, NULL);

	mutex_destroy(&dsc->mutex);

	device_destroy(g_dsc_class, dsc->devt);
	cdev_del(&dsc->cdev);
	unregister_chrdev_region(dsc->devt, NO_CHRDEVS);

	ca_dsc_wq_delete();
	ida_destroy(&dsc->sess_ida);

	devm_kfree(&clnt->dev, dsc);
	dev_info(&clnt->dev, "driver removed\n");
	return 0;
}

#ifdef CONFIG_ALI_STANDBY_TO_RAM
static int ca_dsc_suspend(struct device *dev)
{
	calc_aes_cmac();
	ali_m36_dsc_see_suspend();
	return 0;
}

static int ca_dsc_resume(struct device *dev)
{
	ali_m36_dsc_see_resume();
	return 0;
}

static struct dev_pm_ops dsc_drv_pm_ops = {
	.suspend = ca_dsc_suspend,
	.resume = ca_dsc_resume,
};
#endif

static const struct of_device_id see_dsc_matchtbl[] = {
	{ .compatible = "alitech,dsc" },
	{ }
};

static struct see_client_driver dsc_drv = {
	.probe	= ca_dsc_probe,
	.remove	= ca_dsc_remove,
	.driver	= {
		.name		= "DSC",
		.of_match_table	= see_dsc_matchtbl,
	#ifdef CONFIG_ALI_STANDBY_TO_RAM
		.pm = &dsc_drv_pm_ops,
	#endif
	},
	.see_min_version = SEE_MIN_VERSION(0, 1, 1, 0),
};

static int __init dsc_init(void)
{
	g_dsc_class = class_create(THIS_MODULE, CA_DSC_DRVNAME);
	if (IS_ERR(g_dsc_class))
		return PTR_ERR(g_dsc_class);

	return see_client_driver_register(&dsc_drv);
}

static void __exit dsc_exit(void)
{
	see_client_driver_unregister(&dsc_drv);
	class_destroy(g_dsc_class);
}

module_init(dsc_init);
module_exit(dsc_exit);


MODULE_AUTHOR("ALi Corporation");
MODULE_DESCRIPTION("ALi DeScramble Core");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(CA_DSC_VERSION);
