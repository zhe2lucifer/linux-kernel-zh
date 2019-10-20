/*
 * VSC Driver for Conax Virtual Smart Card
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
#include <linux/file.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/pagemap.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/of.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/reset.h>
#include <ali_soc.h>

#include <alidefinition/adf_vsc.h>
#include <alidefinition/adf_ce.h>

#include "ca_vsc_priv.h"
#include "ca_vsc.h"
#include "ca_vsc_rpc.h"


static struct ca_vsc_store *g_sdev;


void ca_vsc_store_mem_alloc(u32 *mem_addr)
{
	struct ca_vsc_store *sdev = g_sdev;
	struct ca_vsc_drv *drv = sdev->drv;
	dma_addr_t data_addr;

	*mem_addr = (u32) sdev->store.data;

	/* clear memory */
	memset(sdev->store.data, 0x00, VSC_STORE_DATA_SIZE);

	dev_dbg(sdev->dev, "%s,%d: ...\n", __func__, __LINE__);

	data_addr = dma_map_single(&drv->clnt->dev, sdev->store.data,
			VSC_STORE_DATA_SIZE, DMA_TO_DEVICE);
	if (dma_mapping_error(&drv->clnt->dev, data_addr)) {
		dev_err(&drv->clnt->dev, "%s: dma mapping failed!",
				__func__);
		return;
	}

	dma_sync_single_for_device(&drv->clnt->dev, data_addr,
			VSC_STORE_DATA_SIZE, DMA_TO_DEVICE);

	dma_unmap_single(&drv->clnt->dev, data_addr,
			VSC_STORE_DATA_SIZE, DMA_TO_DEVICE);
}

void ca_vsc_store_config(VSC_STORE_CONFIG *config, u32 *addr)
{
	struct ca_vsc_store *sdev = g_sdev;
	struct ca_vsc_drv *drv = sdev->drv;
	dma_addr_t data_addr;

	dev_dbg(sdev->dev, "%s,%d: ...\n", __func__, __LINE__);

	BUG_ON(*addr != (u32)sdev->store.data);

	memcpy(sdev->store.random_key, config->random_key[0],
			VSC_STORE_KEY_SIZE);

	memcpy(sdev->store.hash, config->hash[0],
			VSC_STORE_HASH_SIZE);

	data_addr = dma_map_single(&drv->clnt->dev, sdev->store.data,
			VSC_STORE_DATA_SIZE, DMA_FROM_DEVICE);
	if (dma_mapping_error(&drv->clnt->dev, data_addr)) {
		dev_err(&drv->clnt->dev, "dma mapping failed!");
		return;
	}

	dma_sync_single_for_cpu(&drv->clnt->dev, data_addr,
			VSC_STORE_DATA_SIZE, DMA_FROM_DEVICE);

	dma_unmap_single(&drv->clnt->dev, data_addr,
			VSC_STORE_DATA_SIZE, DMA_FROM_DEVICE);

	sdev->wb = 1;
	wake_up_interruptible(&sdev->wq);
}

static int ca_vsc_store_read(struct ca_vsc_store *sdev, int nonblock)
{
	int ret = 0;

	while (!sdev->wb) {
		if (nonblock)
			return -EAGAIN;

		mutex_unlock(&sdev->drv->mutex);

		ret = wait_event_interruptible(sdev->wq, sdev->wb);
		if (ret < 0) {
			mutex_lock(&sdev->drv->mutex);
			return -ERESTARTSYS;
		}

		mutex_lock(&sdev->drv->mutex);
	}

	sdev->wb = 0;

	return ret;
}

static int ca_vsc_store_write(
		struct ca_vsc_store *sdev,
		struct vsc_store *store)
{
	VSC_STORE_CONFIG config;
	int ret = 0;
	u8 wb = 0;
	dma_addr_t data_addr;
	struct ca_vsc_drv *drv = sdev->drv;

	dev_dbg(sdev->dev, "%s,%d:\n", __func__, __LINE__);

	if (store->data_len != VSC_STORE_DATA_SIZE)
		return -EINVAL;

	if (sdev->state & VSC_STATE_STORE_INITED)
		return -EBUSY;

	if (sdev->state & VSC_STATE_STORE_LOADED)
		return -EBUSY;

