/*
 * CryptoFirewall Driver
 *
 * CF top layer driver, parsing the
 * global platform configurations and
 * probing the dev.
 *
 * Copyright (c) 2016 ALi Corp
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

#include "ali_cf_priv.h"
#include "ali_cf_dbgfs.h"
#include <ca_otp_dts.h>
#include <ca_kl.h>
#include <ali_soc.h>

static inline struct cf_drv *file2drv
(
	struct file *file
)
{
	return file ? file->private_data : NULL;
}

static long cf_ioctl
(
	struct file   *file,
	__u32	cmd,
	unsigned long  arg
)
{
	int ret = -EIO;
	struct cf_drv *drv = file2drv(file);

	if (unlikely(!drv || (drv->chan < 0)))
		return -EBADF;

	if (!arg)
		return -EINVAL;

	switch (cmd) {
	case ALI_CF_IOC_VERSION_INFO:
	{
		struct ali_cf_version version;
		ret = drv->see_ops->ioctl(drv->chan,
				ALI_CF_IO_VERSION_INFO, &version);
		if (0 != ret) {
			dev_dbg(drv->dev, "err IOC_VERSION_INFO\n");
			ret = cf_uret(ret);
			goto out;
		}
		ret = cf_umemcpy((void __user *)arg,
				(void *)&version, sizeof(version));
		if (unlikely(0 != ret)) {
			dev_dbg(drv->dev, "umemcpy, ret:%d\n", ret);
			goto out;
		}
		break;
	}

	case ALI_CF_IOC_FEATURE_VECTOR:
	{
		struct ali_cf_feature feature;
		ret = drv->see_ops->ioctl(drv->chan,
				ALI_CF_IO_FEATURE_VECTOR, &feature);
		if (0 != ret) {
			dev_dbg(drv->dev, "err IOC_FEATURE_VECTOR\n");
			ret = cf_uret(ret);
			goto out;
		}
		ret = cf_umemcpy((void __user *)arg,
				(void *)&feature, sizeof(feature));
		if (unlikely(0 != ret)) {
			dev_dbg(drv->dev, "umemcpy, ret:%d\n", ret);
			goto out;
		}
		break;
	}

	case ALI_CF_IOC_CF_STATUS:
	{
		struct ali_cf_cf_status cf_status;
		ret = drv->see_ops->ioctl(drv->chan,
				ALI_CF_IO_CF_STATUS, &cf_status);
		if (0 != ret) {
			dev_dbg(drv->dev, "err IOC_CF_STATUS\n");
			ret = cf_uret(ret);
			goto out;
		}
		ret = cf_umemcpy((void __user *)arg,
				(void *)&cf_status, sizeof(cf_status));
		if (unlikely(0 != ret)) {
			dev_dbg(drv->dev, "umemcpy, ret:%d\n", ret);
			goto out;
		}
		break;
	}

	case ALI_CF_IOC_TRANS_STATUS:
	{
		struct ali_cf_trans_status trans_status;
		ret = drv->see_ops->ioctl(drv->chan,
				ALI_CF_IO_TRANS_STATUS, &trans_status);
		if (0 != ret) {
			dev_dbg(drv->dev, "err IOC_TRANS_STATUS\n");
			ret = cf_uret(ret);
			goto out;
		}
		ret = cf_umemcpy((void __user *)arg,
				(void *)&trans_status, sizeof(trans_status));
		if (unlikely(0 != ret)) {
			dev_dbg(drv->dev, "umemcpy, ret:%d\n", ret);
			goto out;
		}
		break;
	}

	case ALI_CF_IOC_DECM_STATUS:
	{
		struct ali_cf_decm_status decm_status;
		ret = drv->see_ops->ioctl(drv->chan,
				ALI_CF_IO_DECM_STATUS, &decm_status);
		if (0 != ret) {
			dev_dbg(drv->dev, "err IOC_DECM_STATUS\n");
			ret = cf_uret(ret);
			goto out;
		}
		ret = cf_umemcpy((void __user *)arg,
				(void *)&decm_status, sizeof(decm_status));
		if (unlikely(0 != ret)) {
			dev_dbg(drv->dev, "umemcpy, ret:%d\n", ret);
			goto out;
		}
		break;
	}

	case ALI_CF_IOC_ISSUE_OP_FEATURE:
	case ALI_CF_IOC_ISSUE_OP_DIFF:
	case ALI_CF_IOC_ISSUE_OP_CWCORSHV:
	{
		struct ali_cf_operation operation;
		ret = cf_umemcpy((void *)&operation,
				(void __user *)arg, sizeof(operation));
		if (unlikely(0 != ret)) {
			dev_dbg(drv->dev, "umemcpy, ret:%d\n", ret);
			goto out;
		}
		ret = drv->see_ops->ioctl(drv->chan,
				(ALI_CF_IOC_ISSUE_OP_FEATURE == cmd) ?
				ALI_CF_IO_ISSUE_OP_FEATURE :
				((ALI_CF_IOC_ISSUE_OP_DIFF == cmd) ?
				ALI_CF_IO_ISSUE_OP_DIFF :
				ALI_CF_IO_ISSUE_OP_CWCORSHV), &operation);
		if (0 != ret) {
			dev_dbg(drv->dev, "err IOC_ISSUE_OP\n");
			ret = cf_uret(ret);
			goto out;
		}
		break;
	}

	case ALI_CF_IOC_READ_OP_RESULT:
	{
		struct ali_cf_result result;
		ret = drv->see_ops->ioctl(drv->chan,
				ALI_CF_IO_READ_OP_RESULT, &result);
		if (0 != ret) {
			dev_dbg(drv->dev, "err IOC_READ_OP_RESULT\n");
			ret = cf_uret(ret);
			goto out;
		}
		ret = cf_umemcpy((void __user *)arg,
				(void *)&result, sizeof(result));
		if (unlikely(0 != ret)) {
			dev_dbg(drv->dev, "umemcpy, ret:%d\n", ret);
			goto out;
		}
		break;
	}

	case ALI_CF_IOC_WAIT_OP_DONE:
	{
		struct ali_cf_trans_status cf_trans;
		ret = drv->see_ops->ioctl(drv->chan,
				ALI_CF_IO_WAIT_OP_DONE, &cf_trans);
		if (0 != ret) {
			dev_dbg(drv->dev, "err IOC_WAIT_OP_DONE\n");
			ret = cf_uret(ret);
			goto out;
		}
		ret = cf_umemcpy((void __user *)arg,
				(void *)&cf_trans, sizeof(cf_trans));
		if (unlikely(0 != ret)) {
			dev_dbg(drv->dev, "umemcpy, ret:%d\n", ret);
			goto out;
		}
		break;
	}

	default:
		dev_dbg(drv->dev, "unsupport cmd=0x%x\n", cmd);
		ret = -ENOTTY;
		break;
	}

out:
	return ret;
}

static int cf_open
(
	struct inode *inode,
	struct file  *file
)
{
	struct cf_drv *drv = NULL;

	if (unlikely(!inode || !file))
		return -EBADF;

	drv = container_of(inode->i_cdev, struct cf_drv, cdev);
	if (unlikely(!drv))
		return -EBADF;

	if (file->f_flags & O_NONBLOCK) {
		if (down_trylock(&drv->sem)) {
			dev_info(drv->dev, "cf busy, try again\n");
			return -EAGAIN;
		}
	} else
		down(&drv->sem);

	drv->chan = drv->see_ops->open();
	if (drv->chan < 0) {
		up(&drv->sem);
		return cf_uret(drv->chan);
	}

	file->private_data = (void *)drv;

	return 0;
}


static int cf_close
(
	struct inode *inode,
	struct file  *file
)
{
	struct cf_drv *drv = NULL;

	if (unlikely(!inode || !file))
		return -EBADF;

	drv = file->private_data;
	if (unlikely(!drv))
		return -EBADF;

	if (drv->chan >= 0)
		drv->see_ops->close(drv->chan);

	file->private_data = NULL;
	up(&drv->sem);

	return 0;
}

static ssize_t cf_write(struct file *file,
	const char __user *buf, size_t count, loff_t *f_pos)
{
	struct cf_drv *drv = file2drv(file);
	struct ali_cf_operation operation;
	int ret = -EIO;

	if (unlikely(!drv || (drv->chan < 0)))
		return -EBADF;

	if (sizeof(operation) != count)
		return -EINVAL;

	ret = cf_umemcpy((void *)&operation,
			(void __user *)buf, sizeof(operation));
	if (unlikely(0 != ret)) {
		dev_dbg(drv->dev, "umemcpy, ret:%d\n", ret);
		return ret;
	}

	ret = drv->see_ops->write(drv->chan,
			(void *)&operation, count);
	if (unlikely(0 != ret)) {
		dev_dbg(drv->dev, "umemcpy, ret:%d\n", ret);
		return cf_uret(ret);
	}
	return count;
}

static ssize_t cf_read(struct file *file,
	char __user *buf, size_t count, loff_t *f_pos)
{
	struct cf_drv *drv = file2drv(file);
	struct ali_cf_result result;
	int ret = -EIO;

	if (unlikely(!drv || (drv->chan < 0)))
		return -EBADF;

	if (sizeof(result) != count)
		return -EINVAL;

	ret = drv->see_ops->read(drv->chan,
			(void *)&result, sizeof(result));
	if (unlikely(0 != ret)) {
		dev_dbg(drv->dev, "see_ops read, ret:%d\n", ret);
		return cf_uret(ret);
	}

	ret = cf_umemcpy((void __user *)buf,
			 (void *)&result, sizeof(result));
	if (unlikely(0 != ret)) {
		dev_dbg(drv->dev, "umemcpy, ret:%d\n", ret);
		return ret;
	}

	return count;
}

static const struct file_operations g_cf_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = cf_ioctl,
	.open = cf_open,
	.release = cf_close,
	.write = cf_write,
	.read = cf_read
};

/* Get parameters from DTS */
static int cf_probe_dt(struct see_client *clnt)
{
	struct device_node *dn = clnt->dev.of_node;
	struct cf_plat_data *pdata = NULL;
	int ret = -1;

	pdata = devm_kzalloc(&clnt->dev, sizeof(struct cf_plat_data),
			     GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	/* get info from OTP defined in device tree */
	ret = of_parse_ali_otp(dn, "cf-disable",
				&pdata->is_cf_disabled);
	if (ret) {
		devm_kfree(&clnt->dev, pdata);
		return ret;
	}

	clnt->dev.platform_data = pdata;
	return ret;
}

static int cf_target_open(struct inode *inode, struct file *file)
{
	struct cf_drv *drv = NULL;

	if (unlikely(!inode || !file))
		return -EBADF;

	drv = container_of(inode->i_cdev, struct cf_drv, cdev);
	if (unlikely(!drv))
		return -EBADF;

	down(&drv->sem);
	file->private_data = (void *)drv;
	up(&drv->sem);

	return 0;	
}
static int cf_target_close(struct inode *inode, struct file *file)
{
	struct cf_drv *drv = NULL;

	if (unlikely(!inode || !file))
		return -EBADF;

	drv = file->private_data;
	if (unlikely(!drv))
		return -EBADF;

	down(&drv->sem);
	file->private_data = NULL;
	up(&drv->sem);

	return 0;
}

static long cf_target_ioctl
(
	struct file   *file,
	__u32	cmd,
	unsigned long  arg
)
{
	int ret = -EIO;
	struct cf_drv *drv = file2drv(file);

	if (unlikely(!drv))
		return -EBADF;

	if (!arg)
		return -EINVAL;

	down(&drv->sem);

	switch (cmd) {
	case ALI_CF_IOC_SET_CWC_TARGET:
	{
		struct ali_cf_cwc_target target;
		struct kl_key_cell *key_cell = NULL;
		ret = cf_umemcpy((void *)&target,
				(void __user *)arg, sizeof(target));
		if (unlikely(0 != ret)) {
			dev_dbg(drv->dev, "umemcpy, ret:%d\n", ret);
			goto out;
		}
		ret = fetch_key_cell_by_fd(target.fd, &key_cell);
		if (unlikely(!key_cell)) {
			dev_dbg(drv->dev, "err fetch key_cell\n");
			goto out;
		}

		if (KL_CK_KEY64 == key_cell->ck_size)
			key_cell->ck_parity = KL_CK_PARITY_ODD_EVEN;
		else
			key_cell->ck_parity = (CF_PARITY_EVEN == target.parity) ?
			KL_CK_PARITY_EVEN : KL_CK_PARITY_ODD;
		ret = drv->see_ops->set_target(
			key_cell->pos + ((key_cell->num > 1) ?
			((CF_PARITY_ODD == target.parity) ? 1 : 0) : 0));
		if (0 != ret) {
			dev_dbg(drv->dev, "err IOC_SET_CWC_TARGET\n");
			ret = cf_uret(ret);
			goto out;
		}
		break;
	}

	default:
		dev_dbg(drv->dev, "unsupport cmd=0x%x\n", cmd);
		ret = -ENOTTY;
		break;
	}

out:
	up(&drv->sem);
	return ret;
}

static const struct file_operations g_cf_target_fops = {
	.owner = THIS_MODULE,
	.open = cf_target_open,
	.release = cf_target_close,
	.unlocked_ioctl = cf_target_ioctl,
};

int cf_probe(struct see_client *clnt)
{
	int ret = -1;
	int i = 0;
	struct device_node *child = NULL;
	struct cf_plat_data *pdata = NULL;
	struct cf_drv *drv = NULL;

	const char *devs[] = {
		CF_DEV_NODE, CF_TARGET_DEV_NODE
	};

	const struct file_operations *f_ops[] = {
		&g_cf_fops, &g_cf_target_fops, 
	};

	drv = kzalloc(sizeof(devs)/sizeof(char *) *
			sizeof(struct cf_drv),
			GFP_KERNEL);
	if (!drv)
		return -ENOMEM;

	if (of_have_populated_dt()) {
		ret = cf_probe_dt(clnt);
		if (ret < 0) {
			dev_dbg(&clnt->dev, "Failed to parse DT\n");
			return ret;
		}
	}

	pdata = clnt->dev.platform_data;
	if (unlikely(!pdata)) {
		dev_dbg(&clnt->dev, "No configuration data\n");
		return -ENXIO;
	}

	if (pdata->is_cf_disabled) {
		dev_info(&clnt->dev, "No CF HW device\n");
		return -ENXIO;
	}

	for (i = 0; i < sizeof(devs)/sizeof(char *); i++) {
		sema_init(&drv[i].sem, 1);

		child = of_get_child_by_name(clnt->dev.of_node, devs[i]);
		if (!child) {
			pr_err("Don't find child <functions> of DTS node<%s>!\n",devs[i]);
			ret = alloc_chrdev_region(&drv[i].devt, 0, 1, devs[i]);
			if (ret < 0)
			{
				drv[i].devt =0;
				pr_err("alloc ali dev_t fail\n");
				return -1;
			}
		}
		else{
			ret = of_get_major_minor(child, &drv[i].devt, 
					0, 1, devs[i]);
			if (ret < 0) {
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

		drv[i].see_clnt = clnt;
		if (0 != cf_see_register(&drv[i])) {
			ret = -ENXIO;
			dev_dbg(&clnt->dev, "cf_see_register failed\n");
			goto cleanup;
		}
	}

	cf_dbgfs_create(drv);

	dev_set_drvdata(&clnt->dev, drv);
	dev_info(&clnt->dev, "CF driver probed\n");

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

int cf_remove(struct see_client *clnt)
{
	int i = 0;
	struct cf_drv *drv = dev_get_drvdata(&clnt->dev);

	if (!drv)
		return 0;

	dev_set_drvdata(&clnt->dev, NULL);

	down(&drv->sem);
	cf_dbgfs_remove(drv);
	cf_see_unregister(drv);

	up(&drv->sem);

	for (i = 0; i < 2; i++) {
		unregister_chrdev_region(drv[i].devt, 1);
		device_destroy(drv[i].dev_class, drv[i].devt);
		cdev_del(&drv[i].cdev);
		class_destroy(drv[i].dev_class);
	}

	kfree(drv);
	dev_info(&clnt->dev, "CF driver removed\n");
	return 0;
}

/**
ALi CF DTS node description:

Required properties :
- compatible : should be "alitech,cf"
- reg : unique index within the see_bus
- cf-disable : OTP to disable the CF hardware

Example:
	CF@7 {
			compatible = "alitech,cf";
			reg = <7>;
			cf-disable = <&otp 0x82 4 1>;
	};
*/
static const struct of_device_id see_cf_matchtbl[] = {
	{ .compatible = "alitech,cf" },
	{ }
};

static struct see_client_driver cf_driver = {
	.probe	= cf_probe,
	.remove	= cf_remove,
	.driver	= {
		.name = "CF",
		.of_match_table = see_cf_matchtbl,
	},
	.see_min_version = SEE_MIN_VERSION(0, 1, 1, 0),
};

module_see_client_driver(cf_driver);

MODULE_AUTHOR("ALi (Zhuhai) Corporation");
MODULE_DESCRIPTION("CF Driver for Advanced Security");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");

