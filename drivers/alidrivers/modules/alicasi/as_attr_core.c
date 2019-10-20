/*
 * AS Attribuite Core Dedicate Driver.   
 * Copyright(C) 2016 ALi Corporation. All rights reserved.
 *
 * This source code is part of the generic code that can be used
 * by all the as_attr drivers.
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
 
#include <linux/module.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <ali_cache.h>
#include <linux/idr.h>
#include <linux/kernel.h>

#include "as_attr_core.h"

static DEFINE_IDA(as_attr_ida);
static LIST_HEAD(as_attr_list);
static struct class *as_attr_class;
static dev_t as_attr_devt;

/*
 *	as_attr_open: open the /dev/?#id devices.
 *	@inode: inode of device
 *	@file: file handle to device
 */
static int as_attr_open(struct inode *inode, struct file *file)
{
	int ret;
	struct as_attr_device *as_attr = container_of(inode->i_cdev,\
					      struct as_attr_device, cdev);
	if (unlikely(!as_attr)) {
		ret = -EBADF;
		goto out_open;
	}
	mutex_lock(&as_attr->lock);
	/* the as_attr only support single open! */
	if (test_and_set_bit(AS_ATTR_DEV_OPEN, &as_attr->status)) {
		ret = -EBUSY;
		goto out_open;
	}
					      
	file->private_data = (void *)as_attr;
	if (!as_attr->ops->open) {
		ret = -EINVAL;
		goto out_open;
	}

	as_attr->ops->open(inode, file);
	mutex_unlock(&as_attr->lock);
	
	return nonseekable_open(inode, file);
out_open:
	mutex_unlock(&as_attr->lock);
	return ret;
}

static int as_attr_release(struct inode *inode, struct file *file)
{
	struct as_attr_device *as_attr = (struct as_attr_device *)file->private_data;

	if (unlikely(!as_attr))
		return -EBADF;
	mutex_lock(&as_attr->lock);
	/* make sure that /dev/?#id can be re-opened */
	clear_bit(AS_ATTR_DEV_OPEN, &as_attr->status);

	as_attr->ops->release(inode, file);
	
	file->private_data = NULL;
	mutex_unlock(&as_attr->lock);
	
	return 0;
}

static long as_attr_ioctl(struct file *filp, __u32 cmd, unsigned long arg)
{
	int err;
	struct as_attr_device *as_attr = (struct as_attr_device *)filp->private_data;
	
	if (!as_attr->ops->unlocked_ioctl)
		return -ENOIOCTLCMD;

	mutex_lock(&as_attr->lock);

	if (test_bit(AS_ATTR_UNREGISTERED, &as_attr->status)) {
		err = -ENODEV;
		goto out_ioctl;
	}

	err = as_attr->ops->unlocked_ioctl(filp, cmd, arg);

out_ioctl:
	mutex_unlock(&as_attr->lock);
	return err;

}

static const struct file_operations as_attr_fops = {
	.owner		= THIS_MODULE,
	.open		= as_attr_open,
	.release	= as_attr_release,
	.unlocked_ioctl	= as_attr_ioctl,
};

/*
 *	as_attr_dev_register: register a as_attr device
 *	@as_attr_dev:  as_attr device data
 *
 *	Register a as_attr device node. /dev/as_attr#id
 */
extern int of_get_major_minor(struct device_node *enode, dev_t *dev,
                       unsigned baseminor, unsigned count,const char *name);
static int as_attr_dev_register(struct as_attr_device *as_attr_dev)
{
	int err, devno;

	err = of_get_major_minor(as_attr_dev->device->of_node,&as_attr_devt, 
			0, MAX_AS_ATTR_DEVS, "as_attr");
	if (err < 0) {
		pr_err("as_attr: unable to allocate char dev region\n");
		class_destroy(as_attr_class);
		return err;
	}

	/* Fill in the data structures */
	devno = MKDEV(MAJOR(as_attr_devt), as_attr_dev->id);
	cdev_init(&as_attr_dev->cdev, &as_attr_fops);

	/* Add the device */
	err  = cdev_add(&as_attr_dev->cdev, devno, 1);
	if (err) {
		pr_err("as_attr%d unable to add device %d:%d\n",
			as_attr_dev->id,  MAJOR(as_attr_devt), as_attr_dev->id);
	}
	return err;
}