	config.index = 0;
	memcpy(config.random_key[0], store->random_key,
			VSC_STORE_KEY_SIZE);
	memcpy(config.hash[0], store->hash,
			VSC_STORE_HASH_SIZE);
	memcpy(config.random_key[1], store->random_key,
				VSC_STORE_KEY_SIZE);
	memcpy(config.hash[1], store->hash,
			VSC_STORE_HASH_SIZE);

	data_addr = dma_map_single(&drv->clnt->dev, store->data,
			VSC_STORE_DATA_SIZE, DMA_TO_DEVICE);
	if (dma_mapping_error(&drv->clnt->dev, data_addr)) {
		dev_err(&drv->clnt->dev, "dma mapping failed!");
		return -EADDRNOTAVAIL;
	}

	dma_sync_single_for_device(&drv->clnt->dev, data_addr,
			VSC_STORE_DATA_SIZE, DMA_TO_DEVICE);

	ret = vsc_decrypt_store((u32*)&store->data,
					&config, &config, &config.index, &wb);

	dma_unmap_single(&drv->clnt->dev, data_addr,
			VSC_STORE_DATA_SIZE, DMA_TO_DEVICE);

	if (ret) {
		dev_info(sdev->dev, "%s,%d: config write failed! ret=%d\n",
			__func__, __LINE__, ret);
		ret = -EIO;
		goto exit;
	}

	sdev->state |= VSC_STATE_STORE_LOADED;

exit:
	return ret;
}

static int ca_vsc_store_lib_init(struct ca_vsc_store *sdev,
	struct vsc_lib_init_params *init)
{
	int ret = 0;
	u32 lib_addr;
	u32 lib_len;

	/* TODO: addr currently not used */
#if 0
	/lib_addr = 0xa1d00000 + sdev->drv->pdata->lib_addr;
	lib_len = init->len > 0 ?
			init->len : sdev->drv->pdata->lib_len;
#else
	lib_addr = 0;
	lib_len = VSC_LIB_MAX_LEN;
#endif

	if (lib_len > VSC_LIB_MAX_LEN)
		return -EINVAL;

	if (sdev->state & VSC_STATE_STORE_INITED)
		return 0;

	ret = vsc_lib_init(&lib_addr, &lib_len);
	if (ret) {
		dev_info(sdev->dev, "%s: vsc_lib_init failed. ret=%08x!\n",
				__func__, ret);
		return -EIO;
	}

	sdev->state |= VSC_STATE_STORE_INITED;

	return 0;
}

void ca_vsc_store_clean(void)
{
	vsc_clear_store();
}

void ca_vsc_process_lib(void)
{
	vsc_process_lib();
}

