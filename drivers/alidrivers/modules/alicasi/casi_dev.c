/*
 * ALI CASI device driver
 * Copyright(C) 2016 ALi Corporation. All rights reserved.
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
#include <ca_dsc.h>

#include "as_attr_core.h"
#include "as_attr_utils.h"
#include "casi_dev.h"
#include "casi_rpc.h"
#include "ca_casi_common.h"
#include "../alikl/ca_kl_priv.h"

static int casi_dev_open(struct inode *inode, struct file *file)
{

	struct device *dev = NULL;
	struct ali_casi_entry *casi_entry = NULL;
	struct as_attr_device *casi = NULL;
	casi = container_of(inode->i_cdev,\
					      struct as_attr_device, cdev);
	if (unlikely(!casi))
		return -EBADF;
		
	casi_entry = container_of(casi,\
						struct ali_casi_entry, as_attr_dev);				
	if (unlikely(!casi_entry))
		return -EBADF;
		
	dev = casi_entry->dev;

	down(&casi_entry->sem);
	casi_entry->see_chan = casi_entry->see_ops->open();
	up(&casi_entry->sem);
	if (casi_entry->see_chan < 0) {
		dev_err(dev, "see chan is invalid\n");
		return -EREMOTEIO;	
	}
	
	return 0;
}

static int casi_dev_release(struct inode *inode, struct file *file)
{
	int ret;
	struct device *dev = NULL;
	struct ali_casi_entry *casi_entry = NULL;
	struct as_attr_device *casi = NULL;

	casi = (struct as_attr_device *)file->private_data;
	if (unlikely(!casi))
		return -EBADF;

	casi_entry = container_of(casi, \
						struct ali_casi_entry, as_attr_dev);
	if (unlikely(!casi_entry))
		return -EBADF;

	dev = casi_entry->dev;

	if (casi_entry->see_chan < 0)
		return -EINVAL;
		
	down(&casi_entry->sem);
	ret = casi_entry->see_ops->close(casi_entry->see_chan);
	up(&casi_entry->sem);
	if (ret < 0) {
		dev_err(dev, "see close operation error\n");
		return -EREMOTEIO;
	}
	
	return 0;
}

static long casi_dev_ioctl(struct file *filp, __u32 cmd, unsigned long arg)
{
	int ret;
	struct device *dev = NULL;
	struct as_attr_device *casi = NULL;
	struct ali_casi_entry *casi_entry = NULL;
	struct casi_ta_data_prm *p_casi_prm;
	struct page *page = NULL;
	void *addr = NULL;
	struct casi_see_ta_data_prm see_ta_prm;
	struct casi_kl_prm kl_prm;
	struct ca_kl_sub_dev *kl_sub_dev;
	struct kl_key_cell *key_cell;
	CE_NLEVEL_PARAM ce_nlevel_param;
	int i;
	casi = (struct as_attr_device *)filp->private_data;	
	if (unlikely(!casi))
		return -EBADF;

	casi_entry = container_of(casi, \
						struct ali_casi_entry, as_attr_dev);
	if (unlikely(!casi_entry))
		return -EBADF;

	dev = casi_entry->dev;

	if (casi_entry->see_chan < 0)
		return -EINVAL;
	down(&casi_entry->sem);
	switch (cmd) {
		case CASI_TA_DATA_SET:
		{
			page = alloc_page(GFP_KERNEL | __GFP_DMA);
			if (unlikely(!page)) {
				dev_err(dev, "alloc page failed!\n");
				ret = -ENOMEM;
				goto out;
			}
			addr = kmap(page);
			p_casi_prm = (struct casi_ta_data_prm *)addr;
			ret = as_attr_umemcpy((void *)p_casi_prm, \
				(void __user *)arg, sizeof(struct casi_ta_data_prm));
			if (unlikely(0 != ret)) {
				dev_err(dev, "error umemcpy, ret:%d\n", ret);
				goto out;
			}

			if (CA_ALGO_AES != p_casi_prm->algo && CA_ALGO_TDES != p_casi_prm->algo) {
				ret = -EINVAL;
				goto out;
			}
#ifdef CONFIG_ARM
			see_ta_prm.ta_data_phy = (__u32)virt_to_phys(p_casi_prm->ta_data)&0x7FFFFFFF;
#else
			see_ta_prm.ta_data_phy = (__u32)virt_to_phys(p_casi_prm->ta_data);
			see_ta_prm.ta_data_phy = (see_ta_prm.ta_data_phy&0x1FFFFFFF)|0xa0000000;
#endif
			see_ta_prm.algo = p_casi_prm->algo;
			ret = as_attr_get_dsc_res(p_casi_prm->fd, &casi_entry->as_attr_dev);
			if (ret < 0) {
				dev_err(dev, "fail to get dsc resource\n");
				goto out;
			}
			see_ta_prm.sub_dev_see_hdl = casi_entry->as_attr_dev.dsc_attr.dsc_ses_attr.sub_dev_see_hdl;
			see_ta_prm.stream_id = casi_entry->as_attr_dev.dsc_attr.dsc_ses_attr.stream_id;

			dma_sync_single_for_device(dev,\
						virt_to_phys(addr), CASI_TA_DATA_SIZE,
						DMA_TO_DEVICE);
			
			ret = casi_entry->see_ops->ioctl(casi_entry->see_chan,
				IO_CASI_TA_DATA_SET, (__u32)&see_ta_prm);
			if (0 != ret) {
				dev_err(dev, "ioctl IO_CASI_TA_DATA_SET failed\n");
				goto out;
			}
			
			break;
		}
		case CASI_GEN_NLEVEL_TA_KEY:
		{
			ret = as_attr_umemcpy((void *)&kl_prm, \
				(void __user *)arg, sizeof(struct casi_kl_prm));
			if (unlikely(0 != ret)) {
				dev_err(dev, "error umemcpy, ret:%d\n", ret);
				goto out;
			}
			
			ret = as_attr_get_kl_res(kl_prm.fd, &casi_entry->as_attr_dev);
			if (ret < 0) {
				dev_err(dev, "fail to get kl resource\n");
				goto out;
			}
			
			key_cell = casi_entry->as_attr_dev.kl_attr.key_cell;
			kl_sub_dev = (struct ca_kl_sub_dev *)key_cell->private_data;
			
			memset(&ce_nlevel_param, 0, sizeof(ce_nlevel_param));
			ce_nlevel_param.algo = casi_entry->as_attr_dev.kl_attr.algo;
			ce_nlevel_param.crypto_mode = casi_entry->as_attr_dev.kl_attr.crypt_mode;
			ce_nlevel_param.kl_index = key_cell->kl_sel;
			ce_nlevel_param.otp_addr = kl_sub_dev->root_key_addr;

			if (casi_entry->as_attr_dev.kl_attr.level > 1) {
				ce_nlevel_param.protecting_key_num = casi_entry->as_attr_dev.kl_attr.level - 1;
				for (i=0; i<ce_nlevel_param.protecting_key_num; i++)
				{
					memcpy(&(ce_nlevel_param.protecting_key[i*KL_KEY_SIZE_MAX]), 
						kl_prm.ce_gen_prm.pk[i], KL_KEY_SIZE_MAX);
				}
			}
			else {
				ce_nlevel_param.protecting_key_num = 0;
			}

			switch(kl_prm.ce_gen_prm.run_parity)
			{
				case KL_CK_PARITY_EVEN:
					{
						ce_nlevel_param.pos = key_cell->pos;

						if ((KL_ALGO_TDES == ce_nlevel_param.algo) && (KL_CK_KEY64 == key_cell->ck_size))
						{
							//gen 64bit even key...
							ce_nlevel_param.parity = CE_PARITY_EVEN;
							memcpy(&(ce_nlevel_param.content_key[8]), kl_prm.ce_gen_prm.key_even, 8);
						}
						else
						{
							//gen 128bit even key...
							ce_nlevel_param.parity = CE_PARITY_EVEN_ODD;
							memcpy(ce_nlevel_param.content_key, kl_prm.ce_gen_prm.key_even, KL_KEY_SIZE_MAX);
						}

						ret = casi_entry->see_ops->ioctl(casi_entry->see_chan, \
							IO_CASI_GEN_NLEVEL_TA_KEY, (__u32)&ce_nlevel_param);
						if (0 != ret) {
							dev_err(dev, "ioctl IO_CASI_GEN_NLEVEL_TA_KEY failed\n");
							goto out;
						}
					}
					break;
				case KL_CK_PARITY_ODD:
					{
						ce_nlevel_param.pos = (key_cell->ck_size == KL_CK_KEY128) ? 
								(key_cell->pos + 1) : (key_cell->pos);
						
						if ((KL_ALGO_TDES == ce_nlevel_param.algo) && (KL_CK_KEY64 == key_cell->ck_size))
						{
							//gen 64bit odd key
							ce_nlevel_param.parity = CE_PARITY_ODD;
							memcpy(ce_nlevel_param.content_key, kl_prm.ce_gen_prm.key_odd, 8);
						}
						else
						{
							//gen 128bit odd key...
							ce_nlevel_param.parity = CE_PARITY_EVEN_ODD;
							memcpy(ce_nlevel_param.content_key, kl_prm.ce_gen_prm.key_odd, KL_KEY_SIZE_MAX);
						}

						ret = casi_entry->see_ops->ioctl(casi_entry->see_chan, \
							IO_CASI_GEN_NLEVEL_TA_KEY, (__u32)&ce_nlevel_param);
						if (0 != ret) {
							dev_err(dev, "ioctl IO_CASI_GEN_NLEVEL_TA_KEY failed\n");
							goto out;
						}
					}
					break;
				case KL_CK_PARITY_ODD_EVEN:
				default:
					{
						ce_nlevel_param.parity = CE_PARITY_EVEN_ODD;

						if ((KL_ALGO_TDES == ce_nlevel_param.algo) && (KL_CK_KEY64 == key_cell->ck_size))
						{
							//gen 64bit even odd key...
							ce_nlevel_param.pos = key_cell->pos;
							memcpy(&(ce_nlevel_param.content_key[8]), kl_prm.ce_gen_prm.key_even, 8);
							memcpy(ce_nlevel_param.content_key, kl_prm.ce_gen_prm.key_odd, 8);

							ret = casi_entry->see_ops->ioctl(casi_entry->see_chan, \
								IO_CASI_GEN_NLEVEL_TA_KEY, (__u32)&ce_nlevel_param);
							if (0 != ret) {
								dev_err(dev, "ioctl IO_CASI_GEN_NLEVEL_TA_KEY failed\n");
								goto out;
							}
						}
						else
						{
							//gen 128bit even key...
							ce_nlevel_param.pos = key_cell->pos;
							memcpy(ce_nlevel_param.content_key, kl_prm.ce_gen_prm.key_even, KL_KEY_SIZE_MAX);

							ret = casi_entry->see_ops->ioctl(casi_entry->see_chan, \
								IO_CASI_GEN_NLEVEL_TA_KEY, (__u32)&ce_nlevel_param);
							if (0 != ret) {
								dev_err(dev, "ioctl IO_CASI_GEN_NLEVEL_TA_KEY failed\n");
								goto out;
							}

							//gen 128bit odd key...
							ce_nlevel_param.pos = key_cell->pos+1;
							memcpy(ce_nlevel_param.content_key, kl_prm.ce_gen_prm.key_odd, KL_KEY_SIZE_MAX);

							ret = casi_entry->see_ops->ioctl(casi_entry->see_chan, \
								IO_CASI_GEN_NLEVEL_TA_KEY, (__u32)&ce_nlevel_param);
							if (0 != ret) {
								dev_err(dev, "ioctl IO_CASI_GEN_NLEVEL_TA_KEY failed\n");
								goto out;
							}
						}
					}
					break;
			}

			break;
		}
		default:
			dev_err(dev, "unsupport cmd=0x%x\n", cmd);
			ret = -ENOTTY;
			break;
	}	
out:
	if (page) {
		kunmap(page);
		put_page(page);
	}
	up(&casi_entry->sem);
	return ret;
}

static const struct file_operations casi_dev_fops = {
	.owner		= THIS_MODULE,
	.open		= casi_dev_open,
	.release	= casi_dev_release,
	.unlocked_ioctl	= casi_dev_ioctl,
};

static struct as_attr_device g_as_attr_dev = {
	.name = ALI_CASI_DEV_NAME,
	.ops = &casi_dev_fops,
};

static int casi_probe(struct platform_device *pdev)
{
	struct device *dev;
	struct ali_casi_entry *casi_entry;
	int ret;

	dev = &pdev->dev;

	dev_info(dev, "%s: probe=%p\n", __func__, pdev);

	casi_entry = devm_kzalloc(dev, sizeof(*casi_entry), GFP_KERNEL);
	if (!casi_entry)
		return -ENOMEM;

	casi_entry->dev = &pdev->dev;
	casi_entry->as_attr_dev = g_as_attr_dev;
	casi_entry->see_chan = -1;
	casi_entry->as_attr_dev.device = &pdev->dev;

	casi_entry->as_attr_dev.driver_data = (void *)casi_entry;

	ret = as_attr_register_device(&casi_entry->as_attr_dev);
	if (ret) {
		dev_err(dev, "cannot register casi (%d)\n", ret);
		goto err;
	}

	ret = see_casi_register((void *)casi_entry);
	if (ret < 0) {
		dev_err(dev, "see casi operations register error\n");
		goto err;
	}

	sema_init(&casi_entry->sem, 1);

	platform_set_drvdata(pdev, casi_entry);

	return 0;

 err:
	return ret;
}

static int casi_remove(struct platform_device *dev)
{
	struct ali_casi_entry *casi_entry = platform_get_drvdata(dev);

	see_casi_unregister(casi_entry);

	as_attr_unregister_device(&casi_entry->as_attr_dev);

	return 0;
}

static const struct of_device_id ali_casi_dt_match[] = {
	{ .compatible = "alitech, casi", },
	{ }
};

MODULE_DEVICE_TABLE(of, ali_casi_dt_match);

static struct platform_driver casi_dev_driver = {
	.probe   = casi_probe,
	.remove  = casi_remove,
	.driver  = {
		.name  = "casi",
		.of_match_table = ali_casi_dt_match,
		.owner = THIS_MODULE
	}
};

module_platform_driver(casi_dev_driver);

MODULE_AUTHOR("ALi Corporation");
MODULE_DESCRIPTION("ALi OTP Core");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
