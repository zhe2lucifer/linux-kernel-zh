/*
 * CERT AKL Driver for Advanced Security Key Derivation
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
#include <linux/of.h>
#include <ali_soc.h>

#include "ali_cert_akl_priv.h"
#include "ali_cert_akl_dbgfs.h"

static int cert_akl_key_open(struct inode *inode, struct file *file);
static int cert_akl_key_close(struct inode *inode, struct file *file);

static const struct file_operations g_cert_akl_key_fops = {
	.owner = THIS_MODULE,
	.open = cert_akl_key_open,
	.release = cert_akl_key_close,
};

/* each algo has one corresponding kl */
static int algo2klsel(int algo)
{
	int ret = -ENOTSUPP;

	switch (algo) {
	case CA_AKL_FOR_CSA2:
		ret = 0;
		break;
	case CA_AKL_FOR_AES:
		ret = 1;
		break;
	case CA_AKL_FOR_TDES:
		ret = 2;
		break;
	case CA_AKL_FOR_CSA3:
		ret = 3;
		break;
	case CA_AKL_FOR_ASA:
		ret = 5;
		break;
	default:
		ret = -ENOTSUPP;
		break;
	}

	return ret;
}

static int cert_akl_key_put(struct kl_key_cell *cell)
{
	struct cert_akl_drv *drv = NULL;

	if (unlikely(!cell || !cell->private_data))
		return -EINVAL;

	if (unlikely(!cell->valid))
		return -EINVAL;

	BUG_ON(atomic_read(&cell->_count) <= 0);
	atomic_dec(&cell->_count);

	if (0 == atomic_read(&cell->_count)) {
		drv = (struct cert_akl_drv *)cell->private_data;

		if (cell->pos >= 0) {
			dev_dbg(drv->dev, "free pos: 0x%x\n", cell->pos);
			drv->see_ops->free(cell->kl_sel, cell->pos);
			if (cell->num > 1) {
				dev_dbg(drv->dev, "free pos: 0x%x\n",
					cell->pos + 1);
				drv->see_ops->free(cell->kl_sel, cell->pos + 1);
			}
		}
		memset(cell, CERT_AKL_INVALID_VALUE,
				sizeof(struct kl_key_cell));
		devm_kfree(drv->dev, cell);
	}

	return 0;
}

static int cert_akl_key_get(struct file *file,
	struct kl_key_cell **cell)
{
	struct cert_akl_key *key = NULL;
	struct cert_akl_drv *drv = NULL;

	if (unlikely(NULL == cell)) {
		dev_dbg(drv->dev, "*target is NULL\n");
		return -EINVAL;
	}

	if (unlikely(!file || !file->private_data))
		return -EBADF;

	drv = container_of(file->f_dentry->d_inode->i_cdev,
			struct cert_akl_drv, cdev);
	if (unlikely(!drv))
		return -EBADF;

	key = file->private_data;

	if (!key->cell || (key->cell->pos < 0)) {
		dev_dbg(drv->dev, "fd has no allocated key\n");
		return -ENODATA;
	}

	*cell = key->cell;

	dev_dbg(drv->dev, "Got key pos: 0x%x from akl key file\n",
		key->cell->pos);

	return 0;
}

