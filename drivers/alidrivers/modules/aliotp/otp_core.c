/*
 * One Time Programming Core driver
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
#include <linux/dma-mapping.h>
#include <linux/pagemap.h>
#include <linux/file.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/ali_rpc.h>
#include <rpc_hld/ali_rpc_hld.h>
#include <ali_soc.h>

#include "otp_priv.h"
#include "ca_otp.h"
#include "ca_otp_dts.h"

#define FUNC_HLD_OTP_GET_MUTEX 6

static int see_otp_get_mutex(int flag)
{
	jump_to_func(NULL, ali_rpc_call, flag, (HLD_BASE_MODULE<<24)|(1<<16)|FUNC_HLD_OTP_GET_MUTEX, NULL);
}

static int otp_open(struct inode *inode, struct file *file)
{
	struct otp_dev *otp = container_of(inode->i_cdev,
					      struct otp_dev, cdev);

	mutex_lock(&otp->mutex);
	otp->num_inst++;
	mutex_unlock(&otp->mutex);

	file->private_data = (void *)otp;

	return 0;
}

static int otp_release(struct inode *inode, struct file *file)
{
	struct otp_dev *otp = (struct otp_dev *)file->private_data;

	if (!otp)
		return -EBADF;

	mutex_lock(&otp->mutex);
	file->private_data = NULL;
	otp->num_inst--;
	mutex_unlock(&otp->mutex);

	return 0;
}

static long otp_ioctl(struct file *filp, __u32 cmd, unsigned long arg)
{
	int ret = 0, i;
	struct otp_dev *otp = NULL;
	struct otp_paras para;
	unsigned char *buf = NULL;
	struct flash_protect fp_key;
	struct otp_get_mrkcn mkrcn;
	
	otp = (struct otp_dev *)filp->private_data;
	if (!otp)
		return -EBADF;
		
	mutex_lock(&otp->mutex);
	see_otp_get_mutex(1);

	switch (cmd) {
	case ALI_OTP_READ:
	{
		ret = copy_from_user(&para, (void __user *)arg,
			sizeof(struct otp_paras));
		if (ret != 0)
			goto exit;

		if ((para.len <= 0) || (NULL == para.buf)) {
			ret = -EINVAL;
			goto exit;
		}

		buf = devm_kzalloc(otp->dev,
				para.len, GFP_KERNEL);
		if (NULL == buf) {
			ret = -ENOMEM;
			goto exit;
		}

		memset(buf, 0, para.len);
		ret = otp_hw_read(otp, para.offset, buf, para.len);
		if (ret != para.len) {
			devm_kfree(otp->dev, buf);
			goto exit;
		}

		ret = copy_to_user(para.buf, buf, para.len);
		if (ret != 0) {
			devm_kfree(otp->dev, buf);
			goto exit;
		}

		devm_kfree(otp->dev, buf);
		break;
	}

	case ALI_OTP_WRITE:
	{
		ret = copy_from_user(&para, (void __user *)arg,
			sizeof(struct otp_paras));
		if (ret != 0)
			goto exit;

		if ((para.len <= 0) || (NULL == para.buf)) {
			ret = -EINVAL;
			goto exit;
		}

		buf = devm_kzalloc(otp->dev,
				para.len, GFP_KERNEL);
		if (NULL == buf) {
			ret = -ENOMEM;
			goto exit;
		}

		ret = copy_from_user(buf, para.buf, para.len);
		if (ret != 0) {
			devm_kfree(otp->dev, buf);
			goto exit;
		}

		ret = otp_hw_write(otp, para.offset, buf, para.len);
		if (ret != para.len) {
			devm_kfree(otp->dev, buf);
			goto exit;
		} else {
			ret = 0;
		}

		devm_kfree(otp->dev, buf);
		break;
	}

	case ALI_OTP_GET_FP_KEY:
	{
		for (i = 0; i < 4; i++) {
			fp_key.buf[i] = ioread32(otp->base + OTP_FP_BASE + i * 4);
		}
		ret	= copy_to_user((void *)arg, &fp_key, sizeof(fp_key));
		if (ret != 0)
			goto exit;
			
		break;
	}

	case ALI_OTP_GET_OTP_MRKCN:
	{
		ret = copy_from_user(&mkrcn, (void __user *)arg,
			sizeof(struct otp_get_mrkcn));
		if (ret != 0)
			goto exit;

		if (0 != mkrcn.index && 1 != mkrcn.index) {
			ret = -EINVAL;
			goto exit;
		}
		mkrcn.cn = ioread32(otp->base + OTP_MRKCN_BASE + mkrcn.index * 4);
		ret = copy_to_user((void *)arg, &mkrcn, sizeof(struct otp_get_mrkcn));
		if (ret != 0)
			goto exit;
			
		break;
	}

	default:
		ret = -ENOIOCTLCMD;
		break;
	}

exit:
	see_otp_get_mutex(0);
	mutex_unlock(&otp->mutex);
	return ret;
}

static const struct file_operations otp_fops = {
	.owner		= THIS_MODULE,
	.open		= otp_open,
	.release	= otp_release,
	.unlocked_ioctl	= otp_ioctl,
};

int of_parse_ali_otp(struct device_node *dn, char *label,
	int *val)
{
	struct of_phandle_args otp;
	int ret;
	u32 mask, value;
	struct otp_dev *private;
	struct platform_device *otp_device;

	ret = of_parse_phandle_with_args(dn, label, "#otp-cells", 0, &otp);
	if (ret < 0)
		return ret;

	otp_device = of_find_device_by_node(otp.np);
	if (!otp_device)
		return -ENODEV;

	/* get OTP private data pointer */
	private = platform_get_drvdata(otp_device);
	if (!private)
		return -ENODEV;

	/* Decrement back the node refcount,
	 * it was incremented by of_parse_phandle_with_args
	 */
	of_node_put(otp.np);

	/* read OTP using privdate data pointer */
	ret = otp_hw_read(private, otp.args[0]*sizeof(u32),
		(unsigned char *)&value, sizeof(u32));
	if (ret != sizeof(u32))
		return ret;

	mask = (1 << (otp.args[2] /* number of bit */)) - 1;
	*val = (value >> otp.args[1]) & mask;

	return 0;
}
EXPORT_SYMBOL(of_parse_ali_otp);