/*
 *	as_attr_dev_unregister: unregister a as_attr device
 *	@as_attr_dev: as_attr device data
 *
 */
static int as_attr_dev_unregister(struct as_attr_device *as_attr_dev)
{
	mutex_lock(&as_attr_dev->lock);
	set_bit(AS_ATTR_UNREGISTERED, &as_attr_dev->status);
	mutex_unlock(&as_attr_dev->lock);

	cdev_del(&as_attr_dev->cdev);
	
	return 0;
}

/**
 * as_attr_register_device() - register a as_attr device
 * @as_attr_dev: as_attr device
 *
 * Register a as_attr device with the kernel.
 *
 * A zero is returned on success and a negative errno code for
 * failure.
 */
int as_attr_register_device(struct as_attr_device *as_attr_dev)
{
	int ret, id, devno;

	if (!as_attr_dev || !as_attr_dev->name)
		return -EINVAL;

	mutex_init(&as_attr_dev->lock);
	id = ida_simple_get(&as_attr_ida, 0, MAX_AS_ATTR_DEVS, GFP_KERNEL);
	if (id < 0)
		return id;
	as_attr_dev->id = id;

	ret = as_attr_dev_register(as_attr_dev);
	if (ret) {
		ida_simple_remove(&as_attr_ida, id);
		return ret;
	}

	devno = as_attr_dev->cdev.dev;
	as_attr_dev->device = device_create(as_attr_class, NULL, devno,
					NULL, "ali_%s", as_attr_dev->name);
	if (IS_ERR(as_attr_dev->device)) {
		as_attr_dev_unregister(as_attr_dev);
		ida_simple_remove(&as_attr_ida, id);
		ret = PTR_ERR(as_attr_dev->device);
		return ret;
	}

	list_add(&as_attr_dev->list_node, &as_attr_list);

	return 0;
}
EXPORT_SYMBOL_GPL(as_attr_register_device);

/**
 * as_attr_unregister_device() - unregister a as_attr device
 * @as_attr_dev: as_attr device to unregister
 *
 * Unregister a as_attr device that was previously successfully
 * registered with as_attr_register_device().
 */
void as_attr_unregister_device(struct as_attr_device *as_attr_dev)
{
	int ret;
	int devno;

	if (as_attr_dev == NULL)
		return;

	devno = as_attr_dev->cdev.dev;
	ret = as_attr_dev_unregister(as_attr_dev);
	if (ret)
		pr_err("error unregistering /dev/?#id (err=%d)\n", ret);

	list_del(&as_attr_dev->list_node);	
	mutex_destroy(&as_attr_dev->lock);
	device_destroy(as_attr_class, devno);
	ida_simple_remove(&as_attr_ida, as_attr_dev->id);
	as_attr_dev->device = NULL;
}
EXPORT_SYMBOL_GPL(as_attr_unregister_device);

static int __init as_attr_init(void)
{
	as_attr_class = class_create(THIS_MODULE, "as_attr");
	if (IS_ERR(as_attr_class)) {
		pr_err("couldn't create class\n");
		return PTR_ERR(as_attr_class);
	}

	return 0;
}

static void __exit as_attr_exit(void)
{
	unregister_chrdev_region(as_attr_devt, MAX_AS_ATTR_DEVS);
	class_destroy(as_attr_class);
	ida_destroy(&as_attr_ida);
}

subsys_initcall(as_attr_init);
module_exit(as_attr_exit);

MODULE_AUTHOR("ALi Corporation");
MODULE_DESCRIPTION("ALi AS ATTR Core");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