static int cert_akl_key_aquire(int fd, int algo, int parity)
{
	struct file *file = NULL;
	struct cert_akl_key *key = NULL;
	struct cert_akl_drv *drv = NULL;
	int ret = -1;
	struct kl_key_cell *cell = NULL;

	if (unlikely(fd < 0))
		return -EINVAL;

	file = fget(fd);
	if (unlikely(!file))
		return -EBADF;

	if ((file->f_op != &g_cert_akl_key_fops) ||
		!file->private_data) {
		fput(file);
		return -EBADF;
	}

	key = file->private_data;
	drv = key->drv;

	down(&drv->sem);
	cell = key->cell;
	if (!cell) {
		cell = devm_kzalloc(drv->dev,
					sizeof(struct kl_key_cell),
					GFP_KERNEL);
		if (!cell) {
			ret = -ENOMEM;
			goto out;
		}
		memset(cell, CERT_AKL_INVALID_VALUE,
			sizeof(struct kl_key_cell));
	}

	if ((key->algo != algo) && (cell->pos >= 0)) {
		dev_dbg(drv->dev, "algo configured to others\n");
		ret = -EEXIST;
		goto out;
	}
	key->algo = algo;

	cell->kl_sel = algo2klsel(algo);
	if (unlikely(cell->kl_sel < 0)) {
		dev_dbg(drv->dev, "not support algo??\n");
		ret = cell->kl_sel;
		goto out;
	}
	cell->valid = 1;
	cell->num = (CA_AKL_FOR_CSA2 != algo) ? 2 : 1;
	cell->dev = drv->dev;
	cell->private_data = drv;
	cell->ck_parity = parity;
	cell->put_cell = cert_akl_key_put;
	cell->ck_size = (CA_AKL_FOR_CSA2 != algo) ? 1 : 0;

	ret = cell->pos;
	if (unlikely(cell->pos < 0)) {
		ret = drv->see_ops->alloc(cell->kl_sel, cell->num);
		if (ret > 0) {
			dev_dbg(drv->dev, "allocated pos: 0x%x, nr:%d\n",
				ret, cell->num);
			atomic_set(&cell->_count, 1);
			cell->pos = ret;
		} else {
			devm_kfree(drv->dev, cell);
			cell = NULL;
		}
	}
	key->cell = cell;

out:
	up(&drv->sem);
	fput(file);
	return ret;
}

static long cert_akl_ioctl
(
	struct file   *file,
	__u32	cmd,
	unsigned long  arg
)
{
	int ret = -EIO;
	struct cert_akl_drv *drv = NULL;
	struct ca_akl_cmd akl_cmd;
	struct ca_akl_key key;
	int pos = 0;

	if (unlikely(!file || !file->private_data))
		return -EBADF;

	drv = (struct cert_akl_drv *)file->private_data;
	if (unlikely(!drv || (drv->see_sess < 0)))
		return -EBADF;

	switch (CERT_IO_CMD(cmd)) {
	case CERT_IO_CMD(CA_IO_AKL_SAVEKEY):
	{
		ret = cert_umemcpy((void *)&key,
			(void __user *)arg, sizeof(key));
		if (unlikely(0 != ret)) {
			dev_dbg(drv->dev, "umemcpy, ret:%d\n", ret);
			goto out;
		}

		if ((key.parity != CA_AKL_EVEN_PARITY) &&
			(key.parity != CA_AKL_ODD_PARITY)) {
			dev_dbg(drv->dev, "error parity\n");
			return -EINVAL;
		}

		pos = cert_akl_key_aquire(key.keyfd,
				key.algo, key.parity);
		if (pos < 0) {
			dev_dbg(drv->dev, "aqurie key pos, ret:%d\n", pos);
			ret = pos;
			goto out;
		}

		dev_dbg(drv->dev, "aquired_pos=0x%x, algo=%d, parity=%d\n",
				pos, key.algo, key.parity);

		if (CA_AKL_FOR_CSA2 == key.algo)
			ret = drv->see_ops->savekey(drv->see_sess,
				key.algo, pos, key.parity);
		else {
			pos += (CA_AKL_ODD_PARITY == key.parity) ? 1 : 0;
			ret = drv->see_ops->savekey(drv->see_sess,
				key.algo, pos, key.parity);
		}

		if (0 != ret) {
			dev_dbg(drv->dev, "err see_akl_savekey\n");
			goto out;
		}

		break;
	}

	case CERT_IO_CMD(CA_IO_AKL_EXCHANGE):
	{
		ret = cert_umemcpy((void *)&akl_cmd,
					(void __user *)arg,
					sizeof(akl_cmd));
		if (unlikely(0 != ret)) {
			dev_dbg(drv->dev, "umemcpy, ret:%d\n", ret);
			goto out;
		}

		ret = drv->see_ops->exchange(drv->see_sess,
					(void *)&akl_cmd);
		if (0 != ret)
			dev_dbg(drv->dev, "err see_akl_exchange\n");

		cert_umemcpy((void __user *)arg,
					(void *)&akl_cmd,
					sizeof(akl_cmd));
		break;
	}

	case CERT_IO_CMD(CA_IO_AKL_ACK):
	{
		drv->see_ops->ack(drv->see_sess);
		ret = 0;
		break;
	}

	default:
		dev_dbg(drv->dev, "unsupprot cmd=0x%x\n", cmd);
		ret = -ENOIOCTLCMD;
		break;
	}

out:
	return ret;
}