static long ca_vsc_store_ioctl(struct file *file,
		__u32 cmd, unsigned long arg)
{
	int ret = -EIO;
	struct ca_vsc_store *sdev  = NULL;

	if (unlikely(!file || !file->private_data))
		return -EBADF;

	sdev = (struct ca_vsc_store *)file->private_data;
	if (unlikely(!sdev))
		return -EBADF;

	mutex_lock(&sdev->drv->mutex);

	switch (cmd) {
	case VSC_STORE_WRITE:
	{
		struct vsc_store *store = &sdev->store;
		struct vsc_store *tmp = (struct vsc_store __user *) arg;
		__u32 data_len = 0;

		ret = -EBUSY;
		if (sdev->state & VSC_STATE_STORE_INITED)
			break;

		ret = -EBUSY;
		if (sdev->state & VSC_STATE_STORE_LOADED)
			break;

		ret = -EINVAL;
		if (!tmp)
			break;

		ret = get_user(data_len, (int __user *)&tmp->data_len);
		if (ret) {
			ret = -EFAULT;
			break;
		}

		ret = -EINVAL;
		if (data_len != VSC_STORE_DATA_SIZE)
			break;
		if (!tmp->data)
			break;

		ret = copy_from_user(store->random_key,
				tmp->random_key, VSC_STORE_KEY_SIZE);
		if (ret) {
			ret = -EFAULT;
			break;
		}

		ret = copy_from_user(store->hash,
				tmp->hash, VSC_STORE_HASH_SIZE);
		if (ret) {
			ret = -EFAULT;
			break;
		}

		ret = copy_from_user(store->data,
				tmp->data, VSC_STORE_DATA_SIZE);
		if (ret) {
			ret = -EFAULT;
			break;
		}

		ret = ca_vsc_store_write(sdev, store);
		break;
	}

	case VSC_STORE_READ:
	{
		struct vsc_store *store = &sdev->store;
		struct vsc_store *tmp = (struct vsc_store __user *) arg;
		__u32 data_len = 0;

		ret = -EBUSY;
		if (!(sdev->state & VSC_STATE_STORE_INITED))
			break;

		ret = get_user(data_len, (int __user *)&tmp->data_len);
		if (ret) {
			ret = -EFAULT;
			break;
		}

		ret = -EINVAL;
		if (data_len != VSC_STORE_DATA_SIZE)
			break;
		if (!tmp->data)
			break;

		ret = ca_vsc_store_read(sdev, file->f_flags & O_NONBLOCK);
		if (ret)
			break;

		ret = put_user(store->data_len, &tmp->data_len);
		if (ret) {
			ret = -EFAULT;
			break;
		}

		ret = copy_to_user(tmp->random_key, store->random_key,
				VSC_STORE_KEY_SIZE);
		if (ret) {
			ret = -EFAULT;
			break;
		}

		ret = copy_to_user(tmp->hash, store->hash,
				VSC_STORE_HASH_SIZE);
		if (ret) {
			ret = -EFAULT;
			break;
		}

		ret = copy_to_user(tmp->data, store->data,
				VSC_STORE_DATA_SIZE);
		if (ret) {
			ret = -EFAULT;
			break;
		}

		break;
	}

	case VSC_LIB_INIT:
	{
		struct vsc_lib_init_params init;

		ret = copy_from_user(&init, (void __user *) arg,
				sizeof(struct vsc_lib_init_params));
		if (ret) {
			ret = -EFAULT;
			break;
		}

		ret = ca_vsc_store_lib_init(sdev, &init);
		break;
	}
	#ifdef _CAS9_VSC_RAP_ENABLE_
	case VSC_LIB_GET_KEY://Request keys from VSC_LIB
	{
		unsigned short key_id = 0;
		ret = copy_from_user((void *)&key_id,(void __user *)arg, sizeof(unsigned short));
		if (0 != ret)
		{
			ret = -EFAULT;
			break;
		}
		ret =vsc_lib_get_key(&key_id);
		if (0 != ret) {
			ret = -EFAULT;
		}
		break;
	}
	
	case VSC_CLEAR_STORE:
	{
		ca_vsc_store_clean();
		ret = 0;
		break;
	}
	#endif
	default:
	{
		dev_dbg(sdev->dev, "unsupported cmd=0x%x\n", cmd);
		ret = -ENOIOCTLCMD;
		break;
	}
	} /* switch */

	mutex_unlock(&sdev->drv->mutex);
	return ret;
}

unsigned int ca_vsc_store_poll(struct file *file, poll_table *wait)
{
	struct ca_vsc_store *sdev = NULL;
	unsigned int mask = 0;

	if (unlikely(!wait || !file))
		return -EBADF;

	sdev = (struct ca_vsc_store *)file->private_data;
	if (unlikely(!sdev))
		return -EBADF;

	mutex_lock(&sdev->drv->mutex);

	poll_wait(file, &sdev->wq, wait);
	if (sdev->wb)
		mask |= POLLIN | POLLRDNORM;

	mutex_unlock(&sdev->drv->mutex);

	return mask;
}

static int ca_vsc_store_open(struct inode *inode, struct file  *file)
{
	struct ca_vsc_store *sdev = NULL;

	if (unlikely(!inode || !file))
		return -EBADF;

	sdev = container_of(inode->i_cdev, struct ca_vsc_store, cdev);
	if (unlikely(!sdev))
		return -EBADF;

	mutex_lock(&sdev->drv->mutex);

	if (sdev->state & VSC_STATE_STORE_OPENED) {
		mutex_unlock(&sdev->drv->mutex);
		return -EBUSY;
	}

	init_waitqueue_head(&sdev->wq);
	sdev->wb = 0;
	sdev->state |= VSC_STATE_STORE_OPENED;
	file->private_data = sdev;

	mutex_unlock(&sdev->drv->mutex);
	return 0;
}

static int ca_vsc_store_close(struct inode *inode, struct file  *file)
{
	struct ca_vsc_store *sdev = NULL;

	if (unlikely(!inode || !file || !file->private_data))
		return -EBADF;

	sdev = container_of(inode->i_cdev, struct ca_vsc_store, cdev);
	if (unlikely(!sdev))
		return -EBADF;

	mutex_lock(&sdev->drv->mutex);

	if (!(sdev->state & VSC_STATE_STORE_OPENED)) {
		mutex_unlock(&sdev->drv->mutex);
		return -EBADF;
	}

	sdev->state &= ~VSC_STATE_STORE_OPENED;
	file->private_data = NULL;

	mutex_unlock(&sdev->drv->mutex);
	return 0;
}

