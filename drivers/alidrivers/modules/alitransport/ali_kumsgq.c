/*
 * ALi Kernel-Userspace message queue implementation
 *
 * Copyright (C) 2014-2015 ALi Corporation - http://www.alitech.com

 * Authors: Christian Ruppert <christian.ruppert@alitech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/anon_inodes.h>
#include <linux/mutex.h>
#include <linux/spinlock.h> 
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/uaccess.h>

#include <linux/ali_kumsgq.h>


/* describes a single message which can be in several queues */
struct kumsg {
	int refcnt;
	struct kumsgq *msgq;
	struct mutex lock;
	unsigned long len;
	char data[];
};

struct kumsgelem {
	struct list_head list;
	struct kumsg *msg;
};

struct kumsgfile {
	struct file *file;
	struct list_head messages;
	spinlock_t messages_lock;
	struct list_head list;
	struct kumsgq *msgq;
};

#define ALI_KUMSGQ_EOF (1<<0)

struct kumsgq {
	struct list_head kumsgfiles;
	int refcnt;
	struct mutex files_lock;
	wait_queue_head_t msgwaitq;
	int flags;
};

static inline void ali_kumsgq_get(struct kumsgq *msgq)
{
	msgq->refcnt++;
}


/* !!! ATTENTION !!!
 * This function expects msgq->files_lock to be locked when called.
 * It will return with msgq->files_lock unlocked or msgq->files_lock destroyed.
 */
static void ali_kumsgq_put_locked(struct kumsgq *msgq)
{
	msgq->refcnt--;

	if (msgq->refcnt) {
		mutex_unlock(&msgq->files_lock);
		return;
	}

	mutex_unlock(&msgq->files_lock);

	mutex_destroy(&msgq->files_lock);

	kfree(msgq);
}

struct kumsgq *ali_new_kumsgq(void)
{
	struct kumsgq *msgq = kzalloc(sizeof(struct kumsgq), GFP_KERNEL);
	if (!msgq)
		return ERR_PTR(-ENOMEM);

	ali_kumsgq_get(msgq);

	INIT_LIST_HEAD(&msgq->kumsgfiles);

	init_waitqueue_head(&msgq->msgwaitq);

	mutex_init(&msgq->files_lock);

	return msgq;
}
EXPORT_SYMBOL(ali_new_kumsgq);

int ali_destroy_kumsgq(struct kumsgq *msgq)
{
	if(NULL == msgq)
	{
		return -1;
	}
	mutex_lock(&msgq->files_lock);

	msgq->flags |= ALI_KUMSGQ_EOF;
	wake_up_interruptible(&msgq->msgwaitq);

	ali_kumsgq_put_locked(msgq);
	msgq = NULL;
	return 0;
}
EXPORT_SYMBOL(ali_destroy_kumsgq);

static int ali_kumsgq_msg_push(struct kumsgfile *msgfile, struct kumsg *msg)
{
	struct kumsgelem *me;

	me = kmalloc(sizeof(struct kumsgelem), GFP_KERNEL);
	if (!me)
		return -ENOMEM;

	me->msg = msg;
	msg->refcnt++;

	spin_lock(&msgfile->messages_lock);
	list_add_tail(&me->list, &msgfile->messages);
	spin_unlock(&msgfile->messages_lock);

	return 0;
}

static struct kumsg *ali_kumsgq_msg_pop(struct kumsgfile *msgfile)
{
	struct kumsgelem *me;
	struct kumsg *msg;

	spin_lock(&msgfile->messages_lock);
	me = list_first_entry_or_null(&msgfile->messages,
					struct kumsgelem, list);
	if (!me) {
		spin_unlock(&msgfile->messages_lock);
		return NULL;
	}

	list_del(&me->list);
	spin_unlock(&msgfile->messages_lock);

	msg = me->msg;
	kfree(me);

	return msg;
}

static void ali_kumsgq_msg_done(struct kumsg *msg)
{
	mutex_lock(&msg->lock);

	msg->refcnt--;

	if (msg->refcnt) {
		mutex_unlock(&msg->lock);
		return;
	}

	mutex_unlock(&msg->lock);
	mutex_destroy(&msg->lock);
	kfree(msg);
}