static int cert_akl_open
(
	struct inode *inode,
	struct file  *file
)
{
	struct cert_akl_drv *drv = NULL;

	if (unlikely(!inode || !file))
		return -EBADF;

	drv = container_of(inode->i_cdev, struct cert_akl_drv, cdev);
	if (unlikely(!drv))
		return -EBADF;

	if (file->f_flags & O_NONBLOCK) {
		if (down_trylock(&drv->sem)) {
			dev_dbg(drv->dev, "cert-akl busy, try again\n");
			return -EAGAIN;
		}
	} else
		down(&drv->sem);

	drv->see_sess = drv->see_ops->open();
	if (drv->see_sess < 0) {
		up(&drv->sem);
		return drv->see_sess;
	}

	file->private_data = (void *)drv;

	return 0;
}


static int cert_akl_close
(
	struct inode *inode,
	struct file  *file
)
{
	struct cert_akl_drv *drv = NULL;

	if (unlikely(!inode || !file))
		return -EBADF;

	drv = file->private_data;
	if (unlikely(!drv))
		return -EBADF;

	if (drv->see_sess >= 0) {
		drv->see_ops->ack(drv->see_sess);
		drv->see_ops->close(drv->see_sess);
	}

	file->private_data = NULL;
	up(&drv->sem);

	return 0;
}


static int cert_akl_key_open
(
	struct inode *inode,
	struct file  *file
)
{
	struct cert_akl_drv *drv = NULL;
	struct cert_akl_key *key = NULL;

	if (unlikely(!inode || !file))
		return -EBADF;

	drv = container_of(inode->i_cdev, struct cert_akl_drv, cdev);
	if (unlikely(!drv))
		return -EBADF;

	if (atomic_read(&drv->key_c) >= CERT_AKL_KEY_MAX)
		return -EBUSY;

	key = devm_kzalloc(drv->dev,
			sizeof(struct cert_akl_key),
			GFP_KERNEL);
	if (NULL == key)
		return -ENOMEM;

	key->algo = CERT_AKL_INVALID_VALUE;
	key->drv = drv;
	cert_akl_dbgfs_add(key);
	file->private_data = key;
	atomic_inc(&drv->key_c);

	return 0;
}


static int cert_akl_key_close
(
	struct inode *inode,
	struct file  *file
)
{
	struct cert_akl_drv *drv = NULL;
	struct cert_akl_key *key = NULL;

	if (unlikely(!inode || !file || !file->private_data))
		return -EBADF;

	drv = container_of(inode->i_cdev, struct cert_akl_drv, cdev);
	if (unlikely(!drv))
		return -EBADF;

	key = file->private_data;

	down(&drv->sem);

	cert_akl_dbgfs_del(key);

	cert_akl_key_put(key->cell);

	devm_kfree(drv->dev, key);
	file->private_data = NULL;

	atomic_dec(&drv->key_c);

	up(&drv->sem);

	return 0;
}

static const struct file_operations g_cert_akl_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = cert_akl_ioctl,
	.open = cert_akl_open,
	.release = cert_akl_close,
};

