/*
 * Key Ladder/DSC file descriptor dispatcher interface
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

#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mutex.h>

#include "ca_kl_fd_dispatch.h"


static LIST_HEAD(kl_drv_list);
static struct mutex kl_list_mutex;


struct kl_drv {
	const struct file_operations *file_ops;
	const struct kl_fd_operations *kl_ops;
	struct list_head list;
	unsigned int refcnt;
	struct mutex mutex;
};


static inline struct kl_drv *__get_kl_drv(
					const struct file_operations *file_ops,
					int release)
{
	struct kl_drv *drv;

	mutex_lock(&kl_list_mutex);

	list_for_each_entry(drv, &kl_drv_list, list) {
		if (drv->file_ops == file_ops) {
			mutex_lock(&drv->mutex);
			if (release)
				mutex_unlock(&kl_list_mutex);
			return drv;
		}
	}

	mutex_unlock(&kl_list_mutex);

	return ERR_PTR(-ENXIO);
}

static inline struct kl_drv *get_kl_drv(const struct file_operations *file_ops)
{
	return __get_kl_drv(file_ops, 1);
}

static inline void put_kl_drv(struct kl_drv *drv)
{
	mutex_unlock(&drv->mutex);
}

int fetch_key_cell_by_fd(int fd,
	struct kl_key_cell **cell)
{
	struct file *file;
	struct kl_drv *drv;
	int ret;
	int fput_needed;

	file = fget_light(fd, &fput_needed);
	if (!file)
		return -ENXIO;

	drv = get_kl_drv(file->f_op);
	if (IS_ERR(drv))
		ret = PTR_ERR(drv);
	else {
		ret = drv->kl_ops->fetch_key_cell(file, cell);
		put_kl_drv(drv);
	}

	fput_light(file, fput_needed);

	return ret;
}
EXPORT_SYMBOL(fetch_key_cell_by_fd);

int put_key_cell(struct kl_key_cell *cell)
{
	if (!cell)
		return -EINVAL;

	if (!cell->put_cell)
		return -EFAULT;

	return cell->put_cell(cell);
}
EXPORT_SYMBOL(put_key_cell);


int register_kl_callbacks(const struct file_operations *file_ops,
				const struct kl_fd_operations *kl_ops)
{
	struct kl_drv *drv;

	drv = get_kl_drv(file_ops);

	if (!IS_ERR(drv)) {
		if (drv->file_ops != file_ops)
			return -EADDRINUSE;

		drv->refcnt++;

		put_kl_drv(drv);

		return 0;
	}

	drv = kmalloc(sizeof(struct kl_drv), GFP_KERNEL);
	if (!drv)
		return -ENOMEM;

	mutex_init(&drv->mutex);
	drv->file_ops = file_ops;
	drv->kl_ops   = kl_ops;
	drv->refcnt   = 1;
	list_add(&drv->list, &kl_drv_list);

	return 0;
}
EXPORT_SYMBOL(register_kl_callbacks);

int unregister_kl_callbacks(const struct file_operations *file_ops)
{
	struct kl_drv *drv;

	drv = __get_kl_drv(file_ops, 0);
	if (IS_ERR(drv))
		return PTR_ERR(drv);

	drv->refcnt--;

	if (drv->refcnt) {
		mutex_unlock(&kl_list_mutex);
		put_kl_drv(drv);
		return 0;
	}

	list_del(&drv->list);

	mutex_unlock(&kl_list_mutex);

	put_kl_drv(drv);
	mutex_destroy(&drv->mutex);

	kfree(drv);

	return 0;
}
EXPORT_SYMBOL(unregister_kl_callbacks);

static int __init kl_cb_init(void)
{
	mutex_init(&kl_list_mutex);
	return 0;
}
module_init(kl_cb_init);

static void __exit kl_cb_exit(void)
{
	mutex_destroy(&kl_list_mutex);
}
module_exit(kl_cb_exit);

MODULE_DESCRIPTION("ALi key ladder dispatcher interface");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.1.0");
MODULE_AUTHOR("ALi Corporation");