static const struct file_operations vsc_store_fops = {
	.owner = THIS_MODULE,
	.open = ca_vsc_store_open,
	.release = ca_vsc_store_close,
	.poll = ca_vsc_store_poll,
	.unlocked_ioctl	= ca_vsc_store_ioctl,
	.compat_ioctl	= ca_vsc_store_ioctl,
};

int ca_vsc_store_probe(struct ca_vsc_drv *drv)
{
	int ret = -1;
	struct device_node *child;
	struct ca_vsc_store *sdev = NULL;

	sdev = devm_kzalloc(&drv->clnt->dev,
			sizeof(struct ca_vsc_store),
			GFP_KERNEL);
	if (!sdev)
		return -ENOMEM;

	child = of_get_child_by_name(drv->clnt->dev.of_node, VSC_STORE_DEVNAME);
	if (!child) {
		pr_err("Don't find child <functions> of DTS node<%s>!\n",VSC_STORE_DEVNAME);
		ret = alloc_chrdev_region(&sdev->devt, 0, 1, VSC_STORE_DEVNAME);
		if (ret < 0)
		{
			sdev->devt =0;
			pr_err("alloc ali dev_t fail\n");
			return -1;
		}
	}
	else{
		ret = of_get_major_minor(child,&sdev->devt, 0, 1, VSC_STORE_DEVNAME);
		if (ret  < 0) {
			pr_err("unable to get major and minor for char devive\n");
			return ret;
		}
	}
	cdev_init(&sdev->cdev, &vsc_store_fops);

	sdev->cdev.owner = THIS_MODULE;

	ret = cdev_add(&sdev->cdev, sdev->devt, 1);
	if (ret < 0)
		goto cleanup;

	sdev->dev_class = class_create(THIS_MODULE, VSC_STORE_DEVNAME);
	if (IS_ERR_OR_NULL(sdev->dev_class)) {
		ret = PTR_ERR(sdev->dev_class);
		goto cleanup;
	}

	sdev->dev = device_create(sdev->dev_class, NULL,
				sdev->devt, &sdev, VSC_STORE_DEVNAME);
	if (IS_ERR_OR_NULL(sdev->dev)) {
		ret = PTR_ERR(sdev->dev);
		goto cleanup;
	}

	/* TODO: debugfs */
	sdev->store.data_len = VSC_STORE_DATA_SIZE;
	sdev->store.data = devm_kzalloc(&drv->clnt->dev,
			VSC_STORE_DATA_SIZE, GFP_KERNEL/* | GFP_DMA*/);
	if (!sdev->store.data) {
		ret = -ENOMEM;
		dev_err(&drv->clnt->dev, "%s,%d: devm_kzalloc failed!\n",
				__func__, __LINE__);
		goto cleanup;
	}

	drv->store_dev = sdev;
	sdev->see_clnt = drv->clnt;
	sdev->state = 0;
	sdev->drv = drv;
	g_sdev = sdev;

	dev_dbg(&drv->clnt->dev, "VSC-STORE driver probed\n");

	return 0;

cleanup:
	if (sdev->dev_class) {
		device_destroy(sdev->dev_class, sdev->devt);
		cdev_del(&sdev->cdev);
		class_destroy(sdev->dev_class);
		unregister_chrdev_region(sdev->devt, 1);
	}
	devm_kfree(&drv->clnt->dev, sdev);

	return ret;
}

int ca_vsc_store_remove(struct ca_vsc_drv *drv)
{
	struct ca_vsc_store *sdev = drv->store_dev;

	/* TODO: debugfs */
	device_destroy(sdev->dev_class, sdev->devt);
	cdev_del(&sdev->cdev);
	unregister_chrdev_region(sdev->devt, 1);
	class_destroy(sdev->dev_class);

	devm_kfree(&drv->clnt->dev, sdev->store.data);
	sdev->store.data = NULL;

	devm_kfree(&drv->clnt->dev, sdev);
	drv->store_dev = NULL;
	g_sdev = NULL;
	dev_dbg(&drv->clnt->dev, "VSC-STORE driver removed\n");

	return 0;
}