int cert_akl_probe(struct cert_driver *parent)
{
	int ret = -1;
	int i = 0;
	struct device_node *child;

	const char *devs[] = {
		CERT_AKL_KEY_DEV, CERT_AKL_DEV
	};

	struct cert_akl_drv *drv = NULL;

	const struct file_operations *f_ops[] = {
		&g_cert_akl_key_fops, &g_cert_akl_fops
	};

	drv = kzalloc(sizeof(devs)/sizeof(char *) *
			sizeof(struct cert_akl_drv),
			GFP_KERNEL);
	if (!drv)
		return -ENOMEM;

	for (i = 0; i < sizeof(devs)/sizeof(char *); i++) {
		sema_init(&drv[i].sem, 1);

		child = of_get_child_by_name(parent->clnt->dev.of_node, devs[i]);
		if (!child) {
			pr_err("Don't find child <functions> of DTS node<%s>!\n",devs[i]);
			ret = alloc_chrdev_region(&drv[i].devt, 0, 1, devs[i]);
			if (ret < 0)
			{
				drv->devt = 0;
				return -1;
			}
		}
		else{
			ret = of_get_major_minor(child,&drv[i].devt, 0, 1, devs[i]);
			if (ret  < 0) {
				pr_err("unable to get major and minor for char devive\n");
				return ret;
			}
		}

		cdev_init(&drv[i].cdev, f_ops[i]);

		drv[i].cdev.owner = THIS_MODULE;

		ret = cdev_add(&drv[i].cdev, drv[i].devt, 1);
		if (ret < 0)
			goto cleanup;

		drv[i].dev_class = class_create(THIS_MODULE, devs[i]);
		if (IS_ERR_OR_NULL(drv[i].dev_class)) {
			ret = PTR_ERR(drv[i].dev_class);
			goto cleanup;
		}

		drv[i].dev = device_create(drv[i].dev_class, NULL,
					drv[i].devt, &drv[i], devs[i]);
		if (IS_ERR_OR_NULL(drv[i].dev)) {
			ret = PTR_ERR(drv[i].dev);
			goto cleanup;
		}

		drv[i].see_clnt = parent->clnt;
		if (0 != see_cert_akl_register(&drv[i])) {
			ret = -ENXIO;
			dev_dbg(&parent->clnt->dev, "see_cert_akl_register failed\n");
			goto cleanup;
		}
	}

	drv->fd_ops.fetch_key_cell = cert_akl_key_get;
	register_kl_callbacks(f_ops[0],	&drv->fd_ops);

	cert_akl_dbgfs_create(drv);

	parent->akl_drv = drv;
	dev_dbg(&parent->clnt->dev, "CERT-AKL driver probed\n");

	ret = 0;

cleanup:
	if (unlikely(ret)) {
		if (drv[i].dev_class) {
			device_destroy(drv[i].dev_class, drv[i].devt);
			cdev_del(&drv[i].cdev);
			class_destroy(drv[i].dev_class);
			unregister_chrdev_region(drv[i].devt, 1);
		}
		kfree(drv);
	}

	return ret;
}

int cert_akl_remove(struct cert_driver *parent)
{
	int i = 0;
	struct cert_akl_drv *drv = parent->akl_drv;

	down(&drv[1].sem);
	cert_akl_dbgfs_remove(drv);
	unregister_kl_callbacks(&g_cert_akl_key_fops);
	see_cert_akl_unregister(drv);

	up(&drv[1].sem);

	for (i = 0; i < 2; i++) {
		unregister_chrdev_region(drv[i].devt, 1);
		device_destroy(drv[i].dev_class, drv[i].devt);
		cdev_del(&drv[i].cdev);
		class_destroy(drv[i].dev_class);
	}
	kfree(drv);
	dev_dbg(&parent->clnt->dev, "CERT-AKL driver removed\n");
	return 0;
}

