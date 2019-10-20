/*
 * Some splice utils for ASA
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
#include <linux/wait.h>
#include <linux/poll.h>

#include "ali_cert_asa_splice.h"
#include "ali_cert_asa_priv.h"

static const struct pipe_buf_operations cert_pipe_buf_ops = {
	.can_merge = 0,
	.map = generic_pipe_buf_map,
	.unmap = generic_pipe_buf_unmap,
	.confirm = generic_pipe_buf_confirm,
	.release = generic_pipe_buf_release,
	.steal = generic_pipe_buf_steal,
	.get = generic_pipe_buf_get,
};

static ssize_t cert_asa_pipe_size(struct pipe_inode_info *pipe)
{
	struct pipe_buffer *buf = NULL;
	int i = 0;
	size_t bytes = 0;

	pipe_lock(pipe);
	for (i = 0; i < pipe->nrbufs; i++) {
		buf = pipe->bufs + ((pipe->curbuf + i) &
						(pipe->buffers - 1));
		bytes += buf->len;
		#if 0
		pr_debug("buf:%p-%p, offset:0x%x, len:0x%x, curbuf:%d\n",
		buf, buf->page, buf->offset, buf->len,
		((pipe->curbuf + i) & (pipe->buffers - 1)));
		#endif
	}
	pipe_unlock(pipe);

	return bytes;
}

static void cert_asa_spd_release(struct splice_pipe_desc *spd,
		unsigned int i)
{
	page_cache_release(spd->pages[i]);
}

static ssize_t cert_asa_from_pipe(struct pipe_inode_info *pipe,
	struct file *out, loff_t *ppos, size_t len, unsigned int flags,
	splice_actor *actor)
{
	ssize_t ret;
	struct splice_desc sd = {
		.total_len = len,
		.flags = flags,
		.pos = *ppos,
		.u.file = out,
	};

	pipe_lock(pipe);
	ret = __splice_from_pipe(pipe, &sd,
			actor);
	pipe_unlock(pipe);

	return ret;
}

static ssize_t cert_asa_to_pipe(struct pipe_inode_info *pipe,
		       struct splice_pipe_desc *spd)
{
	int ret = 0;
	int page_nr = 0;

	pipe_lock(pipe);

	for (;;) {
		if (pipe->nrbufs < pipe->buffers) {
			int newbuf = (pipe->curbuf + pipe->nrbufs) &
						(pipe->buffers - 1);
			struct pipe_buffer *buf = pipe->bufs + newbuf;

			buf->page = spd->pages[page_nr];
			buf->offset = spd->partial[page_nr].offset;
			buf->len = spd->partial[page_nr].len;
			buf->private = spd->partial[page_nr].private;
			buf->ops = spd->ops;
			if (spd->flags & SPLICE_F_GIFT)
				buf->flags |= PIPE_BUF_FLAG_GIFT;

			pipe->nrbufs++;
			page_nr++;
			ret += buf->len;

			if (!--spd->nr_pages)
				break;
			if (pipe->nrbufs < pipe->buffers)
				continue;

			break;
		}
	}

	smp_mb();
	if (waitqueue_active(&pipe->wait))
		wake_up_interruptible_sync(&pipe->wait);
	pipe_unlock(pipe);

	return ret;
}

static int cert_asa_grow_spd(const struct pipe_inode_info *pipe,
	struct splice_pipe_desc *spd)
{
	unsigned int buffers = ACCESS_ONCE(pipe->buffers);

	spd->nr_pages_max = buffers;
	if (buffers <= PIPE_DEF_BUFFERS)
		return 0;

	spd->pages = kmalloc(buffers * sizeof(struct page *),
					GFP_KERNEL);
	spd->partial = kmalloc(buffers * sizeof(struct partial_page),
					GFP_KERNEL);

	if (spd->pages && spd->partial)
		return 0;

	kfree(spd->pages);
	kfree(spd->partial);
	return -ENOMEM;
}

static void cert_asa_shrink_spd(struct splice_pipe_desc *spd)
{
	if (spd->nr_pages_max <= PIPE_DEF_BUFFERS)
		return;

	kfree(spd->pages);
	kfree(spd->partial);
}

static const struct cert_asa_splice_ops cert_asa_ops_splice = {
	.to_pipe = cert_asa_to_pipe,
	.from_pipe = cert_asa_from_pipe,
	.spd_release = cert_asa_spd_release,
	.pipe_size = cert_asa_pipe_size,
	.grow_spd = cert_asa_grow_spd,
	.shrink_spd = cert_asa_shrink_spd,
	.buf_ops = &cert_pipe_buf_ops,
};

void cert_asa_splice_register(void *data)
{
	struct cert_asa_drv *drv = (struct cert_asa_drv *)data;
	struct cert_asa_splice_ops *ops = &drv->splice_ops;
	void *func = NULL;

	memcpy(ops, &cert_asa_ops_splice,
			sizeof(struct cert_asa_splice_ops));
	/**
	 * Depend on CONFIG_KALLSYMS
	 *
	 * If the "kallsyms_lookup_name" works, we use the original
	 * splice funcationality provided by "splice.c".
	 */
	func = (void *)kallsyms_lookup_name("splice_to_pipe");
	if (func)
		ops->to_pipe = func;
	func = (void *)kallsyms_lookup_name("splice_from_pipe");
	if (func)
		ops->from_pipe = func;
}