int of_parse_ali_otp_list(struct device_node *dn,
	struct of_ali_otp_val *otps, int num_otps)
{
	int a;
	int ret;

	for (a = 0; a < num_otps; a++) {
		ret = of_parse_ali_otp(dn, otps[a].label, otps[a].valptr);
		if (ret)
			return ret;
	}
	return 0;
}
EXPORT_SYMBOL(of_parse_ali_otp_list);


static int of_prase_vendor_id(struct platform_device *pdev, 
	struct otp_vendor_id *pvid)
{
	int ret, array_num, len;
	int id_array[2];
	const char *p;
	
	if (!pdev || !pvid)
		goto err;
		
	p = of_get_property(pdev->dev.of_node,
		"vendor_id", &len);
	if (!p || !len) {
		dev_err(&pdev->dev,
			"vendor_id not found");
		goto err;
	}

	array_num = len / sizeof(u32);
	if (array_num != 2) {
		dev_err(&pdev->dev,
			"impossible num: %d", array_num);
		goto err;
	}

	ret = of_property_read_u32_array(pdev->dev.of_node, "vendor_id",
		id_array, array_num);
	if (ret) {
		dev_err(&pdev->dev,
			"get vendor id array failed!\n");
		goto err;
	}

	pvid->HighVID 	= id_array[0];
	pvid->LowVID 	= id_array[1];
	pvid->isValid 	= 1;
	
	return 0;
err:
	return -EINVAL;
}

static int of_trigger_kdf_otp(struct platform_device *pdev)
{
	int ret;
	const char *p;
	int root_text_idx_array[OTP_IDX_MAX_NUM];
	int array_num, len, i;
	struct otp_dev *otp;
	struct otp_vendor_id vid;

	if (!pdev)
		return -ENODEV;
	
	otp = (struct otp_dev *)platform_get_drvdata(pdev);
	if (!otp)
	{
		dev_info(&pdev->dev, "no otp dev\n");
		return -ENODEV;
	}

	p = of_get_property(pdev->dev.of_node,
		"kdf_root", &len);
	if (!p || !len) {
		dev_dbg(&pdev->dev,
			"no KDF info, exiting!\n");
		return 0;
	}

	memset(&vid, 0, sizeof(struct otp_vendor_id));
	of_prase_vendor_id(pdev, &vid);
	ret = otp_etsi_kdf_trigger(otp, &vid);
	if (ret) {
		dev_err(&pdev->dev, "otp kdf etsi trigger failed!\n");
		goto err;
	}

	array_num = len / sizeof(u32);
	if (array_num > OTP_IDX_MAX_NUM) {
		dev_err(&pdev->dev,
			"impossible num: %d", array_num);
		goto err;
	}

	ret = of_property_read_u32_array(pdev->dev.of_node, "kdf_root",
		root_text_idx_array, array_num);
	if (ret) {
		dev_dbg(&pdev->dev,
			"no KDF info, exiting!\n");
		return 0;
	}
	
	for (i = 0; i < array_num; i += 2) {
		ret = otp_std_kdf_trigger(otp, root_text_idx_array[i], 
				root_text_idx_array[i+1]);
		if (ret) {
			dev_err(&pdev->dev,
				"otp kdf std trigger failed!, root_idx = %d, text_idx = %d\n",
				root_text_idx_array[i], root_text_idx_array[i+1]);
			goto err;
		}
	}	

	return 0;
err:
	return -EINVAL;
}