int ali_kumsgq_sendmsg(struct kumsgq *msgq, void *msg_buf,
			unsigned long msg_size)
{
	struct kumsgfile *msgfile;
	struct kumsg *msg;
	int ret = 0;

	if(NULL == msgq)
	{
		return -ENOMEM;
	}
	
	mutex_lock(&msgq->files_lock);

	if (list_empty(&msgq->kumsgfiles))
		goto out;

	msg = kmalloc(sizeof(struct kumsg) + msg_size, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->refcnt = 0;
	mutex_init(&msg->lock);
	msg->msgq = msgq;
	msg->len = msg_size;
	memcpy(msg->data, msg_buf, msg_size);

	mutex_lock(&msg->lock);
	list_for_each_entry(msgfile, &msgq->kumsgfiles, list) {
		ret = ali_kumsgq_msg_push(msgfile, msg);
		if (ret)
			break;
	}
	mutex_unlock(&msg->lock);

	wake_up_interruptible(&msgq->msgwaitq);

out:
	mutex_unlock(&msgq->files_lock);

	return ret;
}
EXPORT_SYMBOL(ali_kumsgq_sendmsg);

static ssize_t ali_kumsgq_read(struct file *f, char __user *buf, size_t len,
				loff_t *offs)
{
	struct kumsgfile *msgfile = f->private_data;
	struct kumsgq *msgq = msgfile->msgq;
	struct kumsg *msg;
	int ret;

	msg = ali_kumsgq_msg_pop(msgfile);
	if (msg)
		goto do_read;

	if (msgq->flags & ALI_KUMSGQ_EOF)
		return 0;
	if ((f->f_flags & O_NONBLOCK) || !msg)
		return -EAGAIN;
		
#if 0
	ret = wait_event_interruptible(msgq->msgwaitq,
				(msg = ali_kumsgq_msg_pop(msgfile))
				|| (msgq->flags & ALI_KUMSGQ_EOF));
	if (ret < 0)
		return ret;
#endif

	if (!msg) {
		if (msgq->flags & ALI_KUMSGQ_EOF)
			return 0;
		else {
			BUG();
			return -EIO;
		}
	}

do_read:
	if (len > msg->len)
		len = msg->len;

	ret = len;

	if (copy_to_user(buf, msg->data, len))
		ret = -EFAULT;

	ali_kumsgq_msg_done(msg);

	return ret;
}

static unsigned int ali_kumsgq_poll(struct file *f,
					struct poll_table_struct *wait)
{
	struct kumsgfile *msgfile = f->private_data;
	struct kumsgq *msgq = msgfile->msgq;

	poll_wait(f, &msgq->msgwaitq, wait);

	if (!list_empty(&msgfile->messages))
		return POLLIN | POLLRDNORM;

	if (msgq->flags & ALI_KUMSGQ_EOF)
		return POLLHUP;

	return 0;
}

static int ali_kumsgq_release(struct inode *i, struct file *f)
{
	struct kumsgfile *msgfile = f->private_data;
	struct kumsgq *msgq = msgfile->msgq;
	struct kumsg *msg;

	mutex_lock(&msgq->files_lock);

	/* Flush pending messages */
	while ((msg = ali_kumsgq_msg_pop(msgfile)))
		ali_kumsgq_msg_done(msg);

	list_del(&msgfile->list);

	ali_kumsgq_put_locked(msgq);

	kfree(msgfile);

	return 0;
}

static const struct file_operations ali_kumsgq_fileops = {
	.read		= ali_kumsgq_read,
	.poll		= ali_kumsgq_poll,
	.release	= ali_kumsgq_release,
};

int ali_kumsgq_newfd(struct kumsgq *msgq, int flags)
{
	int ret, fd;
	struct kumsgfile *msgfile;

	if (((flags & O_ACCMODE) != O_RDONLY)
			|| (flags & ~(O_ACCMODE | O_NONBLOCK | O_CLOEXEC)))
		return -EINVAL;

	msgfile = kzalloc(sizeof(struct kumsgfile), GFP_KERNEL);
	if (!msgfile)
		return -ENOMEM;

	msgfile->msgq = msgq;
	INIT_LIST_HEAD(&msgfile->messages);
	spin_lock_init(&msgfile->messages_lock);

	ret = get_unused_fd_flags(O_RDWR | (flags & O_CLOEXEC));
	if (ret < 0)
		goto err_free_msgfile;
	fd = ret;

	msgfile->file = anon_inode_getfile("[ali_kumsgq]", &ali_kumsgq_fileops,
					msgfile, flags);
	if (IS_ERR(msgfile->file)) {
		ret = PTR_ERR(msgfile->file);
		goto err_put_fd;
	}

	fd_install(fd, msgfile->file);

	mutex_lock(&msgq->files_lock);

	list_add(&msgfile->list, &msgq->kumsgfiles);

	ali_kumsgq_get(msgq);

	mutex_unlock(&msgq->files_lock);

	return fd;

err_put_fd:
	put_unused_fd(fd);
err_free_msgfile:
	kfree(msgfile);
	return ret;
}
EXPORT_SYMBOL(ali_kumsgq_newfd);