static int otp_probe(struct platform_device *pdev)
{
	struct otp_dev *otp;
	int ret;
	struct resource *res = NULL;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_info(&pdev->dev, "no resource");
		return -ENODEV;
	}

	dev_info(&pdev->dev,
			"res[0].start:0x%08x, res[0].end:0x%08x\n",
			res->start, res->end);

	otp = devm_kzalloc(&pdev->dev, sizeof(struct otp_dev),
					GFP_KERNEL);
	if (!otp)
		return -ENOMEM;

	/*
	* Character device initialisation
	*/

	ret = of_get_major_minor(pdev->dev.of_node,&otp->devt, 
			0, 1, ALI_OTP_DEVNAME);
	if (ret  < 0) {
		pr_err("unable to get major and minor for char devive\n");
		goto chrdev_alloc_fail;
	}

	cdev_init(&otp->cdev, &otp_fops);
	ret = cdev_add(&otp->cdev, otp->devt, 1);
	if (ret < 0)
		goto cdev_add_fail;

	otp->class = class_create(THIS_MODULE, "ali_otp");
	if (IS_ERR(otp->class)) {
		ret = PTR_ERR(otp->dev);
		goto class_create_fail;
	}
	otp->dev = device_create(otp->class, &pdev->dev,
						otp->devt,
						otp, ALI_OTP_DEVNAME);
	if (IS_ERR(otp->dev)) {
		ret = PTR_ERR(otp->dev);
		goto device_create_fail;
	}

	mutex_init(&otp->mutex);
	platform_set_drvdata(pdev, otp);

	otp->base = devm_ioremap_resource(otp->dev, res);
	otp->memsize = OTP_MAX_MEM_SIZE;

	if (of_trigger_kdf_otp(pdev)) {
		dev_err(&pdev->dev,
			"triggger KDF from DTS failed!\n");
		goto kdf_trigger_fail;
	}

	otp_enable_io_private(otp);
	otp_legacy_interface_support(otp, 1);

	dev_info(&pdev->dev, "driver probed\n");
	return 0;

kdf_trigger_fail:
	device_destroy(otp->class, otp->devt);
device_create_fail:
	class_destroy(otp->class);
class_create_fail:
	cdev_del(&otp->cdev);
cdev_add_fail:
	unregister_chrdev_region(otp->devt, 1);
chrdev_alloc_fail:
	return ret;
}

static int otp_remove(struct platform_device *pdev)
{
	struct otp_dev *otp = platform_get_drvdata(pdev);
	if (!otp)
		return -ENODEV;

	platform_set_drvdata(pdev, NULL);
	mutex_destroy(&otp->mutex);
	device_destroy(otp->class, otp->devt);
	class_destroy(otp->class);
	cdev_del(&otp->cdev);
	unregister_chrdev_region(otp->devt, 1);

	devm_kfree(&pdev->dev, otp);
	dev_info(&pdev->dev, "driver removed\n");
	return 0;
}

static int otp_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int otp_resume(struct platform_device *pdev)
{
	int ret;
	struct otp_dev *otp;
#ifdef CONFIG_ALI_STR_DEBUG_ENABLE
	printk(KERN_EMERG "otp resume in...\n");
#endif

	otp = (struct otp_dev *)platform_get_drvdata(pdev);
	if (!otp)
	{
		dev_info(&pdev->dev, "no otp dev\n");
		return -ENODEV;
	}

	ret = of_trigger_kdf_otp(pdev);
	if (ret) {
		dev_err(&pdev->dev,
			"get kdf root idx failed!\n");
		return ret;
	}

	otp_enable_io_private(otp);

	return 0;
}


static const struct of_device_id ali_otp_dt_ids[] = {
	{ .name = "otp", .compatible = "alitech,otp" },
	{ }
};
MODULE_DEVICE_TABLE(of, ali_otp_dt_ids);

static struct platform_driver otp_driver = {
	.probe   = otp_probe,
	.remove  = otp_remove,
	.suspend = otp_suspend,
	.resume = otp_resume,
	.driver  = {
		.name  = "otp",
		.of_match_table = of_match_ptr(ali_otp_dt_ids),
		.owner = THIS_MODULE
	}
};

static int __init otp_init(void)
{
	return platform_driver_register(&otp_driver);
}

static void __exit otp_exit(void)
{
	platform_driver_unregister(&otp_driver);
}

rootfs_initcall(otp_init);
module_exit(otp_exit);

MODULE_AUTHOR("ALi Corporation");
MODULE_DESCRIPTION("ALi OTP Core");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");


